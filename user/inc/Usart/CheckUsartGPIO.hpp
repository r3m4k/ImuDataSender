/** ****************************************************************************
 * @file    CheckUsartGPIO.hpp
 * @brief   Проверка допустимых TX/RX-выводов USART при компиляции
 * @author  Романовский Роман
 * @date    Ноябрь 2026
 *
 * @details Таблицы для STM32F303VCT6 в корпусе LQFP100 на STM32F3DISCOVERY.
 *          Источник: DS9118 Rev 14, таблицы 13–18:
 *          https://www.st.com/resource/en/datasheet/stm32f303vc.pdf
 *          Используются штатные направления без перестановки TX/RX (SWAP).
 *          USART1–3 требуют AF7, UART4–5 — AF5. При переносе таблицы выводов
 *          и настройка AF в UsartDescriptor и каналах обновляются согласованно.
 *          Занятость выводов компонентами платы и другой периферией не проверяется.
 **************************************************************************** */

#ifndef CHECK_USART_GPIO_HPP
#define CHECK_USART_GPIO_HPP

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include "GpioPort.hpp"
#include "UsartDescriptor.hpp"

namespace STM_CppLib{
    namespace STM_Usart{

        /**
         * @brief Проверяет возможность приёма USART на выбранном выводе.
         * @param usart_type Аппаратный USART/UART.
         * @param port Порт GPIO.
         * @param pin_source Номер вывода в порту.
         * @return true для допустимого RX-вывода STM32F303VCT6; иначе false.
         */
        consteval bool is_usart_rx_gpio(
            UsartTypes usart_type,
            STM_GPIO::GPIO_Port port,
            uint8_t pin_source
        ){
            switch (usart_type){
                case UsartTypes::Usart1:
                    return
                        (port == STM_GPIO::GPIO_Port::PortA && pin_source == 10) ||
                        (port == STM_GPIO::GPIO_Port::PortB && pin_source == 7) ||
                        (port == STM_GPIO::GPIO_Port::PortC && pin_source == 5) ||
                        (port == STM_GPIO::GPIO_Port::PortE && pin_source == 1);

                case UsartTypes::Usart2:
                    return
                        (port == STM_GPIO::GPIO_Port::PortA && pin_source == 3) ||
                        (port == STM_GPIO::GPIO_Port::PortA && pin_source == 15) ||
                        (port == STM_GPIO::GPIO_Port::PortB && pin_source == 4) ||
                        (port == STM_GPIO::GPIO_Port::PortD && pin_source == 6);

                case UsartTypes::Usart3:
                    return
                        (port == STM_GPIO::GPIO_Port::PortB && pin_source == 11) ||
                        (port == STM_GPIO::GPIO_Port::PortC && pin_source == 11) ||
                        (port == STM_GPIO::GPIO_Port::PortD && pin_source == 9) ||
                        (port == STM_GPIO::GPIO_Port::PortE && pin_source == 15);

                case UsartTypes::Uart4:
                    return port == STM_GPIO::GPIO_Port::PortC && pin_source == 11;

                case UsartTypes::Uart5:
                    return port == STM_GPIO::GPIO_Port::PortD && pin_source == 2;
            }
            return false;
        }

        /**
         * @brief Проверяет возможность передачи USART на выбранном выводе.
         * @param usart_type Аппаратный USART/UART.
         * @param port Порт GPIO.
         * @param pin_source Номер вывода в порту.
         * @return true для допустимого TX-вывода STM32F303VCT6; иначе false.
         */
        consteval bool is_usart_tx_gpio(
            UsartTypes usart_type,
            STM_GPIO::GPIO_Port port,
            uint8_t pin_source
        ){
            switch (usart_type){
                case UsartTypes::Usart1:
                    return
                        (port == STM_GPIO::GPIO_Port::PortA && pin_source == 9) ||
                        (port == STM_GPIO::GPIO_Port::PortB && pin_source == 6) ||
                        (port == STM_GPIO::GPIO_Port::PortC && pin_source == 4) ||
                        (port == STM_GPIO::GPIO_Port::PortE && pin_source == 0);

                case UsartTypes::Usart2:
                    return
                        (port == STM_GPIO::GPIO_Port::PortA && pin_source == 2) ||
                        (port == STM_GPIO::GPIO_Port::PortA && pin_source == 14) ||
                        (port == STM_GPIO::GPIO_Port::PortB && pin_source == 3) ||
                        (port == STM_GPIO::GPIO_Port::PortD && pin_source == 5);

                case UsartTypes::Usart3:
                    return
                        (port == STM_GPIO::GPIO_Port::PortB && pin_source == 10) ||
                        (port == STM_GPIO::GPIO_Port::PortC && pin_source == 10) ||
                        (port == STM_GPIO::GPIO_Port::PortD && pin_source == 8);

                case UsartTypes::Uart4:
                    return port == STM_GPIO::GPIO_Port::PortC && pin_source == 10;

                case UsartTypes::Uart5:
                    return port == STM_GPIO::GPIO_Port::PortC && pin_source == 12;
            }
            return false;
        }

    } // namespace STM_Usart
} // namespace STM_CppLib

#endif /*   CHECK_USART_GPIO_HPP   */
