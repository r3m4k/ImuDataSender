/* Includes H files ----------------------------------------------------------*/
#include "main.h"

/* Includes HPP files --------------------------------------------------------*/
#include <array>
#include <variant>

#include "Consts.hpp"
#include "GPTimers.hpp"
#include "Leds.hpp"
#include "GpioPin.hpp"
#include "Usart.hpp"

#include "CommandProcessing.hpp"
#include "Messages.hpp"
#include "MessagePackage.hpp"
#include "ImuPackage.hpp"
#include "RingBuffer.hpp"

#include "TriaxialData.hpp"
#include "SimpleKalmanFilter.hpp"

#include "L3GD20.hpp"
#include "LSM303DLHC.hpp"
#include "SensorsKalmanParams.hpp"

// ----------------------------------------------------------------------------
//
// Standalone STM32F3 empty sample (trace via NONE).
//
// Trace support is enabled by adding the TRACE macro definition.
// By default the trace messages are forwarded to the NONE output,
// but can be rerouted to any device or completely suppressed, by
// changing the definitions required in system/src/diag/trace_impl.c
// (currently OS_USE_TRACE_ITM, OS_USE_TRACE_SEMIHOSTING_DEBUG/_STDOUT).
//

/* #global variables -----------------------------------------*/
RCC_ClocksTypeDef RCC_Clocks; // structure used for setting up the SysTick Interrupt

// Unused global variables that have to be included to ensure correct compiling
// ###### DO NOT CHANGE ######
// ===============================================================================
__IO uint32_t TimingDelay = 0;                     // used with the Delay function
__IO uint8_t DataReady = 0;
__IO uint32_t USBConnectTimeOut = 100;
__IO uint32_t UserButtonPressed = 0;
__IO uint8_t PrevXferComplete = 1;
__IO uint8_t buttonState;
// ===============================================================================

/* Defines -------------------------------------------------------------------*/
#define IST_VECTORS_NUM     98          // Количество векторов прерываний
#define MessageLen          8           // Длина отправляемых информационных сообщений

/* Typedefs ------------------------------------------------------------------*/
typedef void (* const pHandler)(void);

/* Global variables ---------------------------------------------------------*/
extern pHandler __isr_vectors[];

/* ****************************************************************************
 * Пользовательские переменные
 *************************************************************************** */

// Собственная таблица прерываний
__attribute__((aligned(128)))    // Cortex-M4 требует выравнивание по 128 байт!
_user_pHandler _user_vector_table[IST_VECTORS_NUM] = {0};

// Необходимые счётчик и флаг
volatile uint32_t tick_counter = 0;
volatile bool ready_flag = false;

// Светодиоды на плате
STM_CppLib::Leds leds;

// Обработчик поступивших команд
Commands::CommandManager command_manager;

// Интерфейс связи: USART1 на пинах PC4 (TX) и PC5 (RX)
using PinTX_t = STM_CppLib::STM_GPIO::GPIO_Pin
    <STM_CppLib::STM_GPIO::GPIO_Port::PortC, GPIO_PinSource4>;
using PinRX_t = STM_CppLib::STM_GPIO::GPIO_Pin
    <STM_CppLib::STM_GPIO::GPIO_Port::PortC, GPIO_PinSource5>;

STM_CppLib::STM_Usart::Usart1<PinTX_t, PinRX_t> usart1;

// Настройка EXTI на PC1, которое будет программно инициироваться
STM_CppLib::STM_GPIO::GPIO_Pin_EXTI
    <STM_CppLib::STM_GPIO::GPIO_Port::PortC, GPIO_PinSource1, send_data_package> pin_pc1;

// Используемые таймеры -------------------------------------------------------

// Таймер для чтения данных датчиков с частотой 200 Гц
STM_CppLib::STM_Timer::Timer2<[](){
    /* Объявление лямбды, которая будет вызываться в прерывании */
    leds.ChangeLedStatus(LED9);

    tick_counter++;
    ready_flag = true;

}>  timer2;

// Таймер для мерцания светодиодами LED6, LED7
STM_CppLib::STM_Timer::Timer4<[](){
    /* Объявление лямбды, которая будет вызываться в прерывании */
    leds.ChangeLedStatus(LED6);
    leds.ChangeLedStatus(LED7);
}>  timer4;


/* ****************************************************************************
 * Объявление датчиков, используемых фильтров и пакетов данных
 *************************************************************************** */

STM_CppLib::L3GD20      sensor_L3GD20;          // Встроенный гироскоп
STM_CppLib::LSM303DLHC  sensor_LSM303DLHC;      // Встроенный датчик с акселерометром,
                                                // магнитным и температурным датчиками

SimpleKalmanFilter<TriaxialData> acc_filter(LSM303DLHC_acc_variance / 50, LSM303DLHC_acc_variance);
SimpleKalmanFilter<TriaxialData> gyro_filter(L3GD20_gyro_variance   / 50, L3GD20_gyro_variance);

Packages::ImuDataPackage data_package(&acc_filter.filtered_value, &gyro_filter.filtered_value);


/* ****************************************************************************
 * Описание стадий программы
 *************************************************************************** */

class ProgramStage{
    CommandHandlerFunc init_func;
    CommandHandlerFunc execute_func;
    
public:
    bool is_init = false;

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

// -------------------------------------------------------------------------------

// Очередь стадий программ (используется для смены стадий программ).
RingBuffer<ProgramStage*, 2> program_stage_queue;

// Поддерживаемые стадии программы
ProgramStage FooStage(FooStage_init, FooStage_execute);
ProgramStage MeasureStage(MeasureStage_init, MeasureStage_execute);


/* **************************************************************************** */

int main()
{
    /* ***************************************************************************
    * Загрузим собственную таблицу прерываний для возможности её модификации
    *************************************************************************** */

    __disable_irq();    // Отключим прерывания

    // Скопируем исходную таблицу прерываний
    memcpy(_user_vector_table, __isr_vectors, IST_VECTORS_NUM * sizeof(pHandler));

    SCB->VTOR = (uint32_t)_user_vector_table;

    __DSB();    // Ожидаем завершения записи в регистр VTOR
    __ISB();    // Сбрасываем конвейер команд, чтобы следующие инструкции и прерывания
                // использовали новую таблицу векторов

    __enable_irq();     // Включим прерывания

    // ---------------------------------------------------------------------------

    // Получаем текущие значения тактовых частот системы и настроим
    // SysTick для генерации прерываний с периодом 1 мс
    // Если конфигурация SysTick завершилась ошибкой – входим в бесконечный цикл
	RCC_GetClocksFreq(&RCC_Clocks);
	if (SysTick_Config(RCC_Clocks.HCLK_Frequency / 1000))
		while(true) {}
    
    // ---------------------------------------------------------------------------

    // Инициализируем всё оборудования
    InitAll();             
    
    // Поморгаем светодиодами после успешной инициализации
    leds.ToggleLeds();

    // ---------------------------------------------------------------------------

    // Изначально добавим FooStage в program_stage_queue
    // program_stage_queue.put(&FooStage);
    program_stage_queue.put(&MeasureStage);
    ProgramStage* current_stage_ptr = &FooStage;

    // ---------------------------------------------------------------------------
    // Основной цикл программы
    while (true)
    {
        // Выполним все поступившие команды при их наличии
        while (!command_manager.command_queue.is_empty()){
            auto command = command_manager.command_queue.get();
            command.execute();
        }

        // Сменим current_stage_ptr, если есть элементы в очереди program_stage_queue
        if(!program_stage_queue.is_empty()){
            current_stage_ptr->is_init = false;     // Сбросим флаг у текущей стадии
            current_stage_ptr = program_stage_queue.get();
        }

        /* ***********************************************************************
        ШАБЛОН ОТРАБОТКИ СТАДИИ ПРОГРАММЫ:
        Каждая стадия (ProgramStage) отрабатывается по единому принципу:
        1. ИНИЦИАЛИЗАЦИЯ СТАДИИ (однократное выполнение при входе в стадию)
        2. ЦИКЛИЧЕСКОЕ ВЫПОЛНЕНИЕ ОСНОВНОЙ ЛОГИКИ СТАДИИ
        *********************************************************************** */

        if (!current_stage_ptr->is_init){
            current_stage_ptr->init();
        }
        current_stage_ptr->execute();        
    }
}

// -------------------------------------------------------------------------------
// Инициализация оборудования
// -------------------------------------------------------------------------------
void InitAll(){

    leds.Init();
    leds.LedsOn();
    
    usart1.Init(115200);
    usart1.EnableRxInterrupt();

    sensor_L3GD20.Init();
    sensor_LSM303DLHC.Init();

    pin_pc1.InitPinExti();

    // Настройка основного таймера с частотой 200 Гц
    uint32_t tim2_period = 50 - 1;
    timer2.Init(tim2_period, Prescaler_10kHz, nullptr, 2, 0);

    // Настройка таймера для мерцания светодиодами с периодом счёта в 2 с
    uint32_t tim4_period = 20000 - 1;
    timer4.Init(tim4_period);
}

// -------------------------------------------------------------------------------
// Функции для отработки стадий программы
// -------------------------------------------------------------------------------

// Функция для инициализации FooStage
void FooStage_init(){
    // Остановим все таймеры
    timer2.Stop();
    timer2.ResetCounter();
    timer4.Stop();
    timer4.ResetCounter();
    
    // Включим все светодиоды
    leds.LedsOn();
}

// Функция для исполнения FooStage 
void FooStage_execute(){
    __NOP();
}

// Функция для инициализации MeasureStage
void MeasureStage_init(){
    tick_counter = 0;
    leds.LedsOff();

    // Запустим таймеры
    timer2.ResetCounter();
    timer4.ResetCounter();
    
    timer2.Start();
    timer4.Start();
}

// Функция для исполнения MeasureStage 
void MeasureStage_execute(){
    if (ready_flag){
        // Переключим светодиод для индикации работы
        leds.ChangeLedStatus(LED8);

        // Считаем данные с датчиков
        sensor_L3GD20.ReadData();
        sensor_LSM303DLHC.ReadData();

        // Добавим значения в фильтр
        acc_filter.append_value(sensor_LSM303DLHC.acc_data);
        gyro_filter.append_value(sensor_L3GD20.gyro_data);

        // В прерывании обновим данные посылки и отправим её по usart1
        pin_pc1.GenerateSWInterrupt();

        // Сбросим флаг
        ready_flag = false;
    }
}


// -------------------------------------------------------------------------------
// Функция отправки пакета данных 
// (вызывается в прерывании для гарантии целостности данных)
// -------------------------------------------------------------------------------
void send_data_package(){
    data_package.UpdateData();
    data_package.UpdateControlSum();
    usart1.SendPackage(data_package);
}

// -------------------------------------------------------------------------------
// Функции для отработки поступивших команд
// -------------------------------------------------------------------------------

// Функции для перезагрузки МК
void restart(){
    NVIC_SystemReset();
}

// Функция для добавления FooStage в очередь program_stage_queue
void set_FooStage(){
    program_stage_queue.put(&FooStage);
}

// Функция для добавления MeasureStage в очередь program_stage_queue
void set_MeasureStage(){
    program_stage_queue.put(&MeasureStage);
}


// -------------------------------------------------------------------------------
// Отправка сообщений
// -------------------------------------------------------------------------------

void send_confirm_msg(){
    const char* text = "CONFIRM_RECEIVED_COMMAND";
    Packages::MessagePackage msg_package(text, strlen(text));
    usart1.SendPackage(msg_package);
}

void send_handshake_ack(){
    const char* text = "IMU_STM32_ACK";
    Packages::MessagePackage msg_package(text, strlen(text));
    usart1.SendPackage(msg_package);
}

void send_heartbeat_ack(){
    const char* text = "IMU_STM32_ALIVE";
    Packages::MessagePackage msg_package(text, strlen(text));
    usart1.SendPackage(msg_package);
}

void send_error_msg(){
    const char* text = "UNKNOWN_COMMAND";
    Packages::MessagePackage msg_package(text, strlen(text));
    usart1.SendPackage(msg_package);
}


// -------------------------------------------------------------------------------
// Системные функции
// -------------------------------------------------------------------------------

void Error_Handler(void)
{
    /* Turn LED10/3 (RED) on */
    STM_EVAL_LEDOn(LED10);
    STM_EVAL_LEDOn(LED3);
    while (1)
    {
    }
}


// Function to insert a timing delay of nTime
// ###### DO NOT CHANGE ######
void Delay(__IO uint32_t nTime)
{
    TimingDelay = nTime;

    while (TimingDelay != 0){}
}

// Function to Decrement the TimingDelay variable.
// ###### DO NOT CHANGE ######
void TimingDelay_Decrement(void)
{
    if (TimingDelay != 0x00)
    {
        TimingDelay--;
    }
}

// Unused functions that have to be included to ensure correct compiling
// ###### DO NOT CHANGE ######
// =======================================================================
uint32_t L3GD20_TIMEOUT_UserCallback(void)
{
    return 0;
}

uint32_t LSM303DLHC_TIMEOUT_UserCallback(void)
{
    return 0;
}

void UserEP3_OUT_Callback(uint8_t *buffer)
{
    return;
}
// =======================================================================