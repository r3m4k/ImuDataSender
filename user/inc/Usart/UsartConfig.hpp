/** ****************************************************************************
 * @file    UsartConfig.hpp
 * @brief   Прикладная конфигурация байтового USART
 * @author  Романовский Роман
 * @date    Октябрь 2026
 * @details Формат содержит восемь полезных бит. Поля используют перечисления
 *          библиотеки; явный оператор преобразования формирует структуру SPL.
 *          Init() задаёт направление в копии конфигурации из свойств каналов.
 **************************************************************************** */

#ifndef USART_CONFIG_HPP
#define USART_CONFIG_HPP

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include "stm32f30x_usart.h"

namespace STM_CppLib{
    namespace STM_Usart{

        /** 
         * @brief Именованные скорости обмена в бит/с. 
         * */
        enum class UsartBaudRate: uint32_t{
            Baudrate_9600   = 9600,
            Baudrate_19200  = 19200,
            Baudrate_38400  = 38400,
            Baudrate_57600  = 57600,
            Baudrate_115200 = 115200,
            Baudrate_230400 = 230400,
            Baudrate_460800 = 460800,
            Baudrate_921600 = 921600
        };

        /** 
         * @brief Проверка чётности передаваемого слова. 
         */
        enum class UsartParity: uint8_t { None, Even, Odd };

        /** 
         * @brief Число стоп-битов асинхронного слова. 
         */
        enum class UsartStopBits: uint8_t { One, Two };

        /**
         * @brief Число тактов выборки приёмника на один бит данных.
         */
        enum class UsartOversampling: uint8_t{
            Oversampling16,
            Oversampling8
        };

        /** 
         * @brief Структура для конфигурации Usart с полями по умолчанию.
         * @note Поля должны содержать только объявленные значения соответствующих перечислений.
         */
        struct UsartConfig{
            UsartBaudRate baudrate = UsartBaudRate::Baudrate_115200;    ///< Скорость обмена
            UsartParity parity = UsartParity::None;                     ///< Проверка чётности
            UsartStopBits stop_bits = UsartStopBits::One;               ///< Число стоп-битов
            UsartOversampling oversampling = UsartOversampling::Oversampling16; ///< Режим выборки приёмника
            uint32_t direction = USART_Mode_Tx | USART_Mode_Rx; ///< Служебная маска; Init() задаёт её из каналов

            /**
             * @brief Формирует конфигурацию SPL без обращения к аппаратуре.
             * @return Структура настройки USART с восемью полезными битами данных.
             * @note Направление берётся из поля direction.
             * @note Oversampling задаётся аппаратным компонентом отдельно:
             *       USART_InitTypeDef не содержит поля для выбора этого режима.
             */
            explicit operator USART_InitTypeDef() const {
                USART_InitTypeDef result{};
                result.USART_BaudRate = static_cast<uint32_t>(baudrate);
                result.USART_Mode = direction;
                result.USART_HardwareFlowControl = USART_HardwareFlowControl_None;

                switch (parity){
                    case UsartParity::None:
                        result.USART_WordLength = USART_WordLength_8b;
                        result.USART_Parity = USART_Parity_No;
                        break;

                    case UsartParity::Even:
                        result.USART_WordLength = USART_WordLength_9b;
                        result.USART_Parity = USART_Parity_Even;
                        break;

                    case UsartParity::Odd:
                        result.USART_WordLength = USART_WordLength_9b;
                        result.USART_Parity = USART_Parity_Odd;
                        break;
                }

                switch (stop_bits){
                    case UsartStopBits::One:
                        result.USART_StopBits = USART_StopBits_1;
                        break;

                    case UsartStopBits::Two:
                        result.USART_StopBits = USART_StopBits_2;
                        break;
                }

                return result;
            }
        };

    } // namespace STM_Usart
} // namespace STM_CppLib

#endif /*   USART_CONFIG_HPP   */
