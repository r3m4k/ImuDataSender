/** ****************************************************************************
 * @file    Command.hpp
 * @author  Романовский Роман
 * @date    Октябрь 2026
 * @brief   Структурные классы команд для модуля обработки команд.
 *
 * @details Определяет иерархию классов, описывающих команду как связку
 *          кода (байтового массива произвольной длины) и обработчика
 *          (функции без аргументов). Конкретные объекты команд и их
 *          менеджер вынесены в CommandProcessing.hpp.
 *
 *          Классы:
 *          - CommandHandler   — обёртка для функции-обработчика.
 *          - Command          — нешаблонный класс команды (код + обработчик).
 *                             Код представлен view-объектом MutableByteView, 
 *                             ссылающимся на буфер наследника.
 *          - StaticCommand<CodeSize> — шаблонный наследник с собственным
 *            буфером для кода длиной CodeSize байт.
 *          - ByteCommand<CodeSize> — наследник StaticCommand<CodeSize> с CTAD
 *            из массива uint8_t.
 *          - TextCommand<CodeSize> — наследник StaticCommand<CodeSize> с CTAD
 *            из строкового литерала (завершающий '\0' отбрасывается).
 *************************************************************************** */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef COMMAND_HPP
#define COMMAND_HPP

/* Includes ------------------------------------------------------------------*/
#include <cstring>
#include <stdint.h>

#include "BinaryData.hpp"

/* Defines -------------------------------------------------------------------*/

/* Global variables ----------------------------------------------------------*/

// -----------------------------------------------------------------------------
namespace Commands{

    /* Usings ----------------------------------------------------------------*/
    /**
     * @brief   Тип функции-обработчика команды (без аргументов и возврата).
     */
    using CommandCallback = void(*)(void);

    /**
     * @brief   Политика подтверждения получения команды.
     * @details Указывается при регистрации команды и определяет, отправляется
     *          ли Send_Confirm_Cmd при её приёме декодером.
     *          - Required — при приёме команды декодер отправляет подтверждение
     *            (для urgent — немедленно из IRQ, для deferred — через очередь).
     *          - NotRequired — подтверждение не отправляется. Используется
     *            для команд, сам смысл которых — ответ или запрос
     *            (handshake ack, heartbeat ack, запрос confirm от ПК и т.п.).
     */
    enum class ConfirmationPolicy{
        Required,
        NotRequired
    };

    /**
     * @brief   Обёртка для функции-обработчика команды.
     * @details Инкапсулирует указатель на функцию, предоставляет безопасный
     *          запуск (nullptr-обработчик просто ничего не делает).
     *          Поддерживает копирование и перемещение.
     */
    class CommandHandler{
        CommandCallback callback = nullptr;

    public:
        /**
         * @brief   Конструктор по умолчанию (nullptr-обработчик).
         * @details Пустой хэндлер безопасен — execute() на nullptr ничего
         *          не делает. Default-ctor требуется, в частности, для
         *          массивов слотов в SpscRingBuffer<CommandHandler, N>.
         */
        CommandHandler() = default;

        /**
         * @brief   Конструктор из указателя на функцию.
         * @param   callback   Функция-обработчик команды.
         */
        explicit CommandHandler(CommandCallback callback): callback(callback) {}

        /**
         * @brief   Операции копирования.
         */
        CommandHandler(const CommandHandler&) = default;
        CommandHandler& operator=(const CommandHandler&) = default;

        /**
         * @brief   Операции перемещения.
         */
        CommandHandler(CommandHandler&&) noexcept = default;
        CommandHandler& operator=(CommandHandler&&) noexcept = default;

        /**
         * @brief   Запуск обработчика.
         * @details Если callback == nullptr, вызов игнорируется.
         */
        void execute() const {
            if (callback){
                callback();
            }
        }
    };

    // -------------------------------------------------------------------------

    /**
     * @brief   Нешаблонный базовый класс команды.
     * @details Содержит код команды (view-объект MutableByteView, ссылающийся на буфер
     *          наследника) и обработчик. Хранение и размер буфера — забота
     *          наследника StaticCommand<CodeSize>. Command позволяет складывать команды
     *          разной длины кода в один массив указателей на базу.
     * @note    Поля code/handler публичные — в стиле остальных классов проекта.
     * @note    Объекты не удаляются через указатель на Command, поэтому
     *          виртуальный деструктор не требуется.
     */
    class Command{
    public:
        BinaryData::MutableByteView code;           ///< View на буфер кода команды
        CommandHandler handler;                     ///< Обработчик команды
        ConfirmationPolicy confirmation_policy;     ///< Политика подтверждения при приёме

        /**
         * @brief   Конструктор с привязкой к внешнему буферу.
         * @param   code_data             Указатель на буфер кода.
         * @param   code_size             Длина кода в байтах.
         * @param   callback              Функция-обработчик команды.
         * @param   confirmation_policy   Политика подтверждения команды.
         */
        Command(
            uint8_t* code_data,
            uint8_t code_size,
            CommandCallback callback,
            ConfirmationPolicy confirmation_policy
        ):
            code(code_data, code_size),
            handler(callback),
            confirmation_policy(confirmation_policy) {}

        /**
         * @brief   Деструктор по умолчанию.
         */
        ~Command() = default;

        /**
         * @brief   Копирование и перемещение запрещены.
         * @details Команды — глобальные объекты, копировать их нет смысла.
         *          Массивы указателей на Command в CommandManager
         *          хранят ссылки, а не копии.
         */
        Command(const Command&) = delete;
        Command& operator=(const Command&) = delete;
        Command(Command&&) noexcept = delete;
        Command& operator=(Command&&) noexcept = delete;
    };

    // -------------------------------------------------------------------------

    /**
     * @brief   Команда с собственным буфером кода фиксированной длины.
     * @tparam  CodeSize   Длина кода в байтах.
     * @details Наследник Command, владеющий буфером-массивом для кода.
     *          В конструкторе копирует CodeSize байт из переданного указателя
     *          в собственный буфер и инициализирует Command view-объектом
     *          на этот буфер.
     *
     *          Для удобного создания из массивов/литералов следует
     *          использовать наследников ByteCommand<CodeSize> и
     *          TextCommand<CodeSize>,
     *          у каждого из которых есть собственный deduction guide
     *          с выводом CodeSize. Прямое создание объекта
     *          StaticCommand<CodeSize> возможно, но требует явного указания
     *          размера и источника как указателя:
     *
     *            uint8_t raw[3] = {0x01, 0x02, 0x03};
     *            StaticCommand<3> command(raw, callback, confirmation_policy);
     */
    template<uint8_t CodeSize>
    class StaticCommand: public Command{
        static_assert(CodeSize > 0,
            "StaticCommand: command code must not be empty");

        uint8_t code_storage[CodeSize];      ///< Собственный буфер кода команды

    public:
        /**
         * @brief   Конструктор команды из указателя на источник.
         * @param   code_data             Указатель на массив из CodeSize байт.
         *                                Данные копируются в собственный буфер.
         * @param   callback              Функция-обработчик команды.
         * @param   confirmation_policy   Политика подтверждения команды.
         * @note    code_data не должен быть nullptr и должен указывать как
         *          минимум на CodeSize доступных для чтения байт.
         */
        StaticCommand(
            const uint8_t* code_data,
            CommandCallback callback,
            ConfirmationPolicy confirmation_policy
        ):
            Command(code_storage, CodeSize, callback, confirmation_policy){
            std::memcpy(code_storage, code_data, CodeSize);
        }

        /**
         * @brief   Копирование и перемещение запрещены (в стиле Command).
         */
        StaticCommand(const StaticCommand&) = delete;
        StaticCommand& operator=(const StaticCommand&) = delete;
        StaticCommand(StaticCommand&&) noexcept = delete;
        StaticCommand& operator=(StaticCommand&&) noexcept = delete;

        ~StaticCommand() = default;
    };

    // -------------------------------------------------------------------------

    /**
     * @brief   Команда, создаваемая из байтового массива.
     * @tparam  CodeSize   Длина кода в байтах (выводится через deduction guide
     *                     из размера инициализирующего массива).
     * @details Наследник StaticCommand<CodeSize>: не добавляет полей, только
     *          принимает инициализацию в виде массива uint8_t[CodeSize]. 
     * 
     *          Нужен для того, чтобы CTAD корректно выводил CodeSize без конфликта
     *          со TextCommand — каждый тип имеет ровно один способ
     *          создания и ровно один deduction guide.
     */
    template<uint8_t CodeSize>
    class ByteCommand: public StaticCommand<CodeSize>{
    public:
        /**
         * @brief   Конструктор из байтового массива.
         * @param   code_data             Массив байт кода команды.
         * @param   callback              Функция-обработчик команды.
         * @param   confirmation_policy   Политика подтверждения команды.
         */
        ByteCommand(
            const uint8_t (&code_data)[CodeSize], 
            CommandCallback callback, 
            ConfirmationPolicy confirmation_policy
        ):
            StaticCommand<CodeSize>(code_data, callback, confirmation_policy) {}
    };

    /**
     * @brief   Deduction guide для ByteCommand.
     * @details Позволяет писать: ByteCommand command({0x01, 0x02}, callback),
     *          с автоматическим выводом ByteCommand<2>.
     */
    template<uint8_t CodeSize>
    ByteCommand(const uint8_t (&)[CodeSize], CommandCallback, ConfirmationPolicy)
        -> ByteCommand<CodeSize>;

    // -------------------------------------------------------------------------

    /**
     * @brief   Команда, создаваемая из строкового литерала.
     * @tparam  CodeSize   Длина кода в байтах (длина литерала без учёта '\0',
     *                     выводится через deduction guide).
     * @details Наследник StaticCommand<CodeSize>: не добавляет полей, только
     *          принимает инициализацию в виде строкового литерала
     *          длиной CodeSize + 1 (включая завершающий '\0', который
     *          отбрасывается).
     *
     *          Отбрасывание '\0' означает, что на стороне ПК команда
     *          должна отправляться без завершающего нуля (data-поле
     *          пакета содержит только значащие символы).
     */
    template<uint8_t CodeSize>
    class TextCommand: public StaticCommand<CodeSize>{
    public:
        /**
         * @brief   Конструктор из строкового литерала.
         * @param   code_data             Литерал длины CodeSize + 1.
         *                                В буфер копируются первые CodeSize байт.
         * @param   callback              Функция-обработчик команды.
         * @param   confirmation_policy   Политика подтверждения команды.
         */
        TextCommand(
            const char (&code_data)[CodeSize + 1], 
            CommandCallback callback,
            ConfirmationPolicy confirmation_policy
        ):
            StaticCommand<CodeSize>(
                reinterpret_cast<const uint8_t*>(code_data),
                callback,
                confirmation_policy
            ) {}
    };

    /**
     * @brief   Deduction guide для TextCommand.
     * @details Позволяет писать: TextCommand command("TEXT", callback),
     *          с автоматическим выводом TextCommand<4> (длина "TEXT" без '\0').
     *          Строковый литерал имеет тип const char[LiteralSize], где
     *          LiteralSize — длина с учётом '\0'; в целевой тип передаётся
     *          LiteralSize - 1.
     */
    template<uint8_t LiteralSize>
    TextCommand(const char (&)[LiteralSize], CommandCallback, ConfirmationPolicy) 
        -> TextCommand<LiteralSize - 1>;

} // namespace Commands

#endif /*   COMMAND_HPP   */
