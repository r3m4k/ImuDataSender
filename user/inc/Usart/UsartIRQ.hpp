/** ****************************************************************************
 * @file    UsartIRQ.hpp
 * @brief   Регистрация USART и переходник аппаратного вектора через BaseIRQDevice
 * @author  Романовский Роман
 * @date    Октябрь 2026
 * @details Устройство содержит приватный объект UsartIRQ. Переходник вектора
 *          зависит только от IRQn, а не от типа прикладного устройства.
 **************************************************************************** */

#ifndef USART_IRQ_HPP
#define USART_IRQ_HPP

/* Includes ------------------------------------------------------------------*/
#include "BaseIRQDevice.hpp"
#include "UsartIRQDispatcher.hpp"

namespace STM_CppLib{
    namespace STM_Usart{

        /** 
         * @brief Единственный CRTP-переходник одного аппаратного вектора USART. 
         */
        // [Question] А зачем вообще этот класс? Вроде его логика норм будет напрямую в UsartIRQ
        template<IRQn_Type IRQn>
        class UsartIRQVector: private BaseIRQDevice<UsartIRQVector<IRQn>, IRQn>{

            template<typename UsartDevice, UsartTypes Type> friend class UsartIRQ;
            friend class BaseIRQDevice<UsartIRQVector<IRQn>, IRQn>;

            /** 
             * @brief Привязка BaseIRQDevice к внутреннему долгоживущему объекту. 
             */
            UsartIRQVector(){ this->irq_device_ptr = this; }

            /** 
             * @brief Установка вектора при ещё запрещённом канале NVIC. 
             */
            static void init_vector(){
                // [Question] Зачем? если только ради выполнения конструктора, то можно
                // this->irq_device_ptr = this; написать здесь напрямую
                static UsartIRQVector vector;   

                vector.disable_interrupt();
                vector.register_interrupt();
                vector.set_interrupt_priority();
            }

            /** @brief Передаёт общий вектор диспетчеру всех его аппаратных источников. */
            void irq_handler(){ UsartIRQDispatcher::dispatch_irq(IRQn); }
        };

        // [Question] Зачем тут UsartDevice? Если это относится именно к USART, то лучше 
        // использовать связанное название.
        /**
         * @brief  Регистрация одного долгоживущего объекта на аппаратном USART.
         * @tparam UsartDevice   Тип с методом irq_handler().
         * @tparam Type     Аппаратный USART/UART.
         */
        template<typename UsartDevice, UsartTypes Type>
        class UsartIRQ{
            using descriptor_t = UsartDescriptor<Type>;
            
            bool registered = false;    ///< Устройство успешно получило аппаратный ресурс

            /** @brief Вызывает конкретное устройство по сохранённому стабильному адресу. */
            static void dispatch_usart(void* usart_ptr){ static_cast<UsartDevice*>(usart_ptr)->irq_handler(); }

        public:
            /** @brief Создаёт объект без регистрации и настройки NVIC. */
            UsartIRQ() = default;
            UsartIRQ(const UsartIRQ&) = delete;
            UsartIRQ& operator=(const UsartIRQ&) = delete;
            UsartIRQ(UsartIRQ&&) = delete;
            UsartIRQ& operator=(UsartIRQ&&) = delete;

            /**
             * @brief Регистрирует владельца до любых изменений настроек периферии.
             * @return false при другой паре владелец/переходник на занятом ресурсе.
             * @note Повтор той же пары успешен и сохраняет приоритет общего IRQ.
             */
            bool register_device(UsartDevice& device){
                const uint32_t primask = __get_PRIMASK();
                __disable_irq();
                auto& entry = UsartIRQDispatcher::registrations[descriptor_t::UART_INDEX];
                // [Question] Зачем тут entry.handler != &dispatch_usart?
                if (entry.usart_ptr != nullptr &&
                    (entry.usart_ptr != &device || entry.handler != &dispatch_usart)){
                    __set_PRIMASK(primask);
                    return false;
                }
                entry.usart_ptr = &device;
                entry.handler = &dispatch_usart;
                const uint8_t index = UsartIRQDispatcher::usart_irq_vector_index(descriptor_t::IRQn);
                if (!UsartIRQDispatcher::initialized_vectors[index]){
                    UsartIRQVector<descriptor_t::IRQn>::init_vector();
                    UsartIRQDispatcher::initialized_vectors[index] = true;
                }
                registered = true;
                __set_PRIMASK(primask);
                return true;
            }

            /** @brief Разрешает зарегистрированный вектор, не сбрасывая его pending и приоритет. */
            void enable() const {
                if (!registered){ return; }
                UsartIRQVector<descriptor_t::IRQn>::enable_interrupt();
            }

            /**
             * @brief Изменяет приоритет всего аппаратного вектора после регистрации.
             * @param preemption_priority Приоритет вытеснения.
             * @param subpriority Подприоритет в текущей группировке NVIC.
             */
            void set_priority(uint8_t preemption_priority, uint8_t subpriority) const {
                if (!registered){ return; }
                UsartIRQVector<descriptor_t::IRQn>::set_interrupt_priority(preemption_priority, subpriority);
            }
        };

    } // namespace STM_Usart
} // namespace STM_CppLib

#endif /*   USART_IRQ_HPP   */
