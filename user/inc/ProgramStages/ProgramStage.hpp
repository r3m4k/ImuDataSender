/** ****************************************************************************
 * @file    CommandProcessing.hpp
 * @author  Романовский Роман
 * @brief   Для описания стадий программы.
 *
 * @details ...
 *************************************************************************** */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef PROGRAM_STAGE_HPP
#define PROGRAM_STAGE_HPP

/* Includes ------------------------------------------------------------------*/
#include "CommandStructure.hpp"

/* Defines -------------------------------------------------------------------*/

/* Usings --------------------------------------------------------------------*/

/* Global variables ----------------------------------------------------------*/

// -----------------------------------------------------------------------------
namespace ProgramStages{
    // -------------------------------------------------------------------------

    /**
     * @brief   Класс для описание стадии программы и управлением ею.
     * @details Хранит в себе два указателя на CommandHandlerFunc: 
     *          - init_func (функция инициализации стадии);
     *          - execute_func (функция отработки стадии).
     *          Также хранится флаг is_init, который нужен для отслеживания
     *          выполнения инициализации стадии.
     * @note    Перед первым вызовов execute_func необходимо вызвать init_func!
     * 
     * @code 
     *  ProgramStage current_stage = DefaultStage();
     *  if (!current_stage.is_init){
     *      current_stage.init();
     *  }
     *  current_stage.execute();
     * @endcode
     */
    class ProgramStage{
        CommandHandlerFunc init_func;
        CommandHandlerFunc execute_func;
        
    public:
        bool is_init = false;

        ProgramStage() = delete;

        ProgramStage(CommandHandlerFunc _init_func, CommandHandlerFunc _execute_func):
            init_func(_init_func), execute_func(_execute_func) {}

        void init(){
            init_func();
            is_init = true;
        }

        void execute(){
            execute_func();
        }
    };

} // namespace ProgramStages

#endif /*   PROGRAM_STAGE_HPP   */