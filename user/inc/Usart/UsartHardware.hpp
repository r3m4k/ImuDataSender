/** ****************************************************************************
 * @file    UsartHardware.hpp
 * @brief   Общая настройка и управление USART STM32F303 через SPL
 * @author  Романовский Роман
 * @date    Октябрь 2026
 * @details Внутренний компонент общей настройки USART. GPIO и работа направлений
 *          находятся в каналах. Не владеет очередью, декодером и регистрацией IRQ.
 **************************************************************************** */

#ifndef USART_HARDWARE_HPP
#define USART_HARDWARE_HPP

/* Includes ------------------------------------------------------------------*/
#include "stm32f30x_usart.h"
#include "UsartConfig.hpp"
#include "UsartDescriptor.hpp"

namespace STM_CppLib{
    namespace STM_Usart{

        /** 
         * @brief   Общая настройка и управление одним USART
         * @note    Используется композицией в устройстве UsartRx|Tx|RxTx. */
        template<UsartTypes Type>
        class UsartHardware{
            using descriptor_t = UsartDescriptor<Type>;     ///< Тип для используемого идентификатора

            USART_TypeDef* const usart = descriptor_t::get_usart(); ///< Указатель на регистры периферии

            bool configured = false;            ///< Флаг выполненности конфигурации
            volatile bool started = false;      ///< Volatile флаг активности Usart 

        public:
            /**
             * @brief  Проверка диапазона делителя скорости при текущей настройке тактировании
             *         и выбранном oversampling.
             * @param  baudrate      Одно из объявленных значений UsartBaudRate.
             * @param  oversampling  Режим выборки, по умолчанию Oversampling16.
             * @return true, если округлённый делитель до кодирования BRR находится в диапазоне 16...65535.
             */
            static bool validate_baudrate(
                UsartBaudRate baudrate,
                UsartOversampling oversampling = UsartOversampling::Oversampling16
            ){
                const uint32_t baudrate_value = static_cast<uint32_t>(baudrate);
                RCC_ClocksTypeDef clocks;
                RCC_GetClocksFreq(&clocks);

                const uint32_t uart_freq = clocks.*descriptor_t::CLOCK_FREQUENCY;
                uint32_t scaled_freq = 0;
                switch (oversampling){
                    case UsartOversampling::Oversampling16:
                        scaled_freq = uart_freq;
                        break;

                    case UsartOversampling::Oversampling8:
                        scaled_freq = 2U * uart_freq;
                        break;
                }
                // Та же схема округления, что в USART_Init() используемой SPL.
                const uint32_t divider = scaled_freq / baudrate_value +
                    ((scaled_freq % baudrate_value >= baudrate_value / 2U) ? 1U : 0U);
                return divider >= 16U && divider <= 0xFFFFU;
            }

            /**
             * @brief Конфигурация Usart без проверки остановлен ли модуль.
             * @param config Конфигурация Usart.
             * @warning Перед вызовом необходимо заранее остановить Usart!
             */
            void configure(const UsartConfig& config){
                descriptor_t::CLOCK_CMD(descriptor_t::RCC_PERIPH, ENABLE);
                // Сброс настроек и дополнительных параметров USART.
                usart->CR1 = 0;
                usart->CR2 = 0;
                usart->CR3 = 0;
                // SPL рассчитывает и кодирует BRR по OVER8, установленному до USART_Init().
                switch (config.oversampling){
                    case UsartOversampling::Oversampling16:
                        USART_OverSampling8Cmd(usart, DISABLE);
                        break;

                    case UsartOversampling::Oversampling8:
                        USART_OverSampling8Cmd(usart, ENABLE);
                        break;
                }

                USART_InitTypeDef hardware_config = static_cast<USART_InitTypeDef>(config);
                USART_Init(usart, &hardware_config);
                configured = true;
            }

            /** 
             * @brief Запуск ранее сконфигурированного Usart
             * @note  Источники событий настраиваются соответствующими Tx/Rx_channel.
             */
            bool start(){
                if (!configured){ return false; }
                if (started){ return true; }
                USART_Cmd(usart, ENABLE);
                started = true;
                return true;
            }

            /**
             * @brief Выключение USART после остановки его каналов прикладным классом.
             * @note  Вызывается в короткой критической секции после проверки активности
             *        и отключения источников IRQ каналов. Сам метод не проверяет TX/RX.
             */
            void stop(){
                if (!started){ return; }
                USART_Cmd(usart, DISABLE);
                started = false;
            }

            /** 
             * @brief Возврат состояние запуска. 
             */
            bool is_started() const { return started; }
        };

    } // namespace STM_Usart
} // namespace STM_CppLib

#endif /*   USART_HARDWARE_HPP   */
