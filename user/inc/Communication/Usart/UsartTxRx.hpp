/** ****************************************************************************
 * @file    UsartTxRx.hpp
 * @brief   USART/UART с передачей по IRQ и статически выбранным декодером приёма
 * @author  Романовский Роман
 * @date    Октябрь 2026
 *
 * @details Декодер хранится внутри объекта. Конкретный протокол задаётся
 *          приложением; этот заголовок не подключает DecoderImu или команды.
 **************************************************************************** */

#ifndef USART_TX_RX_HPP
#define USART_TX_RX_HPP

/* Includes ------------------------------------------------------------------*/
#include "BaseUsartTx.hpp"
#include "Consepts/ByteDecoder.hpp"

namespace STM_CppLib{
    namespace STM_Usart{

        /**
         * @brief   USART с общей IRQ-привязкой для передачи и приёма.
         * @tparam  Type           Аппаратный модуль.
         * @tparam  TxPin          Вывод передачи.
         * @tparam  RxPin          Вывод приёма.
         * @tparam  Decoder        Тип собственного обработчика входных байтов.
         * @tparam  TxBufferSize   Число слотов; степень двойки, полезно на один меньше.
         * @note    Декодер создаётся без аргументов и выполняется в RX IRQ.
         *          Он должен быть коротким, не блокировать IRQ и не записывать
         *          в TX-очередь этого порта: её единственный писатель работает в main.
         *          Перед Init() пользовательская таблица векторов установлена в VTOR.
         */
        template<UsartTypes Type, 
                 STM_GPIO::GpioPinConcept TxPin,
                 STM_GPIO::GpioPinConcept RxPin, 
                 ByteDecoder Decoder,
                 uint32_t TxBufferSize = 256>
        class UsartTxRx: public BaseUsartTx<Type, TxBufferSize>,
                         public BaseIRQDevice<UsartTxRx<Type, TxPin, RxPin, Decoder, TxBufferSize>,
                                              UsartDescriptor<Type>::IRQn>{
            static_assert(std::default_initializable<Decoder>,
                          "USART decoder must be default-initializable");

        private:
            TxPin pin_tx; ///< Вывод передачи
            RxPin pin_rx; ///< Вывод приёма
            Decoder decoder; ///< Состояние обработки одного входного потока

        public:
            /** @brief Создаёт пины и декодер без настройки оборудования. */
            UsartTxRx() = default;

            /** @brief Отключает IRQ до разрушения декодера; текущая передача прерывается. */
            ~UsartTxRx(){
                if (this->initialized){
                    this->shutdown();
                    this->irq_device_ptr = nullptr;
                }
            }

            /** @brief Копирование и перемещение владельца USART запрещены. */
            UsartTxRx(const UsartTxRx&) = delete;
            UsartTxRx& operator=(const UsartTxRx&) = delete;
            UsartTxRx(UsartTxRx&&) = delete;
            UsartTxRx& operator=(UsartTxRx&&) = delete;

            /**
             * @brief   Настройка пинов, USART и общего IRQ, затем запуск модуля.
             * @param   baudrate   Скорость при config == nullptr.
             * @param   config     Конфигурация либо nullptr для 8N1 без flow control.
             *                     Поле Mode определяется классом и равно TX | RX.
             * @param   preemption_priority   Приоритет вытеснения IRQ.
             * @param   subpriority           Подприоритет IRQ.
             * @return  false при повторной инициализации или занятой IRQ-привязке
             *          этой специализации; true после запуска.
             * @note    RXNE разрешается отдельно. Валидность пинов/AF и владение
             *          модулем среди разных типов обеспечивает приложение.
             *          Байтовый API не сохраняет девятый бит принятого слова.
             */
            bool Init(uint32_t baudrate, const UsartConfig* config = nullptr,
                      uint8_t preemption_priority = DefaultIRQChannelPreemptionPriority,
                      uint8_t subpriority = DefaultIRQChannelSubPriority){
                if (this->initialized || this->irq_device_ptr != nullptr){ return false; }
                this->prepare_init(baudrate, config, USART_Mode_Tx | USART_Mode_Rx);
                this->init_pin(pin_tx);
                this->init_pin(pin_rx);

                NVIC_InitTypeDef irq_config;
                irq_config.NVIC_IRQChannel = UsartDescriptor<Type>::IRQn;
                irq_config.NVIC_IRQChannelPreemptionPriority = preemption_priority;
                irq_config.NVIC_IRQChannelSubPriority = subpriority;
                irq_config.NVIC_IRQChannelCmd = DISABLE;
                this->irq_device_ptr = this;
                this->InitInterrupt(&irq_config);
                return this->finish_init(preemption_priority, subpriority);
            }

            /** @brief Разрешает передачу принятых байтов декодеру после Init(). */
            void EnableRxInterrupt(){ this->set_rx_interrupt(true); }

            /** @brief Запрещает RXNE, сохраняя работу передачи через TX IRQ. */
            void DisableRxInterrupt(){ this->set_rx_interrupt(false); }

            /** @brief Обрабатывает RXNE, очищает ORE и обслуживает TXE/TC. */
            void irq_handler(){
                if (USART_GetITStatus(this->usart, USART_IT_RXNE) == SET){
                    const uint8_t byte = static_cast<uint8_t>(USART_ReceiveData(this->usart));
                    decoder.byte_processing(byte);
                }
                USART_ClearITPendingBit(this->usart, USART_IT_ORE);
                this->handle_tx_interrupt();
            }
        };

    } // namespace STM_Usart
} // namespace STM_CppLib

#endif /*   USART_TX_RX_HPP   */
