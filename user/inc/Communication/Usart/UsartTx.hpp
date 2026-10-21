/** ****************************************************************************
 * @file    UsartTx.hpp
 * @brief   USART/UART только для передачи через прерывания STM32F30x
 * @author  Романовский Роман
 * @date    Октябрь 2026
 *
 * @details Не содержит RX-пина и декодера. Размер TX-очереди задаётся шаблоном.
 **************************************************************************** */

#ifndef USART_TX_HPP
#define USART_TX_HPP

/* Includes ------------------------------------------------------------------*/
#include "BaseUsartTx.hpp"

namespace STM_CppLib{
    namespace STM_Usart{

        /**
         * @brief   Передатчик USART с собственным обработчиком TXE/TC.
         * @tparam  Type           Аппаратный модуль.
         * @tparam  TxPin          Вывод TX с подходящей альтернативной функцией.
         * @tparam  TxBufferSize   Число слотов; степень двойки, полезно на один меньше.
         * @note    Объект и методы управления используются из main. Перед Init()
         *          пользовательская таблица векторов должна быть установлена в VTOR.
         */
        template<UsartTypes Type, STM_GPIO::GpioPinConcept TxPin, uint32_t TxBufferSize = 256>
        class UsartTx: public BaseUsartTx<Type, TxBufferSize>,
                       public BaseIRQDevice<UsartTx<Type, TxPin, TxBufferSize>,
                                            UsartDescriptor<Type>::IRQn>{
        private:
            TxPin pin_tx; ///< Вывод передачи

        public:
            /** @brief Создаёт объект без настройки оборудования и регистрации IRQ. */
            UsartTx() = default;

            /** @brief Немедленно отключает оборудование и восстанавливает прежний вектор. */
            ~UsartTx(){
                if (this->initialized){
                    this->shutdown();
                    this->irq_device_ptr = nullptr;
                }
            }

            /** @brief Копирование и перемещение владельца USART запрещены. */
            UsartTx(const UsartTx&) = delete;
            UsartTx& operator=(const UsartTx&) = delete;
            UsartTx(UsartTx&&) = delete;
            UsartTx& operator=(UsartTx&&) = delete;

            /**
             * @brief   Настройка TX-пина, USART и IRQ, затем запуск модуля.
             * @param   baudrate   Скорость при config == nullptr.
             * @param   config     Конфигурация либо nullptr для 8N1 без flow control.
             *                     Поле Mode определяется классом и равно TX.
             * @param   preemption_priority   Приоритет IRQ.
             * @param   subpriority           Подприоритет IRQ.
             * @return  false при повторной инициализации или занятой IRQ-привязке
             *          этой специализации; true после запуска.
             * @note    Валидность пина/AF и владение модулем среди разных типов
             *          обеспечивает приложение. Байтовый API не передаёт девятый бит.
             */
            bool Init(uint32_t baudrate, const UsartConfig* config = nullptr,
                      uint8_t preemption_priority = DefaultIRQChannelPreemptionPriority,
                      uint8_t subpriority = DefaultIRQChannelSubPriority){
                if (this->initialized || this->irq_device_ptr != nullptr){ return false; }
                this->prepare_init(baudrate, config, USART_Mode_Tx);
                this->init_pin(pin_tx);

                NVIC_InitTypeDef irq_config;
                irq_config.NVIC_IRQChannel = UsartDescriptor<Type>::IRQn;
                irq_config.NVIC_IRQChannelPreemptionPriority = preemption_priority;
                irq_config.NVIC_IRQChannelSubPriority = subpriority;
                // BaseIRQDevice регистрирует вектор при выключенном канале NVIC.
                irq_config.NVIC_IRQChannelCmd = DISABLE;
                this->irq_device_ptr = this;
                this->InitInterrupt(&irq_config);
                return this->finish_init(preemption_priority, subpriority);
            }

            /** @brief Обслуживание очереди TX и флага физического завершения передачи. */
            void irq_handler(){ this->handle_tx_interrupt(); }
        };

    } // namespace STM_Usart
} // namespace STM_CppLib

#endif /*   USART_TX_HPP   */
