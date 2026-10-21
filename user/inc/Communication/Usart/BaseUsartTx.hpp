/** ****************************************************************************
 * @file    BaseUsartTx.hpp
 * @brief   Общая реализация передачи USART через очередь и прерывания
 * @author  Романовский Роман
 * @date    Октябрь 2026
 *
 * @details Основа UsartTx и UsartTxRx для STM32F30x. Единственный писатель
 *          TX-очереди работает в main, единственный читатель — в USART IRQ.
 *          Отправка копирует буфер целиком либо отказывает без записи его части.
 *          Блокирующих методов отправки и динамической аллокации нет.
 **************************************************************************** */

#ifndef BASE_USART_TX_HPP
#define BASE_USART_TX_HPP

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

#include "BaseIRQDevice.hpp"
#include "BasePackage.hpp"
#include "BinaryData.hpp"
#include "GpioPin.hpp"
#include "SpscRingBuffer.hpp"
#include "UsartConfig.hpp"
#include "UsartDescriptor.hpp"

namespace STM_CppLib{
    namespace STM_Usart{

        /**
         * @brief   Очередь TX, аппаратное состояние и общий обработчик TXE/TC.
         * @tparam  Type           Аппаратный модуль USART/UART.
         * @tparam  TxBufferSize   Число слотов; степень двойки, не меньше 2.
         * @note    Полезная ёмкость равна TxBufferSize - 1 байт.
         *          Один аппаратный модуль может иметь только одного владельца,
         *          в том числе среди разных специализаций и старого Usart.
         *          Инициализация и управление портом выполняются из main.
         */
        template<UsartTypes Type, uint32_t TxBufferSize>
        class BaseUsartTx{
            static_assert(Type == UsartTypes::Usart1 || Type == UsartTypes::Usart2 ||
                          Type == UsartTypes::Usart3 || Type == UsartTypes::Uart4 ||
                          Type == UsartTypes::Uart5, "Unsupported USART type");

        protected:
            USART_TypeDef* const usart = UsartDescriptor<Type>::get_USARTx(); ///< Регистры модуля

        private:
            Containers::SpscRingBuffer<uint8_t, TxBufferSize> tx_buffer; ///< Очередь исходящих байтов
            volatile uint32_t tx_busy = 0; ///< Флаг main/IRQ: передача ещё не завершена по TC
            bool started = false; ///< Модуль разрешён для отправки

        protected:
            /** @brief Создаёт очередь без обращения к аппаратным регистрам. */
            BaseUsartTx() = default;

            /** @brief Наследник отключает IRQ через shutdown() до разрушения своих полей. */
            ~BaseUsartTx() = default;

            /**
             * @brief   Подготовка регистров до регистрации IRQ конечного класса.
             * @param   baudrate   Скорость для конфигурации по умолчанию.
             * @param   config     Пользовательская конфигурация либо nullptr.
             * @param   mode       Направления, определяемые конечным классом.
             */
            void prepare_init(uint32_t baudrate, const UsartConfig* config, uint32_t mode){
                UsartDescriptor<Type>::PeriphClockCmd(UsartDescriptor<Type>::RCC_Periph, ENABLE);
                NVIC_DisableIRQ(UsartDescriptor<Type>::IRQn);
                __DSB();
                __ISB();

                USART_ITConfig(usart, USART_IT_TXE, DISABLE);
                USART_ITConfig(usart, USART_IT_TC, DISABLE);
                USART_ITConfig(usart, USART_IT_RXNE, DISABLE);
                USART_Cmd(usart, DISABLE);
                started = false;

                UsartConfig selected = config != nullptr ? *config : UsartConfig{
                    .BaudRate = baudrate,
                    .WordLength = USART_WordLength_8b,
                    .StopBits = USART_StopBits_1,
                    .Parity = USART_Parity_No,
                    .Mode = mode,
                    .HardwareFlowControl = USART_HardwareFlowControl_None
                };
                selected.Mode = mode;

                USART_InitTypeDef hardware_config;
                hardware_config.USART_BaudRate = selected.BaudRate;
                hardware_config.USART_WordLength = selected.WordLength;
                hardware_config.USART_StopBits = selected.StopBits;
                hardware_config.USART_Parity = selected.Parity;
                hardware_config.USART_Mode = selected.Mode;
                hardware_config.USART_HardwareFlowControl = selected.HardwareFlowControl;
                USART_Init(usart, &hardware_config);
            }

            /**
             * @brief   Настройка вывода для альтернативной функции USART.
             * @tparam  Pin   Тип вывода GPIO.
             * @param   pin   Объект настраиваемого вывода.
             */
            template<STM_GPIO::GpioPinConcept Pin>
            static void init_pin(Pin& pin){
                pin.InitPin(GPIO_Mode_AF, GPIO_PuPd_UP);
                GPIO_PinAFConfig(
                    STM_GPIO::GPIO_PortDescriptor<Pin::get_port()>::get_GPIO_Type(),
                    Pin::get_pin_source(), UsartDescriptor<Type>::AF);
            }

            /**
             * @brief   Завершение инициализации после регистрации IRQ.
             * @param   preemption_priority   Приоритет вытеснения IRQ.
             * @param   subpriority           Подприоритет IRQ.
             * @return  Результат запуска модуля.
             */
            bool finish_init(uint8_t preemption_priority, uint8_t subpriority){
                // NVIC_Init(DISABLE) не записывает приоритет в используемой SPL.
                NVIC_SetPriority(UsartDescriptor<Type>::IRQn,
                    NVIC_EncodePriority(NVIC_GetPriorityGrouping(), preemption_priority, subpriority));
                NVIC_ClearPendingIRQ(UsartDescriptor<Type>::IRQn);
                return start();
            }

            /**
             * @brief   Немедленное отключение IRQ и модуля при уничтожении.
             * @note    Незавершённая передача прерывается. Для сохранения данных
             *          до уничтожения следует дождаться !IsTxBusy() в приложении.
             */
            void shutdown(){
                NVIC_DisableIRQ(UsartDescriptor<Type>::IRQn);
                __DSB();
                __ISB();
                USART_ITConfig(usart, USART_IT_TXE, DISABLE);
                USART_ITConfig(usart, USART_IT_TC, DISABLE);
                USART_ITConfig(usart, USART_IT_RXNE, DISABLE);
                USART_Cmd(usart, DISABLE);
                NVIC_ClearPendingIRQ(UsartDescriptor<Type>::IRQn);
                started = false;
                tx_busy = 0;
            }

            /** @brief Обработка TXE и физического завершения передачи по TC. */
            void handle_tx_interrupt(){
                if (USART_GetITStatus(usart, USART_IT_TXE) == SET){
                    if (!tx_buffer.is_empty()){
                        USART_SendData(usart, tx_buffer.get());
                    }
                    if (tx_buffer.is_empty()){
                        USART_ITConfig(usart, USART_IT_TXE, DISABLE);
                        USART_ITConfig(usart, USART_IT_TC, ENABLE);
                    }
                }

                if (USART_GetITStatus(usart, USART_IT_TC) == SET){
                    USART_ITConfig(usart, USART_IT_TC, DISABLE);
                    USART_ClearITPendingBit(usart, USART_IT_TC);
                    if (tx_buffer.is_empty()){
                        tx_busy = 0;
                    }
                    else{
                        // Новые данные могли быть опубликованы до запуска TXE из main.
                        USART_ITConfig(usart, USART_IT_TXE, ENABLE);
                    }
                }
            }

            /**
             * @brief   Изменение RXNE без гонки с обновлением CR1 из TX IRQ.
             * @param   enabled   Разрешить либо запретить RXNE.
             */
            void set_rx_interrupt(bool enabled){
                const uint32_t primask = __get_PRIMASK();
                __disable_irq();
                USART_ITConfig(usart, USART_IT_RXNE, enabled ? ENABLE : DISABLE);
                __set_PRIMASK(primask);
            }

            /**
             * @brief   Запуск модуля после регистрации IRQ конечным классом.
             * @return  true после разрешения модуля и NVIC.
             */
            bool start(){
                if (started){ return true; }
                USART_Cmd(usart, ENABLE);
                started = true;
                // Запись RAM-вектора должна завершиться до разрешения IRQ.
                __DSB();
                __ISB();
                NVIC_EnableIRQ(UsartDescriptor<Type>::IRQn);
                return true;
            }

        public:
            /** @brief Копирование и перемещение аппаратного владельца запрещены. */
            BaseUsartTx(const BaseUsartTx&) = delete;
            BaseUsartTx& operator=(const BaseUsartTx&) = delete;
            BaseUsartTx(BaseUsartTx&&) = delete;
            BaseUsartTx& operator=(BaseUsartTx&&) = delete;

            /**
             * @brief   Остановка без ожидания передачи.
             * @return  false при незавершённой передаче; модуль продолжит работу.
             *          true, если модуль остановлен либо ещё не инициализирован.
             */
            [[nodiscard]] bool Stop(){
                if (!started){ return true; }
                if (IsTxBusy()){ return false; }
                NVIC_DisableIRQ(UsartDescriptor<Type>::IRQn);
                __DSB();
                __ISB();
                USART_ITConfig(usart, USART_IT_TXE, DISABLE);
                USART_ITConfig(usart, USART_IT_TC, DISABLE);
                USART_Cmd(usart, DISABLE);
                started = false;
                return true;
            }

            /**
             * @brief   Количество свободных байтов в TX-очереди.
             */
            uint32_t GetTxFreeSpace() const { return tx_buffer.free_space(); }

            /**
             * @brief   Проверка наличия принятой, но ещё не завершённой передачи.
             * @return  true до опустошения очереди и обработки TC последнего байта.
             */
            bool IsTxBusy() const {
                return !tx_buffer.is_empty() || tx_busy != 0;
            }

            /**
             * @brief   Копирование всего буфера в очередь с запуском TX IRQ.
             * @param   data   Источник; nullptr допустим только при size == 0.
             * @param   size   Количество байтов.
             * @return  true, если буфер принят целиком; false без записи его части
             *          при нехватке места, невалидном источнике или остановленном порте.
             * @note    Только один писатель из main. Нельзя вызывать из RX IRQ
             *          или других прерываний. Исходные данные можно сразу переиспользовать.
             *          Успех означает приём в очередь, а не физическое завершение.
             */
            [[nodiscard]] bool SendBuffer(const uint8_t* data, uint32_t size){
                if (!started){ return false; }
                if (!tx_buffer.put_buffer(data, size)){ return false; }
                if (size == 0){ return true; }

                // Только обновление CR1 и состояния; копирование выполнено с разрешёнными IRQ.
                const uint32_t primask = __get_PRIMASK();
                __disable_irq();
                // Работающий IRQ мог уже забрать опубликованный массив.
                // В этом случае сохраняем установленное им состояние TC.
                if (!tx_buffer.is_empty()){
                    tx_busy = 1;
                    USART_ITConfig(usart, USART_IT_TC, DISABLE);
                    USART_ITConfig(usart, USART_IT_TXE, ENABLE);
                }
                __set_PRIMASK(primask);
                return true;
            }

            /**
             * @brief   Добавление одного байта в TX-очередь.
             * @param   byte   Отправляемый байт.
             * @return  Результат SendBuffer().
             */
            [[nodiscard]] bool SendByte(uint8_t byte){ return SendBuffer(&byte, 1); }

            /**
             * @brief   Копирование пакета в TX-очередь.
             * @param   package   Пакет с данными и длиной.
             * @return  Результат SendBuffer().
             */
            [[nodiscard]] bool SendPackage(const Packages::BasePackage& package){
                return SendBuffer(package.data_ptr, package.len);
            }

            /**
             * @brief   Копирование сообщения в TX-очередь.
             * @param   message   Представление значащих байтов сообщения.
             * @return  Результат SendBuffer().
             */
            [[nodiscard]] bool SendMessage(const BinaryData::MutableByteView& message){
                return SendBuffer(message.bytes_msg, message.msg_size);
            }
        };

    } // namespace STM_Usart
} // namespace STM_CppLib

#endif /*   BASE_USART_TX_HPP   */
