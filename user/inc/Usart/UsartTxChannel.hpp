/** ****************************************************************************
 * @file    UsartTxChannel.hpp
 * @brief   TX-пин, очередь и аппаратное обслуживание передачи USART
 * @author  Романовский Роман
 * @date    Октябрь 2026
 * @details Единственный писатель работает в main, читатель — в USART IRQ.
 *          Компонент не включает общий модуль и не регистрирует аппаратный IRQ.
 **************************************************************************** */

#ifndef USART_TX_CHANNEL_HPP
#define USART_TX_CHANNEL_HPP

/* Includes ------------------------------------------------------------------*/
#include "GpioPin.hpp"
#include "SpscRingBuffer.hpp"
#include "stm32f30x_usart.h"
#include "UsartDescriptor.hpp"

namespace STM_CppLib{
    namespace STM_Usart{

        /**
         * @brief TX-компонент с собственным пином и очередью main → IRQ.
         * @tparam Type Аппаратный USART/UART.
         * @tparam TxPin Тип вывода передачи.
         * @tparam BufferSize Число слотов; полезная ёмкость равна BufferSize - 1 байт.
         */
        template<UsartTypes Type, STM_GPIO::GpioPinConcept TxPin, uint32_t BufferSize>
        class UsartTxChannel{
            using descriptor_t = UsartDescriptor<Type>;

            static_assert(TxPin::get_pin_source() < 16, "Invalid USART TX pin");

            USART_TypeDef* const usart = descriptor_t::get_usart(); ///< Регистры аппаратного модуля
            TxPin pin_tx; ///< Вывод передачи
            Containers::SpscRingBuffer<uint8_t, BufferSize> buffer; ///< Очередь main → IRQ
            volatile bool tx_written = false; ///< IRQ устанавливает после записи TDR, main проверяет TC

            /** @brief Записывает байт в TDR; аппаратно сбрасываются TXE и TC. */
            void write_byte(uint8_t byte){
                USART_SendData(usart, byte);
                tx_written = true;
            }

            /** @brief Запрашивает TXE из main без гонки с обновлением CR1 в IRQ. */
            void request_tx(){
                const uint32_t primask = __get_PRIMASK();
                __disable_irq();
                USART_ITConfig(usart, USART_IT_TC, DISABLE);
                USART_ITConfig(usart, USART_IT_TXE, ENABLE);
                __set_PRIMASK(primask);
            }

        public:
            /**
             * @brief Аппаратная маска направления для общей инициализации USART.
             */
            static constexpr uint32_t DIRECTION = USART_Mode_Tx;

            /**
             * @brief Настраивает TX-пин и сбрасывает признак записи TDR, сохраняя очередь.
             * @note Вызывается после общей настройки при остановленном USART.
             */
            void init(){
                pin_tx.InitPin(GPIO_Mode_AF, GPIO_PuPd_UP);
                GPIO_PinAFConfig(
                    STM_GPIO::GPIO_PortDescriptor<TxPin::get_port()>::get_GPIO_Type(),
                    TxPin::get_pin_source(), descriptor_t::AF);
                tx_written = false;
            }

            /**
             * @brief Копирует массив целиком и запускает обслуживание TXE.
             * @param data Источник; nullptr допустим только при size == 0.
             * @param size Количество байтов.
             * @return false без частичной записи при нехватке места или неверном указателе.
             * @note Только из main после запуска USART. Проверку запуска выполняет устройство.
             *       Копирование выполняется с исходной маской прерываний.
             */
            bool send_buffer(const uint8_t* data, uint32_t size){
                if (!buffer.put_buffer(data, size)){ return false; }
                if (size != 0){ resume(); }
                return true;
            }

            /** @brief Возобновляет сохранённую очередь после запуска USART. */
            void resume(){
                // Если IRQ уже опустошил очередь, лишний TXE будет отключён им без записи TDR.
                if (!buffer.is_empty()){ request_tx(); }
            }

            /** @brief Обслуживает TXE и TC в контексте единственного IRQ-читателя. */
            void handle_interrupt(){
                if (USART_GetITStatus(usart, USART_IT_TXE) == SET){
                    if (!buffer.is_empty()){ write_byte(buffer.get()); }
                    if (buffer.is_empty()){
                        USART_ITConfig(usart, USART_IT_TXE, DISABLE);
                        USART_ITConfig(usart, USART_IT_TC, ENABLE);
                    }
                }
                if (USART_GetITStatus(usart, USART_IT_TC) == SET){
                    // TC остаётся установленным: IsTxBusy() проверяет физическое состояние.
                    USART_ITConfig(usart, USART_IT_TC, DISABLE);
                }
            }

            /**
             * @brief Проверяет незавершённую аппаратную передачу независимо от очереди.
             * @note Вызывать только при запущенном USART; состояние запуска проверяет устройство.
             */
            bool is_busy() const {
                return tx_written && USART_GetFlagStatus(usart, USART_FLAG_TC) == RESET;
            }

            /**
             * @brief Отключает только источники TXE и TC данного USART.
             * @note Вызывать в критической секции после проверки активности передачи.
             */
            void disable_interrupts(){
                USART_ITConfig(usart, USART_IT_TXE, DISABLE);
                USART_ITConfig(usart, USART_IT_TC, DISABLE);
            }

            /** @brief Возвращает число свободных байтов с точки зрения писателя из main. */
            uint32_t free_space() const { return buffer.free_space(); }

            /** @brief Возвращает число байтов только в программной очереди, без TDR и сдвигового регистра. */
            uint32_t buffered_data_size() const { return BufferSize - 1U - buffer.free_space(); }
        };

    } // namespace STM_Usart
} // namespace STM_CppLib

#endif /*   USART_TX_CHANNEL_HPP   */
