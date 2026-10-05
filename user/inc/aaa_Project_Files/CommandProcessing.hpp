/** ****************************************************************************
 * @file    CommandProcessing.hpp
 * @author  Романовский Роман
 * @brief   Модуль обработки команд для встроенной системы.
 *
 * @details Регистрирует поддерживаемые команды и предоставляет менеджер
 *          для их сопоставления и выполнения. Структурные классы команд
 *          (CommandHandler, Command, Command<N>, ByteCommand<N>,
 *          TextCommand<N>) определены в CommandStructure.hpp.
 *
 *          Поддерживаются два вида команд:
 *          - Срочные (urgent) — выполняются немедленно в контексте декодера
 *            (в IRQ USART). Используются для коротких IRQ-safe действий.
 *          - Отложенные (deferred) — помещаются в очередь и исполняются
 *            в основном цикле программы.
 *
 *          В этом файле определены:
 *          - inline-объекты ByteCommand/TextCommand (поддерживаемые команды);
 *          - CommandManager — контейнер со списками urgent/deferred команд
 *                             и очередью для отложенного исполнения.
 *************************************************************************** */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef COMMAND_PROCESSING_HPP
#define COMMAND_PROCESSING_HPP

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

#include "main.h"
#include "Messages.hpp"
#include "SpscRingBuffer.hpp"
#include "CommandStructure.hpp"

/* Defines -------------------------------------------------------------------*/

/* Usings --------------------------------------------------------------------*/

/* Global variables ----------------------------------------------------------*/

// -----------------------------------------------------------------------------
namespace Commands{

    // -------------------------------------------------------------------------
    /*!
     * @defgroup SupportedCommands Поддерживаемые команды
     * @brief    Глобальные объекты команд, регистрируемые в CommandManager.
     * @details  Каждая команда — inline-объект ByteCommand<N> или TextCommand<N>
     * @{
     */

    /**
     * @brief   Срочная команда: перезагрузка микроконтроллера.
     */
    inline ByteCommand Restart_Cmd({0xff, 0xff}, restart, ConfirmationPolicy::Required);

    /**
     * @brief   Отложенная команда: переход в стадию FooStage.
     */
    inline ByteCommand Set_FooStage_Cmd({0xaa, 0x01}, set_FooStage, ConfirmationPolicy::Required);

    /**
     * @brief   Отложенная команда: переход в стадию MeasureStage.
     */
    inline ByteCommand Set_MeasureStage_Cmd({0xaa, 0x02}, set_MeasureStage, ConfirmationPolicy::Required);

    /**
     * @brief   Отложенная команда: отправка подтверждения приёма.
     */
    inline TextCommand Send_Confirm_Cmd("CONFIRM", send_confirm_msg, ConfirmationPolicy::NotRequired);

    /**
     * @brief   Отложенная команда: отправка ack на handshake.
     */
    inline TextCommand Send_Handshake_Ack_Cmd("HANDSHAKE_REQ", send_handshake_ack, ConfirmationPolicy::NotRequired);

    /**
     * @brief   Отложенная команда: отправка ack на heartbeat.
     */
    inline TextCommand Send_Heartbeat_Ack_Cmd("HEARTBEAT_REQ", send_heartbeat_ack, ConfirmationPolicy::NotRequired);

    /**
     * @brief   Системная команда: отправка сообщения об ошибке.
     */
    inline TextCommand Send_Error_Cmd("UNKNOWN_COMMAND", send_error_msg, ConfirmationPolicy::NotRequired);

    /** @} */ // SupportedCommands

    // -------------------------------------------------------------------------

    /**
     * @brief   Менеджер поддерживаемых команд.
     * @details Хранит два статических списка указателей на Command:
     *          urgent_commands (исполняются в IRQ-контексте немедленно)
     *          и deferred_commands (помещаются в очередь для исполнения в main).
     *          Также владеет очередью CommandHandler для отложенных команд.
     * @note    При добавлении новой команды (ByteCommand или TextCommand)
     *          не забыть включить её в соответствующий массив ниже.
     */
    class CommandManager{
    public:
        /**
         * @brief   Список срочных команд (исполняются в контексте декодера).
         */
        inline static const Command* urgent_commands[] = {
            &Restart_Cmd
        };

        /**
         * @brief   Список отложенных команд (помещаются в очередь).
         */
        inline static const Command* deferred_commands[] = {
            &Set_FooStage_Cmd,
            &Set_MeasureStage_Cmd,
            &Send_Confirm_Cmd,
            &Send_Handshake_Ack_Cmd,
            &Send_Heartbeat_Ack_Cmd
        };

        /**
         * @brief   Очередь отложенных команд для исполнения в main.
         * @details Используется SPSC-очередь: put() вызывается из контекста
         *          USART IRQ (через DecoderImu::process_command_packet),
         *          get() — из основного цикла main.
         */
        Containers::SpscRingBuffer<CommandHandler, 8> command_queue;

        /**
         * @brief   Поиск срочной команды по коду.
         * @param   code   View на байты кода (обычно data-секция пакета).
         * @return  Указатель на найденную Command или nullptr.
         */
        const Command* find_urgent(const BinaryData::MutableByteView& code) const {
            for (const Command* cmd : urgent_commands){
                if (cmd->code == code){
                    return cmd;
                }
            }
            return nullptr;
        }

        /**
         * @brief   Поиск отложенной команды по коду.
         * @param   code   View на байты кода (обычно data-секция пакета).
         * @return  Указатель на найденную Command или nullptr.
         */
        const Command* find_deferred(const BinaryData::MutableByteView& code) const {
            for (const Command* cmd : deferred_commands){
                if (cmd->code == code){
                    return cmd;
                }
            }
            return nullptr;
        }

        /**
         * @brief   Добавление команды в очередь отложенного исполнения.
         * @param   command   Ссылка на команду (только её handler копируется
         *                    в очередь).
         * @return  true, если команда добавлена; false, если очередь заполнена.
         */
        bool add_to_queue(const Command& command){
            return command_queue.put(command.handler);
        }
    };

} // namespace Commands

#endif /*   COMMAND_PROCESSING_HPP   */