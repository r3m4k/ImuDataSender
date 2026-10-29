/** ****************************************************************************
 * @file    ExtiPin.hpp
 * @author  Романовский Роман
 * @brief   Шаблонный класс для работы с внешними прерываниями (EXTI)
 * @details Предоставляет типобезопасные обёртку над GPIO с поддержкой внешних
 *          прерываний через EXTI.
 **************************************************************************** */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef EXTI_PIN_HPP
#define EXTI_PIN_HPP

/* Includes ------------------------------------------------------------------*/
#include <concepts>

#include "stm32f30x.h"
#include "stm32f30x_rcc.h"
#include "stm32f30x_gpio.h"

#include "Consts.hpp"
#include "Exti.hpp"
#include "GpioPin.hpp"
#include "ExtiIRQ.hpp"

/* Defines -------------------------------------------------------------------*/

/* Global variables ----------------------------------------------------------*/

// -----------------------------------------------------------------------------
namespace STM_CppLib{
    namespace STM_GPIO{

        /**
         * @brief   Класс для работы с выводом GPIO, поддерживающим внешние прерывания (EXTI).
         * @tparam  port                   Порт GPIO.
         * @tparam  pin_source             Номер вывода.
         * @tparam  external_irq_handler   Функция-обработчик прерывания (тип handler_t), вызываемая при срабатывании EXTI.
         * @details Наследует функциональность GPIO_Pin и настройки EXTI. Содержит объект
         *          регистрации GPIO_EXTI_n для совместной работы линий с общим IRQ.
         *          Позволяет одной командой инициализировать пин, настроить EXTI и NVIC.
         * @note    Для корректной работы необходимо, чтобы EXTI_Descriptor<port, pin_source> предоставлял
         *          корректные параметры источника порта и номер вектора прерывания.
         * @note    Объект должен существовать всё время работы IRQ. Копирование и перемещение запрещены.
         *          Очистка флага EXTI выполняется диспетчером перед вызовом пользовательского обработчика.
         */
        template <GPIO_Port port, uint8_t pin_source, handler_t external_irq_handler>
        class GPIO_Pin_EXTI: 
            public GPIO_Pin<port, pin_source>,

            public STM_EXTI::GPIO_EXTI<STM_EXTI::EXTI_Descriptor<port, pin_source>::PortSource, pin_source>
        {
            static_assert(pin_source < 16, "EXTI line must be from 0 to 15!");
            static_assert(external_irq_handler != nullptr, "EXTI handler can not be nullptr");

            // Приватный объект для регистрации прерывания
            STM_EXTI::GPIO_EXTI_n<GPIO_Pin_EXTI> irq_registration;

        public:
            /**
             * @brief   Создаёт объект пина и его регистрации без настройки аппаратуры.
             */
            GPIO_Pin_EXTI() = default;

            /**
             * @brief   Полная инициализация пина с поддержкой EXTI.
             * @param   GPIO_Mode                 Режим работы вывода (по умолчанию GPIO_Mode_IN).
             * @param   GPIO_PuPd                 Тип подтяжки (по умолчанию GPIO_PuPd_NOPULL).
             * @param   GPIO_Speed                Скорость работы вывода.
             * @param   GPIO_OType                Тип выхода.
             * @param   EXTI_InitStructure_ptr    Указатель на структуру инициализации EXTI (или nullptr).
             * @param   NVIC_InitStructure_ptr    Указатель на структуру инициализации NVIC (или nullptr).
             * @return  false при конфликте регистрации или несовпадении линии/IRQ в настройках.
             * @details Регистрирует обработчик, настраивает GPIO, включает тактирование SYSCFG
             *          и настраивает EXTI. При конфликте настройки аппаратуры не изменяются.
             * @note    Вызывать из основного кода после создания объекта. Прежнее состояние
             *          маски прерываний восстанавливается после завершения настройки.
             */
            bool InitPinExti(
                GPIOMode_TypeDef GPIO_Mode = GPIO_Mode_IN,
                GPIOPuPd_TypeDef GPIO_PuPd = GPIO_PuPd_NOPULL,
                GPIOSpeed_TypeDef GPIO_Speed = GPIO_Speed_50MHz,
                GPIOOType_TypeDef GPIO_OType = GPIO_OType_PP,
                EXTI_InitTypeDef* EXTI_InitStructure_ptr = nullptr,
                NVIC_InitTypeDef* NVIC_InitStructure_ptr = nullptr
            ){
                if ((EXTI_InitStructure_ptr != nullptr) &&
                    (EXTI_InitStructure_ptr->EXTI_Line != static_cast<uint32_t>(pin_source))){
                    return false;
                }

                const uint32_t primask = __get_PRIMASK();
                __disable_irq();

                if (!irq_registration.InitInterrupt(NVIC_InitStructure_ptr)){
                    __set_PRIMASK(primask);
                    return false;
                }

                // Инициализация пина
                this->InitPin(GPIO_Mode, GPIO_PuPd, GPIO_Speed, GPIO_OType);
                // Активация тактирования блока SYSCFG, необходимый для работы EXTI 
                RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);
                // Сбросим возможно установленный бит прерывания EXTI_Line 
                EXTI_ClearITPendingBit(static_cast<uint32_t>(pin_source));
                // Инициализация EXTI
                this->InitExti(EXTI_InitStructure_ptr);

                __set_PRIMASK(primask);
                return true;
            }

            /**
             * @brief   Изменяет приоритет вектора IRQ данного пина.
             * @param   preemption_priority Приоритет вытеснения.
             * @param   sub_priority Подприоритет в текущей группировке NVIC.
             * @note    Вызывать после InitPinExti(). Приоритет общий для всех линий этого вектора.
             */
            void SetInterruptPriority(uint8_t preemption_priority, uint8_t sub_priority){
                irq_registration.SetInterruptPriority(preemption_priority, sub_priority);
            }
            
            /**
             * @brief   Статический переходник к пользовательскому обработчику EXTI.
             * @note    Вызывается диспетчером после очистки pending-флага линии.
             */
            static void irq_handler(){
                external_irq_handler();
            }
        };
    
    } // namespace STM_GPIO
} // namespace STM_CppLib

#endif /*   EXTI_PIN_HPP   */