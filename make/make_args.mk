# ----------------------------------
# Используемые переменные в Makefile
# ----------------------------------

# ОС, на которой собирается проект
# SYSTEM - переменная, задаваемая при запуске makefile
# Зададим её тут, чтобы при запуске makefile из командной строки не задавать её
SYSTEM := windows

# Названеи проекта
PROGRAM_NAME := ImuDataSender

# Корень проекта относительно рабочего каталога Make.
# Скрипты запускают Make из корня проекта.
SOURCE_DIR = .

# Название итогового исполняемого файла
BINARY = ${PROGRAM_NAME}.elf

# Место хранения полученных объектных файлов
ifeq ($(SYSTEM), windows)
	BIN_PLACE = ${SOURCE_DIR}/Debug_Win
else ifeq ($(SYSTEM), linux)
	BIN_PLACE = ${SOURCE_DIR}/Debug_Lin
else
    $(error The OS is not specified)
endif

ifeq ($(SYSTEM), windows)
	GCC_PLACE = C:/SysGCC/arm-eabi
else ifeq ($(SYSTEM), linux)
	GCC_PLACE = /home/mike/gcc-arm-none-eabi-8-2019-q3-update/
else
    $(error The OS is not specified)
endif

# Исполняемый файл компиллятора-компоновщика
CC = ${GCC_PLACE}/bin/arm-none-eabi-gcc
CP = ${GCC_PLACE}/bin/arm-none-eabi-g++

# Флаги компиллятора
COMPILER_FLAGS = \
-mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -O0 -fmessage-length=0 -fsigned-char \
-ffunction-sections -fdata-sections -ffreestanding -fno-move-loop-invariants -Wall -Wextra -g3

# Предопределенные константы
DEFINES= \
-DUSE_STDPERIPH_DRIVER \
-DDEBUG \
-DSTM32F30X \
-DHSE_VALUE=8000000

# флаги для gcc - не перепутывать!!! только gnu11 разрешает пользоваться
# директивами встроенного ассемблера "asm" -они могут встречаться в стандартной библиотеке

GCC_FLAGS = -std=gnu11 ${COMPILER_FLAGS} -c

# Зависимости заголовков сохраняются отдельно для каждого C/C++ объекта.
DEPFLAGS = -MMD -MP -MF "$@.d" -MQ "$@"

# флаги для g++
# GPP_FLAGS = -std=gnu++11 ${COMPILER_FLAGS} -c -fabi-version=0 -fno-exceptions -fno-rtti -fno-use-cxa-atexit -fno-threadsafe-statics
GPP_FLAGS = -std=c++20 ${COMPILER_FLAGS} -c -fno-exceptions -fno-rtti -fno-use-cxa-atexit -fno-threadsafe-statics

INCLUDES = \
-I"${GCC_PLACE}/arm-none-eabi/include" \
-I"${SOURCE_DIR}/include" \
-I"${SOURCE_DIR}/user" \
-I"${SOURCE_DIR}/user/inc" \
-I"${SOURCE_DIR}/user/inc/aaa_Project_Files" \
-I"${SOURCE_DIR}/user/inc/Common" \
-I"${SOURCE_DIR}/user/inc/Communication" \
-I"${SOURCE_DIR}/user/inc/Concepts" \
-I"${SOURCE_DIR}/user/inc/DataContainers" \
-I"${SOURCE_DIR}/user/inc/Exti" \
-I"${SOURCE_DIR}/user/inc/Filters" \
-I"${SOURCE_DIR}/user/inc/Packages" \
-I"${SOURCE_DIR}/user/inc/Periph" \
-I"${SOURCE_DIR}/user/inc/ProgramStages" \
-I"${SOURCE_DIR}/user/inc/Sensors" \
-I"${SOURCE_DIR}/user/inc/Timers" \
-I"${SOURCE_DIR}/user/inc/Usart" \
-I"${SOURCE_DIR}/system/include" \
-I"${SOURCE_DIR}/system/include/cmsis" \
-I"${SOURCE_DIR}/system/include/stm32f3-stdperiph" \
-I"${SOURCE_DIR}/system/include/additionally" \
-I"${SOURCE_DIR}/system/USB_LIB/include"

# Скрипты компоновщика входят в зависимости ELF-файла.
LINKER_SCRIPTS = \
${SOURCE_DIR}/ldscripts/mem.ld \
${SOURCE_DIR}/ldscripts/libs.ld \
${SOURCE_DIR}/ldscripts/sections.ld

# флаги для линковщика
LINK_FLAGS = ${COMPILER_FLAGS} \
$(foreach script,${LINKER_SCRIPTS},-T "$(script)") \
-L"${SOURCE_DIR}/ldscripts" \
-nostartfiles -Xlinker --gc-sections -Wl,-Map,${BIN_PLACE}/${PROGRAM_NAME}.map # --specs=nano.specs 

# Список используемых поддиректорий для сборки проекта
SUBDIRS_OBJ =

# Используемые в проекте библиотеки
LIBS = -L"${GCC_PLACE}/arm-none-eabi/arm/v5te/hard" -lm

# Список всех объектных файлов проекта
OBJECTS =
