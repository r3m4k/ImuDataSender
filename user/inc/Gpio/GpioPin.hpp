/** ****************************************************************************
 * @file    GpioPin.hpp
 * @author  Романовский Роман
 * @brief   Шаблонный класс для работы с выводами GPIO
 * @details Предоставляет класс GPIO_Pin для инициализации, установки, сброса
 *          и чтения вывода. Подключает концепт GpioPinConcept из отдельного заголовка.
 *          Для работы с внешними прерываниями подключите ExtiPin.hpp.
 **************************************************************************** */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef GPIO_PIN_HPP
#define GPIO_PIN_HPP

/* Includes ------------------------------------------------------------------*/
#include "stm32f30x.h"
#include "stm32f30x_rcc.h"
#include "stm32f30x_gpio.h"

#include "Consts.hpp"
#include "GpioPort.hpp"
#include "../Concepts/GpioPinConcept.hpp"

/* Defines -------------------------------------------------------------------*/

/* Global variables ----------------------------------------------------------*/

// -----------------------------------------------------------------------------
namespace STM_CppLib{
    namespace STM_GPIO{

        /**
         * @brief   Шаблонный класс для управления конкретным выводом GPIO.
         * @tparam  port        Порт GPIO (значение перечисления GPIO_Port).
         * @tparam  pin_source  Номер вывода (0..15).
         * @details Позволяет инициализировать вывод, устанавливать/сбрасывать его состояние
         *          и читать логический уровень. Все операции выполняются через соответствующие
         *          функции SPL. Тип порта и номер вывода фиксируются на этапе компиляции,
         *          что обеспечивает типобезопасность и минимальные накладные расходы.
         */
        template <GPIO_Port port, uint8_t pin_source>
        class GPIO_Pin{

        public:

            /**
             * @brief   Возвращает значение порта GPIO, заданное шаблонным параметром.
             * @return  GPIO_Port   Значение перечисления порта (PortA..PortF).
             */
            static constexpr GPIO_Port get_port() { return port; }

            /**
             * @brief   Возвращает номер вывода, заданный шаблонным параметром.
             * @return  uint8_t     Номер вывода (0..15).
             */
            static constexpr uint8_t get_pin_source() { return pin_source; }

            /**
             * @brief   Инициализирует вывод с заданными параметрами.
             * @param   GPIO_Mode   Режим работы вывода (вход, выход, альтернативная функция и т.д.).
             * @param   GPIO_PuPd   Тип подтяжки (к питанию, к земле, без подтяжки).
             * @param   GPIO_Speed  Скорость работы вывода (для выходных режимов).
             * @param   GPIO_OType  Тип выхода (Push-Pull или Open-Drain).
             * @note    Перед настройкой включает тактирование соответствующего порта через RCC.
             *          Структура GPIO_InitTypeDef создаётся и заполняется автоматически,
             *          после чего вызывается GPIO_Init.
             */
            void InitPin(
                GPIOMode_TypeDef GPIO_Mode = GPIO_Mode_OUT,
                GPIOPuPd_TypeDef GPIO_PuPd = GPIO_PuPd_NOPULL,
                GPIOSpeed_TypeDef GPIO_Speed = GPIO_Speed_50MHz,
                GPIOOType_TypeDef GPIO_OType = GPIO_OType_PP
            ){
                
                RCC_AHBPeriphClockCmd(GPIO_PortDescriptor<port>::RCC_Periph, ENABLE);
                
                GPIO_InitTypeDef GPIO_InitStructure;
        
                /* Configure the GPIOx pin */
                GPIO_InitStructure.GPIO_Pin = (1U << pin_source);
                GPIO_InitStructure.GPIO_Mode = GPIO_Mode;
                GPIO_InitStructure.GPIO_OType = GPIO_OType;
                GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd;
                GPIO_InitStructure.GPIO_Speed = GPIO_Speed;
                GPIO_Init(GPIO_PortDescriptor<port>::get_GPIO_Type(), &GPIO_InitStructure);
            }

            /**
             * @brief   Устанавливает высокий уровень на выводе.
             */
            void SetPin(){
                GPIO_SetBits(GPIO_PortDescriptor<port>::get_GPIO_Type(), (1U << pin_source));
            }

            /**
             * @brief   Устанавливает низкий уровень на выводе.
             */
            void ResetPin(){
                GPIO_ResetBits(GPIO_PortDescriptor<port>::get_GPIO_Type(), (1U << pin_source));
            }

            /**
             * @brief   Читает текущее состояние вывода.
             * @return  BitAction: Bit_SET, если на выводе высокий уровень, иначе Bit_RESET.
             */
            BitAction ReadPin(){
                return (GPIO_ReadInputDataBit(GPIO_PortDescriptor<port>::get_GPIO_Type(),(1U << pin_source)) == Bit_SET) ? Bit_SET : Bit_RESET;
            }

        };

    } // namespace STM_GPIO
} // namespace STM_CppLib

#endif /*   GPIO_PIN_HPP   */