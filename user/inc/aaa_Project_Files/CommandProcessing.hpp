/** ****************************************************************************
 * @file    CommandProcessing.hpp
 * @author  Романовский Роман
 * @brief   Модуль обработки команд для встроенной системы.
 *
 * @details Регистрирует поддерживаемые команды и задаёт тип менеджера
 *          для их сопоставления и выполнения. Структурные классы команд
 *          определены в Command.hpp, шаблон менеджера — в CommandManager.hpp.
 *
 *          Поддерживаются два вида команд:
 *          - Срочные (urgent) — выполняются немедленно в контексте декодера
 *            (в IRQ USART). Используются для коротких IRQ-safe действий.
 *          - Отложенные (deferred) — помещаются в очередь и исполняются
 *            в основном цикле программы.
 *
 *          В этом файле определены:
 *          - inline-объекты ByteCommand/TextCommand (поддерживаемые команды);
 *          - массивы указателей на срочные и отложенные команды;
 *          - ProjectCommandManager — менеджер с этими списками команд.
 *************************************************************************** */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef COMMAND_PROCESSING_HPP
#define COMMAND_PROCESSING_HPP

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "Command.hpp"
#include "CommandManager.hpp"

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
     * @brief   Список отложенных команд (помещаются в очередь).
     * @note    Новую отложенную команду необходимо включить в этот массив.
     */
    inline constexpr const Command* deferred_commands[] = {
        &Set_FooStage_Cmd,
        &Set_MeasureStage_Cmd,
        &Send_Confirm_Cmd,
        &Send_Handshake_Ack_Cmd,
        &Send_Heartbeat_Ack_Cmd
    };

    /**
     * @brief   Список срочных команд (исполняются в контексте декодера).
     * @note    Новую срочную команду необходимо включить в этот массив.
     */
    inline constexpr const Command* urgent_commands[] = {
        &Restart_Cmd
    };

    /**
     * @brief   Тип менеджера с командами данного приложения.
     */
    using ProjectCommandManager = CommandManager<deferred_commands, urgent_commands>;

} // namespace Commands

#endif /*   COMMAND_PROCESSING_HPP   */