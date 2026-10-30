/** ****************************************************************************
 * @file    UsartRx.hpp
 * @brief   Прикладной USART только для приёма байтов декодером в IRQ
 * @author  Романовский Роман
 * @date    Октябрь 2026
 * @details Не содержит TX-пина и TX-очереди. Объект существует всё время работы МК.
 **************************************************************************** */

#ifndef USART_RX_HPP
#define USART_RX_HPP

/* Includes ------------------------------------------------------------------*/
#include "UsartHardware.hpp"
#include "UsartIRQ.hpp"
#include "UsartRxChannel.hpp"

namespace STM_CppLib{
    namespace STM_Usart{

        /**
         * @brief Приёмник с собственным декодером, создаваемым без аргументов.
         * @tparam Type Аппаратный USART/UART.
         * @tparam RxPin Тип вывода приёма.
         * @tparam Decoder Тип с методом void byte_processing(uint8_t).
         * @note Декодер выполняется в IRQ, не должен блокировать обработку и отправлять в TX-очереди.
         */
        template<UsartTypes Type, STM_GPIO::GpioPinConcept RxPin, ByteDecoder Decoder>
        class UsartRx{
            friend class UsartIRQ<UsartRx, Type>;

            using rx_channel_t = UsartRxChannel<Type, RxPin, Decoder>;

            UsartHardware<Type> hardware; ///< Аппаратное состояние модуля
            rx_channel_t rx; ///< RX-пин, аппаратный приём и декодер
            UsartIRQ<UsartRx, Type> irq_registration; ///< Единственная регистрация устройства

            /** @brief Обслуживает данные и ошибки приёмника данного устройства. */
            void irq_handler(){
                if (hardware.is_started()){ rx.handle_interrupt(); }
            }

        public:
            /** @brief Создаёт устройство без настройки аппаратуры и регистрации IRQ. */
            UsartRx() = default;
            UsartRx(const UsartRx&) = delete;
            UsartRx& operator=(const UsartRx&) = delete;
            UsartRx(UsartRx&&) = delete;
            UsartRx& operator=(UsartRx&&) = delete;

            /**
             * @brief Настраивает или переинициализирует приёмник и сразу разрешает приём.
             * @param config Скорость, формат слова и режим выборки;
             *               значения по умолчанию — 115200, 8N1, Oversampling16.
             * @return false без изменения настроек при занятом ресурсе, активном RX
             *         или неподдерживаемой конфигурации.
             * @note Декодер сохраняется. Непрочитанное аппаратное слово при перенастройке сбрасывается.
             */
            bool Init(const UsartConfig& config){
                if (!hardware.validate_baudrate(config.baudrate, config.oversampling)){ return false; }
                if (!irq_registration.register_device(*this)){ return false; }
                if (!Stop()){ return false; }
                UsartConfig selected_config = config;
                selected_config.direction = rx_channel_t::DIRECTION;
                hardware.configure(selected_config);
                rx.init();
                return Start();
            }

            /** @brief Возобновляет приём после Stop(); возвращает false до успешного Init(). */
            bool Start(){
                if (hardware.is_started()){ return true; }
                if (!hardware.start()){ return false; }
                rx.enable_interrupts();
                irq_registration.enable();
                return true;
            }

            /** @brief Останавливает приём; false при принимаемом слове. Состояние декодера сохраняется. */
            bool Stop(){
                const uint32_t primask = __get_PRIMASK();
                __disable_irq();
                if (hardware.is_started()){
                    if (rx.is_busy()){
                        __set_PRIMASK(primask);
                        return false;
                    }
                    rx.disable_interrupts();
                    hardware.stop();
                }
                __set_PRIMASK(primask);
                return true;
            }

            /** @brief Проверяет приём отдельного слова, а не пакета и не наличие байта в RDR. */
            bool IsRxBusy() const { return hardware.is_started() && rx.is_busy(); }

            /** @brief Меняет приоритет всего вектора IRQ после Init(), не меняя его разрешение. */
            void SetInterruptPriority(uint8_t preemption_priority, uint8_t subpriority = 0){
                irq_registration.set_priority(preemption_priority, subpriority);
            }
        };

    } // namespace STM_Usart
} // namespace STM_CppLib

#endif /*   USART_RX_HPP   */
