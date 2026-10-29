/** ****************************************************************************
 * @file    ExtiIRQ.hpp
 * @brief   Регистрация прерываний EXTI по типу пина
 * @author  Романовский Роман
 * @date    Октябрь 2026
 *
 * @details GPIO_EXTI_n связывает статический обработчик пина с общим диспетчером
 *          и регистрирует аппаратный вектор через существующий BaseIRQDevice.
 *          Объект регистрации должен существовать всё время работы IRQ.
 **************************************************************************** */

#ifndef EXTI_IRQ_HPP
#define EXTI_IRQ_HPP

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

#include "Exti.hpp"
#include "ExtiIRQDispatcher.hpp"
#include "BaseIRQDevice.hpp"

namespace STM_CppLib{
    namespace STM_EXTI{

        /**
         * @brief   Регистрация линии и перенаправление IRQ в общий диспетчер EXTI.
         * @tparam  PinExti_t Тип пина с get_port(), get_pin_source() и статическим irq_handler().
         * @note    Объект хранится внутри GPIO_Pin_EXTI и не копируется и не перемещается.
         *          Совмещённый EXTI2_TS обслуживается только в части линии GPIO EXTI2.
         */
        template<typename PinExti_t>
        class GPIO_EXTI_n:
            public BaseIRQDevice<GPIO_EXTI_n<PinExti_t>,
                                 EXTI_Descriptor<PinExti_t::get_port(), 
                                 PinExti_t::get_pin_source()>::IRQn>
        {
            using descriptor_t = EXTI_Descriptor<
                PinExti_t::get_port(), PinExti_t::get_pin_source()>;
            using irq_base_t = BaseIRQDevice<GPIO_EXTI_n<PinExti_t>, descriptor_t::IRQn>;

        public:
            /**
             * @brief Привязывает переходник BaseIRQDevice к текущему объекту.
             * @note Конструктор не разрешает аппаратные прерывания.
             */
            GPIO_EXTI_n(){
                this->irq_device_ptr = this;
            }

            GPIO_EXTI_n(const GPIO_EXTI_n&) = delete;
            GPIO_EXTI_n& operator=(const GPIO_EXTI_n&) = delete;
            GPIO_EXTI_n(GPIO_EXTI_n&&) = delete;
            GPIO_EXTI_n& operator=(GPIO_EXTI_n&&) = delete;

            /**
             * @brief Регистрирует обработчик линии и настраивает её вектор NVIC.
             * @param NVIC_InitStructure_ptr Явная настройка NVIC либо nullptr для значений по умолчанию.
             * @return false при занятой линии или несовпадении номера IRQ в структуре.
             * @note Повторная регистрация того же переходника успешна. Без явной структуры
             *       уже настроенный общий вектор сохраняет приоритет и состояние разрешения.
             *       Вызывать после создания всего объекта пина, из основного кода.
             */
            bool InitInterrupt(NVIC_InitTypeDef* NVIC_InitStructure_ptr = nullptr)
            {
                constexpr uint8_t pin_source = PinExti_t::get_pin_source();

                if ((NVIC_InitStructure_ptr != nullptr) &&
                    (NVIC_InitStructure_ptr->NVIC_IRQChannel != descriptor_t::IRQn)){
                    return false;
                }

                constexpr uint8_t vector_index = (pin_source < 5) ? pin_source :
                    ((pin_source < 10) ? 5 : 6);
                constexpr uint8_t vector_mask = static_cast<uint8_t>(1U << vector_index);

                const uint32_t primask = __get_PRIMASK();
                __disable_irq();

                if (!EXTI_IRQ_Dispatcher::register_handler(pin_source, &PinExti_t::irq_handler)){
                    __set_PRIMASK(primask);
                    return false;
                }

                if (((EXTI_IRQ_Dispatcher::initialized_vectors & vector_mask) == 0) ||
                    (NVIC_InitStructure_ptr != nullptr)){
                    irq_base_t::InitInterrupt(NVIC_InitStructure_ptr);
                    EXTI_IRQ_Dispatcher::initialized_vectors |= vector_mask;
                }

                __set_PRIMASK(primask);
                return true;
            }

            /**
             * @brief Изменяет приоритет NVIC без изменения состояния разрешения IRQ.
             * @param preemption_priority Приоритет вытеснения.
             * @param sub_priority Подприоритет в текущей группировке NVIC.
             * @note Вызывать после InitInterrupt(). Для общего IRQ изменение касается всех его линий.
             */
            static void SetInterruptPriority(uint8_t preemption_priority, uint8_t sub_priority){
                NVIC_SetPriority(descriptor_t::IRQn,
                    NVIC_EncodePriority(NVIC_GetPriorityGrouping(), preemption_priority, sub_priority));
            }

            /**
             * @brief Передаёт обработку своего вектора общему диспетчеру EXTI.
             * @note Вызывается переходником BaseIRQDevice в контексте IRQ.
             */
            void irq_handler(){
                EXTI_IRQ_Dispatcher::dispatch_irq(descriptor_t::IRQn);
            }
        };

    } // namespace STM_EXTI
} // namespace STM_CppLib

#endif /*   EXTI_IRQ_HPP   */
