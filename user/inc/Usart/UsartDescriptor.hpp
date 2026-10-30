/** ****************************************************************************
 * @file    UsartDescriptor.hpp
 * @brief   Статические аппаратные сведения USART/UART STM32F303
 * @author  Романовский Роман
 * @date    Октябрь 2026
 * @details Не настраивает периферию и не хранит изменяемое состояние владения.
 **************************************************************************** */

#ifndef USART_DESCRIPTOR_HPP
#define USART_DESCRIPTOR_HPP

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include "stm32f30x.h"
#include "stm32f30x_gpio.h"
#include "stm32f30x_rcc.h"

namespace STM_CppLib{
    namespace STM_Usart{

        /** 
         * @brief Идентификаторы аппаратных модулей под конкретный МК. 
         */
        enum class UsartTypes: uint8_t { Usart1, Usart2, Usart3, Uart4, Uart5 };

        /**
         * @brief   Статическая привязка одного аппаратного модуля.
         * @tparam  Type Идентификатор USART/UART.
         * @note    AF применим к выбранным приложением выводам USART с AF7/UART с AF5.
         *          Допустимость конкретных выводов проверяется по datasheet МК.
         */
        template<UsartTypes Type>
        class UsartDescriptor{
            static_assert(
                Type == UsartTypes::Usart1 ||
                Type == UsartTypes::Usart2 ||
                Type == UsartTypes::Usart3 ||
                Type == UsartTypes::Uart4  ||
                Type == UsartTypes::Uart5,
                "Unsupported USART type"
            );

        public:
            /**
             * @brief Индекс аппаратного USART/UART в общей таблице владельцев.
             */
            static constexpr uint8_t UART_INDEX = []() -> uint8_t{
                if constexpr (Type == UsartTypes::Usart1)     { return 0; }
                else if constexpr (Type == UsartTypes::Usart2){ return 1; }
                else if constexpr (Type == UsartTypes::Usart3){ return 2; }
                else if constexpr (Type == UsartTypes::Uart4) { return 3; }
                else if constexpr (Type == UsartTypes::Uart5) { return 4; }
            }();

            /**
             * @brief Номер аппаратного вектора прерывания USART/UART.
             */
            static constexpr IRQn_Type IRQn = [](){
                if constexpr (Type == UsartTypes::Usart1)     { return USART1_IRQn; }
                else if constexpr (Type == UsartTypes::Usart2){ return USART2_IRQn; }
                else if constexpr (Type == UsartTypes::Usart3){ return USART3_IRQn; }
                else if constexpr (Type == UsartTypes::Uart4) { return UART4_IRQn;  }
                else if constexpr (Type == UsartTypes::Uart5) { return UART5_IRQn;  }
            }();

            /**
             * @brief Маска включения тактирования аппаратного модуля.
             */
            static constexpr uint32_t RCC_PERIPH = [](){
                if constexpr (Type == UsartTypes::Usart1)     { return RCC_APB2Periph_USART1; }
                else if constexpr (Type == UsartTypes::Usart2){ return RCC_APB1Periph_USART2; }
                else if constexpr (Type == UsartTypes::Usart3){ return RCC_APB1Periph_USART3; }
                else if constexpr (Type == UsartTypes::Uart4) { return RCC_APB1Periph_UART4;  }
                else if constexpr (Type == UsartTypes::Uart5) { return RCC_APB1Periph_UART5;  }
            }();

            /**
             * @brief Указатель на функцию управления тактированием соответствующей шины.
             */
            static constexpr auto CLOCK_CMD = [](){
                if constexpr (Type == UsartTypes::Usart1)     { return &RCC_APB2PeriphClockCmd; }
                else if constexpr (Type == UsartTypes::Usart2){ return &RCC_APB1PeriphClockCmd; }
                else if constexpr (Type == UsartTypes::Usart3){ return &RCC_APB1PeriphClockCmd; }
                else if constexpr (Type == UsartTypes::Uart4) { return &RCC_APB1PeriphClockCmd; }
                else if constexpr (Type == UsartTypes::Uart5) { return &RCC_APB1PeriphClockCmd; }
            }();

            /**
             * @brief Указатель на поле входной частоты USART/UART в RCC_ClocksTypeDef.
             */
            static constexpr auto CLOCK_FREQUENCY = [](){
                if constexpr (Type == UsartTypes::Usart1)     { return &RCC_ClocksTypeDef::USART1CLK_Frequency; }
                else if constexpr (Type == UsartTypes::Usart2){ return &RCC_ClocksTypeDef::USART2CLK_Frequency; }
                else if constexpr (Type == UsartTypes::Usart3){ return &RCC_ClocksTypeDef::USART3CLK_Frequency; }
                else if constexpr (Type == UsartTypes::Uart4) { return &RCC_ClocksTypeDef::UART4CLK_Frequency;  }
                else if constexpr (Type == UsartTypes::Uart5) { return &RCC_ClocksTypeDef::UART5CLK_Frequency;  }
            }();

            /**
             * @brief Номер альтернативной функции GPIO для выводов USART/UART.
             */
            static constexpr uint8_t AF = []() -> uint8_t{
                if constexpr (Type == UsartTypes::Usart1)     { return GPIO_AF_7; }
                else if constexpr (Type == UsartTypes::Usart2){ return GPIO_AF_7; }
                else if constexpr (Type == UsartTypes::Usart3){ return GPIO_AF_7; }
                else if constexpr (Type == UsartTypes::Uart4) { return GPIO_AF_5; }
                else if constexpr (Type == UsartTypes::Uart5) { return GPIO_AF_5; }
            }();

            /** @brief Возвращает адрес регистров без обязательного константного вычисления. */
            static USART_TypeDef* get_usart(){
                if constexpr (Type == UsartTypes::Usart1)     { return USART1; }
                else if constexpr (Type == UsartTypes::Usart2){ return USART2; }
                else if constexpr (Type == UsartTypes::Usart3){ return USART3; }
                else if constexpr (Type == UsartTypes::Uart4) { return UART4;  }
                else if constexpr (Type == UsartTypes::Uart5) { return UART5;  }
            }
        };

    } // namespace STM_Usart
} // namespace STM_CppLib

#endif /*   USART_DESCRIPTOR_HPP   */
