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

        template<typename Device, UsartTypes Type> class UsartIRQ;
        template<IRQn_Type IRQn> class UsartIRQVector;

        /** @brief Фиксированное общее хранилище регистраций аппаратных ресурсов. */
        class UsartIRQDispatcher{
            template<typename Device, UsartTypes Type> friend class UsartIRQ;
            template<IRQn_Type IRQn> friend class UsartIRQVector;

            /** @brief Пара адреса владельца и статического переходника его типа. */
            struct Registration{
                void* owner; ///< Стабильный адрес прикладного устройства
                void (*handler)(void*); ///< Статический переходник без виртуальных вызовов
            };

            static constexpr uint8_t RESOURCE_COUNT = 5; ///< Число USART/UART STM32F303
            inline static Registration registrations[RESOURCE_COUNT] = {}; ///< Общая таблица владельцев
            inline static bool initialized_vectors[RESOURCE_COUNT] = {}; ///< Первичная настройка векторов
            inline static constexpr IRQn_Type vectors[RESOURCE_COUNT] = {
                UsartDescriptor<UsartTypes::Usart1>::IRQn,
                UsartDescriptor<UsartTypes::Usart2>::IRQn,
                UsartDescriptor<UsartTypes::Usart3>::IRQn,
                UsartDescriptor<UsartTypes::Uart4>::IRQn,
                UsartDescriptor<UsartTypes::Uart5>::IRQn
            }; ///< Аппаратные векторы в порядке индексов дескрипторов

            /** @brief Возвращает единый индекс состояния для всех источников одного вектора. */
            static uint8_t vector_index(IRQn_Type irqn){
                for (uint8_t i = 0; i < RESOURCE_COUNT; i++){
                    if (vectors[i] == irqn){ return i; }
                }
                return RESOURCE_COUNT;
            }

            /** @brief Вызывает всех владельцев данного IRQ; каждый проверяет свои источники. */
            static void dispatch_irq(IRQn_Type irqn){
                for (uint8_t i = 0; i < RESOURCE_COUNT; i++){
                    if (vectors[i] == irqn && registrations[i].handler != nullptr){
                        registrations[i].handler(registrations[i].owner);
                    }
                }
            }
        };

    } // namespace STM_Usart
} // namespace STM_CppLib

#endif /*   USART_IRQ_DISPATCHER_HPP   */
