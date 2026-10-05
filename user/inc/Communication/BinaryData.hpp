/** ***************************************************************************
 * @file    MutableByteView.hpp
 * @author  Романовский Роман
 * @brief   Универсальный контейнер сообщения + наследник с собственным буфером.
 *
 * @details Классы для работы с сообщениями произвольной длины:
 *          - MutableByteView — базовый контейнер, не владеющий буфером напрямую.
 *            Хранит указатель на внешний буфер и текущий размер значащих
 *            данных. Используется как тип приёма по ссылке (const MutableByteView&)
 *            для сообщений любого размера.
 *          - StaticByteBuffer<N> — наследник с собственным буфером фиксированной
 *            длины N байт.
 *************************************************************************** */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef BINARY_DATA_HPP
#define BINARY_DATA_HPP

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <string.h>

/* Defines -------------------------------------------------------------------*/

/* Global variables ----------------------------------------------------------*/

// -----------------------------------------------------------------------------
namespace BinaryData{

    /**
     * @brief   Базовый контейнер сообщения, без владения буфером.
     * @details Хранит указатель на внешний буфер и текущий размер значащих
     *          данных. Сам буфер, его обнуление и безопасное копирование
     *          находятся в наследнике (StaticByteBuffer<N>).
     * @note    Время жизни буфера должно перекрывать время жизни объекта.
     *          Класс не выполняет аллокаций.
     */
    class MutableByteView{
    public:
        uint8_t* bytes_msg = nullptr;   ///< Указатель на буфер
        uint8_t msg_size = 0;           ///< Фактический размер данных

        /**
         * @brief   Конструктор по умолчанию запрещён — MutableByteView без буфера невалиден.
         */
        MutableByteView() = delete;

        /**
         * @brief   Конструктор с привязкой к внешнему буферу.
         * @param   buf   Указатель на внешний буфер.
         * @note    Данные не копируются. buf должен оставаться валидным
         *          всё время жизни объекта. msg_size не изменяется (0 по умолчанию).
         */
        explicit MutableByteView(uint8_t* buf): bytes_msg(buf) {}

        /**
         * @brief   Конструктор-view с указанием размера значащих данных.
         * @param   buf    Указатель на внешний буфер.
         * @param   size   Количество значащих байт в буфере (msg_size).
         * @note    Данные не копируются. Используется для построения view
         *          на готовый участок (например, data-секцию принятого пакета).
         */
        MutableByteView(uint8_t* buf, uint8_t size): bytes_msg(buf), msg_size(size) {}

        /**
         * @brief   Деструктор по умолчанию.
         */
        ~MutableByteView() = default;

        /**
         * @brief   Move-операции запрещены (класс не поддерживает move-семантику).
         */
        MutableByteView(MutableByteView&&) noexcept = delete;
        MutableByteView& operator=(MutableByteView&&) noexcept = delete;

        /**
         * @brief   Оператор сравнения двух сообщений.
         * @return  true, если размеры и содержимое совпадают.
         */
        bool operator==(const MutableByteView& other) const {
            if (msg_size != other.msg_size) return false;
            return memcmp(bytes_msg, other.bytes_msg, msg_size) == 0;
        }

        /**
         * @brief   Лёгкий сброс сообщения – только обнуление размера.
         * @details Содержимое буфера не стирается; следующие записи через
         *          наследника перезапишут нужные ячейки. Безопасно вызывать
         *          в «горячем пути» декодера (нет memset'а в IRQ).
         */
        void reset() {
            msg_size = 0;
        }
    };

    // -------------------------------------------------------------------------

    /**
     * @brief   Сообщение с собственным буфером фиксированной длины.
     * @tparam  Capacity   Ёмкость буфера в байтах.
     * @details Наследник MutableByteView, владеющий буфером-массивом. Предоставляет
     *          операции, требующие знания размера буфера (clear, operator=).
     *          Доступ к полям bytes_msg/msg_size — через унаследованные поля базы.
     */
    template<uint8_t Capacity>
    class StaticByteBuffer: public MutableByteView{
        uint8_t buffer[Capacity] = {0};   ///< Собственный буфер фиксированного размера

    public:
        /**
         * @brief   Конструктор по умолчанию. Буфер инициализируется нулями.
         */
        StaticByteBuffer(): MutableByteView(buffer) {}

        /**
         * @brief   Конструктор, копирующий данные из внешнего буфера.
         * @param   src    Указатель на источник данных.
         * @param   size   Количество байт для копирования (не более N).
         */
        StaticByteBuffer(const uint8_t* src, uint8_t size): MutableByteView(buffer) {
            if (size > Capacity) size = Capacity;
            msg_size = size;
            memcpy(buffer, src, size);
        }

        /**
         * @brief   Конструктор копирования.
         */
        StaticByteBuffer(const StaticByteBuffer& other): MutableByteView(buffer) {
            msg_size = other.msg_size;
            memcpy(buffer, other.bytes_msg, msg_size);
        }

        /**
         * @brief   Оператор присваивания копированием.
         * @details Защита от переполнения: если other.msg_size превышает N,
         *          копируется не более N байт.
         */
        StaticByteBuffer& operator=(const StaticByteBuffer& other) {
            if (this != &other) {
                uint8_t n = (other.msg_size <= Capacity) ? other.msg_size : Capacity;
                memcpy(buffer, other.bytes_msg, n);
                msg_size = n;
            }
            return *this;
        }

        /**
         * @brief   Move-операции запрещены (класс не поддерживает move-семантику).
         */
        StaticByteBuffer(StaticByteBuffer&&) noexcept = delete;
        StaticByteBuffer& operator=(StaticByteBuffer&&) noexcept = delete;

        /**
         * @brief   Очистка сообщения – заполняет весь буфер нулями.
         */
        void clear() {
            memset(buffer, 0, Capacity);
            msg_size = 0;
        }

        /**
         * @brief   Деструктор по умолчанию.
         */
        ~StaticByteBuffer() = default;
    };

} // namespace MutableByteView

#endif /*   BINARY_DATA_HPP   */