/** ****************************************************************************
 * @file    ByteDecoder.hpp
 * @brief   Минимальный контракт обработчика входного байтового потока
 * @author  Романовский Роман
 * @date    Октябрь 2026
 *
 * @details Не зависит от аппаратной платформы и конкретного протокола.
 **************************************************************************** */

#ifndef BYTE_DECODER_HPP
#define BYTE_DECODER_HPP

/* Includes ------------------------------------------------------------------*/
#include <concepts>
#include <stdint.h>

namespace STM_CppLib{
    namespace STM_Usart{

        /**
         * @brief   Проверка вызова обработчика для одного принятого байта.
         * @tparam  T   Тип обработчика; наследование от BaseDecoder не требуется.
         */
        template<typename T>
        concept ByteDecoder = requires(T& decoder, uint8_t byte){
            { decoder.byte_processing(byte) } -> std::same_as<void>;
        };

    } // namespace STM_Usart
} // namespace STM_CppLib

#endif /*   BYTE_DECODER_HPP   */
