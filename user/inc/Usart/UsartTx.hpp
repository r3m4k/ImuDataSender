/** ****************************************************************************
 * @file    UsartTx.hpp
 * @brief   Прикладной USART только для передачи через очередь и IRQ
 * @author  Романовский Роман
 * @date    Октябрь 2026
 * @details Компоненты объединены композицией. Объект существует всё время
 *          работы МК; управление и отправка выполняются единственным писателем из main.
 **************************************************************************** */

#ifndef USART_TX_HPP
#define USART_TX_HPP

/* Includes ------------------------------------------------------------------*/
#include "BasePackage.hpp"
#include "BinaryData.hpp"
#include "UsartHardware.hpp"
#include "UsartIRQ.hpp"
#include "UsartTxChannel.hpp"

namespace STM_CppLib{
    namespace STM_Usart{

        /**
         * @brief Передатчик без RX-пина и декодера.
         * @tparam Type Аппаратный USART/UART.
         * @tparam TxPin Тип вывода передачи.
         * @tparam TxBufferSize Число слотов, степень двойки от 2; полезная ёмкость на один меньше.
         */
        template<UsartTypes Type, STM_GPIO::GpioPinConcept TxPin, uint32_t TxBufferSize = 256>
        class UsartTx{
            friend class UsartIRQ<UsartTx, Type>;

            using tx_channel_t = UsartTxChannel<Type, TxPin, TxBufferSize>;

            UsartHardware<Type> hardware; ///< Аппаратное состояние модуля
            tx_channel_t tx; ///< TX-пин, очередь и аппаратная передача
            UsartIRQ<UsartTx, Type> irq_registration; ///< Единственная регистрация устройства

            /** @brief Обслуживает разрешённые события передачи данного устройства. */
            void irq_handler(){
                if (hardware.is_started()){ tx.handle_interrupt(); }
            }

        public:
            /** @brief Создаёт устройство без настройки аппаратуры и регистрации IRQ. */
            UsartTx() = default;
            UsartTx(const UsartTx&) = delete;
            UsartTx& operator=(const UsartTx&) = delete;
            UsartTx(UsartTx&&) = delete;
            UsartTx& operator=(UsartTx&&) = delete;

            /**
             * @brief Настраивает или переинициализирует USART, оставляя его остановленным.
             * @param config Скорость, формат слова и режим выборки;
             *               значения по умолчанию — 115200, 8N1, Oversampling16.
             * @return false без изменения настроек при занятом ресурсе, активном аппаратном
             *         TX или неподдерживаемой конфигурации. Непустая очередь не мешает Init().
             * @note Очередь сохраняется. Для запуска передачи необходимо вызвать Start().
             */
            bool Init(const UsartConfig& config){
                if (!hardware.validate_baudrate(config.baudrate, config.oversampling)){ return false; }
                if (!irq_registration.register_device(*this)){ return false; }
                if (!Stop()){ return false; }
                UsartConfig selected_config = config;
                selected_config.direction = tx_channel_t::DIRECTION;
                hardware.configure(selected_config);
                tx.init();
                return true;
            }

            /** @brief Запускает USART после Init() или Stop(); возвращает false до успешного Init(). */
            bool Start(){
                if (hardware.is_started()){ return true; }
                if (!hardware.start()){ return false; }
                tx.resume();
                irq_registration.enable();
                return true;
            }

            /** @brief Останавливает свободный аппаратный TX, сохраняя очередь; false при активной передаче. */
            bool Stop(){
                const uint32_t primask = __get_PRIMASK();
                __disable_irq();
                if (hardware.is_started()){
                    if (tx.is_busy()){
                        __set_PRIMASK(primask);
                        return false;
                    }
                    tx.disable_interrupts();
                    hardware.stop();
                }
                __set_PRIMASK(primask);
                return true;
            }

            /** @brief Проверяет незавершённую аппаратную передачу, не наличие программной очереди. */
            bool IsTxBusy() const { return hardware.is_started() && tx.is_busy(); }

            /** @brief Возвращает число свободных байтов в программной TX-очереди; вызывать из main. */
            uint32_t GetTxFreeSpace() const { return tx.free_space(); }

            /** @brief Возвращает число байтов в программной очереди, без аппаратных регистров; вызывать из main. */
            uint32_t GetTxBufferedDataSize() const { return tx.buffered_data_size(); }

            /**
             * @brief Копирует массив целиком в TX-очередь; единственный писатель находится в main.
             * @param data Источник; nullptr разрешён только для size == 0.
             * @param size Число байтов.
             * @return true после копирования всего массива; false без частичной записи.
             */
            bool SendBuffer(const uint8_t* data, uint32_t size){
                return hardware.is_started() && tx.send_buffer(data, size);
            }

            /** @brief Добавляет один байт; возвращает результат SendBuffer(). */
            bool SendByte(uint8_t byte){ return SendBuffer(&byte, 1); }

            /** @brief Копирует пакет в очередь; возвращает результат SendBuffer(). */
            bool SendPackage(const Packages::BasePackage& package){ return SendBuffer(package.data_ptr, package.len); }

            /** @brief Копирует значащие байты сообщения; возвращает результат SendBuffer(). */
            bool SendMessage(const BinaryData::MutableByteView& message){ return SendBuffer(message.bytes_msg, message.msg_size); }

            /** @brief Меняет приоритет всего вектора IRQ после Init(), не меняя его разрешение. */
            void SetInterruptPriority(uint8_t preemption_priority, uint8_t subpriority = 0){
                irq_registration.set_priority(preemption_priority, subpriority);
            }
        };

    } // namespace STM_Usart
} // namespace STM_CppLib

#endif /*   USART_TX_HPP   */
