/** ****************************************************************************
 * @file    GpioPin.hpp
 * @author  Романовский Роман
 * @brief   Шаблонные классы для работы с выводами GPIO и внешними прерываниями (EXTI)
 * @details Предоставляет типобезопасные обёртки над GPIO: базовый класс GPIO_Pin для
 *          управления выводом (инициализация, установка/сброс, чтение) и расширенный
 *          класс GPIO_Pin_EXTI с поддержкой внешних прерываний через EXTI.
 *          Также содержит концепт GpioPinConcept для проверки наличия необходимых методов.
 **************************************************************************** */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef GPIO_PIN_HPP
#define GPIO_PIN_HPP

/* Includes ------------------------------------------------------------------*/
#include <concepts>

#include "stm32f30x.h"
#include "stm32f30x_rcc.h"
#include "stm32f30x_gpio.h"

#include "Consts.hpp"
#include "Exti.hpp"
#include "GpioPort.hpp"
#include "ExtiIRQ.hpp"

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
                __enable_irq();
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

#endif /*   GPIO_PIN_HPP   */