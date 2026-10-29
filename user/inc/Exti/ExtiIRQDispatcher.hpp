/** ****************************************************************************
 * @file    ExtiIRQDispatcher.hpp
 * @brief   Общее хранилище и диспетчер обработчиков линий EXTI
 * @author  Романовский Роман
 * @date    Октябрь 2026
 *
 * @details Все специализации GPIO_EXTI_n используют одну таблицу из 16 функций.
 *          Диспетчер очищает pending-флаг перед вызовом обработчика линии.
 **************************************************************************** */

#ifndef EXTI_IRQ_DISPATCHER_HPP
#define EXTI_IRQ_DISPATCHER_HPP

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

#include "stm32f30x.h"
#include "stm32f30x_exti.h"

#include "Consts.hpp"
#include "BaseIRQDevice.hpp"

namespace STM_CppLib{
    namespace STM_EXTI{

        template<typename PinExti_t>
        class GPIO_EXTI_n;

        /**
         * @brief Общее состояние регистрации и распределение событий EXTI.
         * @note Линии GPIO регистрируются через GPIO_EXTI_n, TSC — через RegisterTSCHandler().
         *       Создание объектов пользователем не требуется.
         */
        class EXTI_IRQ_Dispatcher:
            private BaseIRQDevice<EXTI_IRQ_Dispatcher, EXTI2_TS_IRQn>
        {

            template<typename PinExti_t>
            friend class GPIO_EXTI_n;

            friend class BaseIRQDevice<EXTI_IRQ_Dispatcher, EXTI2_TS_IRQn>;

            inline static handler_t irq_vectors[16] = {};
            inline static handler_t tsc_handler = nullptr;
            inline static uint8_t initialized_vectors = 0;

            /**
             * @brief Привязывает переходник BaseIRQDevice к текущему диспетчеру.
             * @note Создаётся при регистрации TSC, если общий вектор ещё не настроен.
             */
            EXTI_IRQ_Dispatcher(){
                this->irq_device_ptr = this;
            }

            /**
             * @brief Передаёт совмещённый IRQ обработчикам EXTI2 и TSC.
             * @note Вызывается переходником BaseIRQDevice в контексте прерывания.
             */
            void irq_handler(){
                dispatch_irq(EXTI2_TS_IRQn);
            }

            /**
             * @brief Регистрирует обработчик линии EXTI.
             * @note Повторная регистрация той же функции успешна; занятая линия или неверные аргументы дают false.
             */
            static bool register_handler(uint8_t pin_source, handler_t handler){
                if ((pin_source >= 16) || (handler == nullptr)){
                    return false;
                }

                if ((irq_vectors[pin_source] != nullptr) &&
                    (irq_vectors[pin_source] != handler)){
                    return false;
                }

                irq_vectors[pin_source] = handler;
                return true;
            }

            /**
             * @brief   Обрабатывает ожидающее прерывание указанной линии EXTI.
             * @note    Pending-флаг очищается перед вызовом функции, даже если обработчик не зарегистрирован.
             */
            static void dispatch_line(uint8_t pin_source){
                if (EXTI_GetITStatus(pin_source) == RESET){
                    return;
                }

                EXTI_ClearITPendingBit(pin_source);

                if (irq_vectors[pin_source] != nullptr){
                    irq_vectors[pin_source]();
                }
            }

            /**
             * @brief   Направляет события вектора IRQ обработчикам соответствующих линий EXTI.
             * @note    Для общего IRQ проверяются все его линии; неизвестный вектор игнорируется.
             */
            static void dispatch_irq(IRQn_Type irqn){
                switch (irqn){
                    case EXTI0_IRQn:
                        dispatch_line(0);
                        break;
                    case EXTI1_IRQn:
                        dispatch_line(1);
                        break;
                    case EXTI2_TS_IRQn:
                        dispatch_line(2);
                        if (tsc_handler != nullptr){
                            tsc_handler();
                        }
                        break;
                    case EXTI3_IRQn:
                        dispatch_line(3);
                        break;
                    case EXTI4_IRQn:
                        dispatch_line(4);
                        break;
                    case EXTI9_5_IRQn:
                        for (uint8_t pin_source = 5; pin_source <= 9; pin_source++){
                            dispatch_line(pin_source);
                        }
                        break;
                    case EXTI15_10_IRQn:
                        for (uint8_t pin_source = 10; pin_source <= 15; pin_source++){
                            dispatch_line(pin_source);
                        }
                        break;
                    default:
                        break;
                }
            }
            
        public:
            /**
             * @brief Регистрирует обработчик TSC в совмещённом векторе EXTI2_TS.
             * @param handler Функция проверки, обработки и очистки флагов прерываний TSC.
             * @return false при nullptr или попытке заменить зарегистрированную функцию другой.
             * @note Вызывать из основного кода. Повторная регистрация той же функции успешна.
             *       Функция вызывается и при событии EXTI2, поэтому сама проверяет флаги TSC.
             *       Вектор NVIC при необходимости настраивается со значениями по умолчанию;
             *       уже настроенный вектор сохраняется. Источники прерываний TSC не включаются.
             */
            static bool RegisterTSCHandler(handler_t handler){
                if (handler == nullptr){
                    return false;
                }

                const uint32_t primask = __get_PRIMASK();
                __disable_irq();

                if ((tsc_handler != nullptr) && (tsc_handler != handler)){
                    __set_PRIMASK(primask);
                    return false;
                }

                tsc_handler = handler;

                constexpr uint8_t vector_mask = static_cast<uint8_t>(1U << 2);
                if ((initialized_vectors & vector_mask) == 0){
                    static EXTI_IRQ_Dispatcher dispatcher;
                    dispatcher.InitInterrupt();
                    initialized_vectors |= vector_mask;
                }

                __set_PRIMASK(primask);
                return true;
            }
        };

    } // namespace STM_EXTI
} // namespace STM_CppLib

#endif /*   EXTI_IRQ_DISPATCHER_HPP   */
