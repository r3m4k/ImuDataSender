/** ****************************************************************************
 * @file    UsartTxRx.hpp
 * @brief   Прикладной USART с независимыми TX/RX-компонентами и одной регистрацией IRQ
 * @author  Романовский Роман
 * @date    Октябрь 2026
 * @details Не наследуется от UsartTx или UsartRx. Объект существует всё время работы МК.
 **************************************************************************** */

#ifndef USART_TX_RX_HPP
#define USART_TX_RX_HPP

/* Includes ------------------------------------------------------------------*/
#include "BasePackage.hpp"
#include "BinaryData.hpp"
#include "UsartHardware.hpp"
#include "UsartIRQ.hpp"
#include "UsartTxChannel.hpp"
#include "UsartRxChannel.hpp"

namespace STM_CppLib{
    namespace STM_Usart{

        /**
         * @brief Приёмопередатчик с очередью main → TX IRQ и декодером RX IRQ.
         * @tparam Type Аппаратный USART/UART.
         * @tparam TxPin Тип вывода передачи.
         * @tparam RxPin Тип вывода приёма.
         * @tparam Decoder Тип с методом void byte_processing(uint8_t).
         * @tparam TxBufferSize Число слотов, степень двойки от 2; полезная ёмкость на один меньше.
         * @note Управление и отправка только из main. Декодер не отправляет в TX-очередь.
         */
        template<UsartTypes Type, STM_GPIO::GpioPinConcept TxPin, STM_GPIO::GpioPinConcept RxPin,
                 ByteDecoder Decoder, uint32_t TxBufferSize = 256>
        class UsartTxRx{
            friend class UsartIRQ<UsartTxRx, Type>;

            using tx_channel_t = UsartTxChannel<Type, TxPin, TxBufferSize>;
            using rx_channel_t = UsartRxChannel<Type, RxPin, Decoder>;

            UsartHardware<Type> hardware; ///< Общая настройка и состояние USART
            tx_channel_t tx; ///< TX-пин, очередь и аппаратная передача
            rx_channel_t rx; ///< RX-пин, аппаратный приём и декодер
            UsartIRQ<UsartTxRx, Type> irq_registration; ///< Одна регистрация для обоих направлений

            /** @brief Обслуживает TX до пользовательского RX-декодера, не задерживая ожидающий TXE. */
            void irq_handler(){
                if (!hardware.is_started()){ return; }
                tx.handle_interrupt();
                rx.handle_interrupt();
            }

        public:
            /** @brief Создаёт устройство без настройки аппаратуры и регистрации IRQ. */
            UsartTxRx() = default;
            UsartTxRx(const UsartTxRx&) = delete;
            UsartTxRx& operator=(const UsartTxRx&) = delete;
            UsartTxRx(UsartTxRx&&) = delete;
            UsartTxRx& operator=(UsartTxRx&&) = delete;

            /**
             * @brief Настраивает или переинициализирует оба направления и запускает устройство.
             * @param config Скорость, формат слова и режим выборки;
             *               значения по умолчанию — 115200, 8N1, Oversampling16.
             * @return false без изменения настроек при занятом ресурсе, активном аппаратном
             *         TX/RX или неподдерживаемой конфигурации. Непустая очередь не мешает Init().
             * @note Очередь и декодер сохраняются, ожидающие данные TX используют новые настройки.
             *       Непрочитанное аппаратное слово RX при перенастройке сбрасывается.
             */
            bool Init(const UsartConfig& config){
                if (!hardware.validate_baudrate(config.baudrate, config.oversampling)){ return false; }
                if (!irq_registration.register_device(*this)){ return false; }
                if (!Stop()){ return false; }
                UsartConfig selected_config = config;
                selected_config.direction = tx_channel_t::DIRECTION | rx_channel_t::DIRECTION;
                hardware.configure(selected_config);
                tx.init();
                rx.init();
                return Start();
            }

            /** @brief Возобновляет оба направления и очередь; возвращает false до успешного Init(). */
            bool Start(){
                if (hardware.is_started()){ return true; }
                if (!hardware.start()){ return false; }
                rx.enable_interrupts();
                tx.resume();
                irq_registration.enable();
                return true;
            }

            /** @brief Останавливает устройство, сохраняя очередь и декодер; false при активном TX/RX. */
            bool Stop(){
                const uint32_t primask = __get_PRIMASK();
                __disable_irq();
                if (hardware.is_started()){
                    if (tx.is_busy() || rx.is_busy()){
                        __set_PRIMASK(primask);
                        return false;
                    }
                    tx.disable_interrupts();
                    rx.disable_interrupts();
                    hardware.stop();
                }
                __set_PRIMASK(primask);
                return true;
            }

            /** @brief Проверяет незавершённую аппаратную передачу независимо от программной очереди. */
            bool IsTxBusy() const { return hardware.is_started() && tx.is_busy(); }

            /** @brief Проверяет приём отдельного слова, а не пакета и не наличие байта в RDR. */
            bool IsRxBusy() const { return hardware.is_started() && rx.is_busy(); }

            /** @brief Возвращает число свободных байтов в программной TX-очереди; вызывать из main. */
            uint32_t GetTxFreeSpace() const { return tx.free_space(); }

            /** @brief Возвращает число байтов программной очереди без аппаратных регистров; вызывать из main. */
            uint32_t GetTxBufferedDataSize() const { return tx.buffered_data_size(); }

            /**
             * @brief Копирует массив целиком в очередь; единственный писатель находится в main.
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

#endif /*   USART_TX_RX_HPP   */
