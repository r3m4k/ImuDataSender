/** ****************************************************************************
 * @file    CommandManager.hpp
 * @brief   Шаблонный менеджер срочных и отложенных команд.
 * @author  Романовский Роман
 * @date    Ноябрь 2026
 *
 * @details Получает списки команд через константные ссылки в параметрах
 *          шаблона. Выполняет поиск команд по коду и хранит SPSC-очередь
 *          обработчиков для отложенного исполнения в основном цикле.
 **************************************************************************** */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef COMMAND_MANAGER_HPP
#define COMMAND_MANAGER_HPP

/* Includes ------------------------------------------------------------------*/
#include "BinaryData.hpp"
#include "Command.hpp"
#include "SpscRingBuffer.hpp"

// -----------------------------------------------------------------------------
namespace Commands{

    /**
     * @brief   Менеджер команд с заданными при компиляции списками.
     * @tparam  DeferredCommands   Массив указателей на отложенные команды.
     * @tparam  UrgentCommands     Массив указателей на срочные команды.
     * @details Списки передаются по константной ссылке и не копируются.
     *          Поддерживаются C-массивы и std::array с элементами,
     *          преобразуемыми в const Command*.
     * @note    Списки и объекты команд должны иметь статическое время жизни.
     *          Поиск возвращает команду, но не исполняет её обработчик.
     */
    template<const auto& DeferredCommands, const auto& UrgentCommands>
    class CommandManager{
    public:
        /**
         * @brief   Очередь отложенных команд для исполнения в main.
         * @details put() вызывается из единственного контекста приёма команд,
         *          get() — из основного цикла main. Доступно 7 записей.
         */
        Containers::SpscRingBuffer<CommandHandler, 8> command_queue;

        /**
         * @brief   Поиск срочной команды по коду.
         * @param   code   View на байты кода (обычно data-секция пакета).
         * @return  Указатель на найденную Command или nullptr.
         */
        const Command* find_urgent(const BinaryData::MutableByteView& code) const {
            for (const Command* cmd : UrgentCommands){
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
            for (const Command* cmd : DeferredCommands){
                if (cmd->code == code){
                    return cmd;
                }
            }
            return nullptr;
        }

        /**
         * @brief   Добавление команды в очередь отложенного исполнения.
         * @param   command   Команда, обработчик которой копируется в очередь.
         * @return  true, если команда добавлена; false, если очередь заполнена.
         */
        bool add_to_queue(const Command& command){
            return command_queue.put(command.handler);
        }
    };

} // namespace Commands

#endif /*   COMMAND_MANAGER_HPP   */
