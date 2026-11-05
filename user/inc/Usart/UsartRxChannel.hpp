/** ****************************************************************************
 * @file    UsartRxChannel.hpp
 * @brief   RX-пин, аппаратный приём USART и пользовательский декодер
 * @author  Романовский Роман
 * @date    Октябрь 2026
 * @details Декодер выполняется в IRQ. Программная RX-очередь не создаётся.
 *          Компонент не включает общий модуль и не регистрирует аппаратный IRQ.
 **************************************************************************** */

#ifndef USART_RX_CHANNEL_HPP
#define USART_RX_CHANNEL_HPP

/* Includes ------------------------------------------------------------------*/
#include "GpioPin.hpp"
#include "CheckUsartGPIO.hpp"
#include "ByteDecoder.hpp"
#include "stm32f30x_usart.h"
#include "UsartDescriptor.hpp"

namespace STM_CppLib{
    namespace STM_Usart{

        /**
         * @brief RX-компонент с собственными пином и состоянием декодера.
         * @tparam Type Аппаратный USART/UART.
         * @tparam RxPin Тип вывода приёма.
         * @tparam Decoder Тип с методом void byte_processing(uint8_t).
         */
        template<UsartTypes Type, STM_GPIO::GpioPinConcept RxPin, ByteDecoderConcept Decoder>
        class UsartRxChannel{
            using descriptor_t = UsartDescriptor<Type>;

            static_assert(
                is_usart_rx_gpio(Type, RxPin::get_port(), RxPin::get_pin_source()),
                "Selected GPIO does not support RX for this USART"
            );
            
            static_assert(std::default_initializable<Decoder>, "USART decoder must be default-initializable");

            USART_TypeDef* const usart = descriptor_t::get_usart(); ///< Регистры аппаратного модуля
            RxPin pin_rx; ///< Вывод приёма
            Decoder decoder; ///< Сохраняется при Stop(), Start() и повторном Init()

            /**
             * @brief Читает слово и очищает зафиксированные ошибки до пользовательского вызова.
             * @param byte Принятый байт при результате true.
             * @return true для слова без PE/FE/NE. ORE означает потерю следующего слова,
             *         но не требует отбрасывать уже находящееся в RDR слово.
             */
            bool read_byte(uint8_t& byte){
                const uint32_t status = usart->ISR;
                const bool received = (status & USART_ISR_RXNE) != 0;
                // А что проверяется то?
                if (received){ 
                    byte = static_cast<uint8_t>(USART_ReceiveData(usart)); 
                }
                const uint32_t errors = status & (USART_ISR_PE | USART_ISR_FE | USART_ISR_NE | USART_ISR_ORE);
                if (errors != 0){ usart->ICR = errors; }
                return received && (status & (USART_ISR_PE | USART_ISR_FE | USART_ISR_NE)) == 0;
            }

        public:
            /**
             * @brief Аппаратная маска направления для общей инициализации USART.
             */
            static constexpr uint32_t DIRECTION = USART_Mode_Rx;

            /**
             * @brief Настраивает RX-пин и очищает старые аппаратные данные и ошибки.
             * @note Вызывается после общей настройки при остановленном USART. Декодер сохраняется.
             */
            void init(){
                pin_rx.InitPin(GPIO_Mode_AF, GPIO_PuPd_UP);
                GPIO_PinAFConfig(
                    STM_GPIO::GPIO_PortDescriptor<RxPin::get_port()>::get_GPIO_Type(),
                    RxPin::get_pin_source(), 
                    descriptor_t::AF
                );
                USART_RequestCmd(usart, USART_Request_RXFRQ, ENABLE);
                
                // [Question] Что тут делаем?
                usart->ICR = USART_ICR_PECF | USART_ICR_FECF | USART_ICR_NCF | USART_ICR_ORECF;
            }

            /**
             * @brief Разрешает источники RXNE и ошибок после запуска USART.
             * @note Вызывается до возобновления TX; повторный Start() устройства каналы не перенастраивает.
             */
            void enable_interrupts(){
                USART_ITConfig(usart, USART_IT_RXNE, ENABLE);
                USART_ITConfig(usart, USART_IT_ERR, ENABLE);
                USART_ITConfig(usart, USART_IT_PE, ENABLE);
            }

            /**
             * @brief Отключает только источники приёма и его ошибок.
             * @note Вызывать в критической секции после проверки активности приёма.
             */
            void disable_interrupts(){
                USART_ITConfig(usart, USART_IT_RXNE, DISABLE);
                USART_ITConfig(usart, USART_IT_ERR, DISABLE);
                USART_ITConfig(usart, USART_IT_PE, DISABLE);
            }

            /**
             * @brief Проверяет приём отдельного слова по аппаратному BUSY.
             * @note Вызывать только при запущенном USART; состояние запуска проверяет устройство.
             */
            bool is_busy() const { return USART_GetFlagStatus(usart, USART_FLAG_BUSY) == SET; }

            /** @brief Передаёт корректный принятый байт декодеру после чтения RDR и очистки ошибок. */
            void handle_interrupt(){
                uint8_t byte;
                if (read_byte(byte)){ decoder.byte_processing(byte); }
            }
        };

    } // namespace STM_Usart
} // namespace STM_CppLib

#endif /*   USART_RX_CHANNEL_HPP   */
