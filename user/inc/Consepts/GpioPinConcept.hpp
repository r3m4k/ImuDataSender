/** ****************************************************************************
 * @file    GpioPin.hpp
 * @author  Романовский Роман
 **************************************************************************** */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef GPIO_PIN_HPP
#define GPIO_PIN_HPP

/* Includes ------------------------------------------------------------------*/
#include <concepts>

/* Defines -------------------------------------------------------------------*/

/* Global variables ----------------------------------------------------------*/

// -----------------------------------------------------------------------------
namespace STM_CppLib{
    namespace STM_GPIO{

        /**
         * @brief   Концепт, описывающий требования к типу пина GPIO для использования в шаблонах.
         * @tparam  T   Тип, который проверяется на соответствие концепту.
         * @details Концепт требует наличия методов с сигнатурами,
         *          аналогичными методам класса GPIO_Pin из библиотеки STM_CppLib.
         * @note    Для проверки InitPin используются типы параметров GPIOMode_TypeDef, GPIOPuPd_TypeDef
         *          и указатель на GPIO_InitTypeDef; фактические значения не важны.
         */
        template <typename T>
        concept GpioPinConcept = requires(T pin) {
            { pin.get_port() }       -> std::same_as<GPIO_Port>;
            { pin.get_pin_source() } -> std::same_as<uint8_t>;
            { pin.SetPin() }   -> std::same_as<void>;
            { pin.ResetPin() } -> std::same_as<void>;
            { pin.ReadPin() }  -> std::same_as<BitAction>;
            { pin.InitPin(GPIOMode_TypeDef(), 
                          GPIOPuPd_TypeDef(), 
                          GPIOSpeed_TypeDef(),
                          GPIOOType_TypeDef()) } -> std::same_as<void>;
        };
    } // namespace STM_GPIO
} // namespace STM_CppLib

#endif /*   GPIO_PIN_HPP   */