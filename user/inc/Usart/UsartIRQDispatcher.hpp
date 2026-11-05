/** ****************************************************************************
 * @file    UsartIRQDispatcher.hpp
 * @brief   Общая таблица владельцев и распределение аппаратных IRQ USART
 * @author  Романовский Роман
 * @date    Октябрь 2026
 * @details Таблица общая для всех типов устройств, пинов, декодеров и размеров
 *          очередей. Регистрация сохраняется всё время работы микроконтроллера.
 **************************************************************************** */

#ifndef USART_IRQ_DISPATCHER_HPP
#define USART_IRQ_DISPATCHER_HPP

/* Includes ------------------------------------------------------------------*/
#include "UsartDescriptor.hpp"

namespace STM_CppLib{
    namespace STM_Usart{

        template<typename UsartDevice, UsartTypes Type> class UsartIRQ;
        template<IRQn_Type IRQn> class UsartIRQVector;

        /** 
         * @brief Фиксированное общее хранилище регистраций аппаратных ресурсов. 
         */
        class UsartIRQDispatcher{
            template<typename UsartDevice, UsartTypes Type> friend class UsartIRQ;
            template<IRQn_Type IRQn> friend class UsartIRQVector;

            /** @brief Пара адреса владельца и статического переходника его типа. */
            // [Question] Нужно пояснение для чего эта структура. 
            // И скорее всего её надо переименовать вместе с её полями.
            struct UsartIRQEntry{
                void* usart_ptr; ///< Стабильный адрес прикладного устройства
                void (*handler)(void*); ///< Статический переходник без виртуальных вызовов
            };

            static constexpr uint8_t USART_NUM = 5; ///< Число USART/UART на МК
            inline static UsartIRQEntry registrations[USART_NUM] = {}; ///< Общая таблица владельцев
            inline static bool initialized_vectors[USART_NUM] = {}; ///< Первичная настройка векторов

            /**
             * @brief Массив номеров прерываний для всех Usart.
             */
            inline static constexpr IRQn_Type usart_irq_vector[USART_NUM] = {
                UsartDescriptor<UsartTypes::Usart1>::IRQn,
                UsartDescriptor<UsartTypes::Usart2>::IRQn,
                UsartDescriptor<UsartTypes::Usart3>::IRQn,
                UsartDescriptor<UsartTypes::Uart4>::IRQn,
                UsartDescriptor<UsartTypes::Uart5>::IRQn
            }; ///< Аппаратные векторы в порядке индексов дескрипторов

            /** 
             * @brief Получение индекса прерывания в массиве векторов прерывания Usart. 
             */
            static uint8_t usart_irq_vector_index(IRQn_Type irqn){
                uint8_t index;
                for (index = 0; index < USART_NUM; index++){
                    if (usart_irq_vector[index] == irqn) break;
                }
                return index;   // А если не нашёлся нужный вектор?
            }

            /** @brief Вызывает всех владельцев данного IRQ; каждый проверяет свои источники. */
            static void dispatch_irq(IRQn_Type irqn){
                for (uint8_t i = 0; i < USART_NUM; i++){
                    if (usart_irq_vector[i] == irqn && registrations[i].handler != nullptr){
                        registrations[i].handler(registrations[i].usart_ptr);
                    }
                }
            }
        };

    } // namespace STM_Usart
} // namespace STM_CppLib

#endif /*   USART_IRQ_DISPATCHER_HPP   */
