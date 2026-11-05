@echo off
setlocal EnableExtensions DisableDelayedExpansion

rem Настройки читаются как пары ИМЯ=значение, без выполнения команд из .env.
for %%I in ("%~dp0..") do set "embedded_dir=%%~fI"
set "env_file=%embedded_dir%\.env"

if not exist "%env_file%" (
    echo Local configuration not found: "%env_file%" 1>&2
    echo Copy .env.example to .env and configure the tool paths. 1>&2
    exit /b 1
)

for %%V in (PROGRAM_NAME BUILD_DIR OPENOCD_EXE OPENOCD_SCRIPTS_DIR OPENOCD_CONFIG) do set "%%V="
for /f "usebackq eol=# tokens=1,* delims==" %%A in ("%env_file%") do (
    if "%%A"=="PROGRAM_NAME" set "PROGRAM_NAME=%%~B"
    if "%%A"=="BUILD_DIR" set "BUILD_DIR=%%~B"
    if "%%A"=="OPENOCD_EXE" set "OPENOCD_EXE=%%~B"
    if "%%A"=="OPENOCD_SCRIPTS_DIR" set "OPENOCD_SCRIPTS_DIR=%%~B"
    if "%%A"=="OPENOCD_CONFIG" set "OPENOCD_CONFIG=%%~B"
)

for %%V in (PROGRAM_NAME BUILD_DIR OPENOCD_EXE OPENOCD_SCRIPTS_DIR OPENOCD_CONFIG) do if not defined %%V (
    echo Missing configuration value: %%V 1>&2
    exit /b 1
)

rem Явно заданный ELF разрешается относительно каталога вызывающего процесса.
set "elf_file=%embedded_dir%\%BUILD_DIR%\%PROGRAM_NAME%.elf"
if not "%~1"=="" set "elf_file=%~f1"
if not exist "%elf_file%" (
    echo ELF file not found: "%elf_file%" 1>&2
    echo Build the firmware first: scripts\build.cmd build_all 1>&2
    exit /b 1
)
set "openocd_elf_file=%elf_file:\=/%"

rem Относительные пути из .env разрешаются от корня проекта.
pushd "%embedded_dir%"
if errorlevel 1 exit /b 1
"%OPENOCD_EXE%" -s "%OPENOCD_SCRIPTS_DIR%" -f "%OPENOCD_CONFIG%" -c "program {%openocd_elf_file%} verify reset exit"
set "flash_result=%errorlevel%"
popd
exit /b %flash_result%
