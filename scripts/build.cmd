@echo off
setlocal EnableExtensions DisableDelayedExpansion

rem Настройки читаются как пары ИМЯ=значение, без выполнения команд из .env.
for %%I in ("%~dp0..") do set "embedded_dir=%%~fI"
set "env_file=%embedded_dir%\.env"

if "%~1"=="" goto usage
set "target=%~1"
set "valid_target="
for %%T in (build_all rebuild_all clean_all build_libs rebuild_lib clean_libs build_user rebuild_user clean_user) do if "%target%"=="%%T" set "valid_target=1"
if not defined valid_target goto usage

if not exist "%env_file%" (
    echo Local configuration not found: "%env_file%" 1>&2
    echo Copy .env.example to .env and configure the tool paths. 1>&2
    exit /b 1
)

for %%V in (SYSTEM PROGRAM_NAME BUILD_DIR GCC_PLACE MAKE_COMMAND) do set "%%V="
for /f "usebackq eol=# tokens=1,* delims==" %%A in ("%env_file%") do (
    if "%%A"=="SYSTEM" set "SYSTEM=%%~B"
    if "%%A"=="PROGRAM_NAME" set "PROGRAM_NAME=%%~B"
    if "%%A"=="BUILD_DIR" set "BUILD_DIR=%%~B"
    if "%%A"=="GCC_PLACE" set "GCC_PLACE=%%~B"
    if "%%A"=="MAKE_COMMAND" set "MAKE_COMMAND=%%~B"
)

for %%V in (SYSTEM PROGRAM_NAME BUILD_DIR GCC_PLACE) do if not defined %%V (
    echo Missing configuration value: %%V 1>&2
    exit /b 1
)
if /i not "%SYSTEM%"=="windows" (
    echo Native cmd scripts require SYSTEM=windows. 1>&2
    exit /b 1
)
if not defined MAKE_COMMAND set "MAKE_COMMAND=make"
set "GCC_PLACE=%GCC_PLACE:\=/%"
set "BUILD_DIR=%BUILD_DIR:\=/%"

rem Цель обязательна; остальные аргументы передаются Make, например -j4.
pushd "%embedded_dir%"
if errorlevel 1 exit /b 1
"%MAKE_COMMAND%" -C "%embedded_dir%" -f make/Makefile.mk %* "SYSTEM=windows" "PROGRAM_NAME=%PROGRAM_NAME%" "SOURCE_DIR=." "GCC_PLACE=%GCC_PLACE%" "BIN_PLACE=./%BUILD_DIR%"
set "build_result=%errorlevel%"
popd
exit /b %build_result%

:usage
echo Usage: scripts\build.cmd TARGET [MAKE_OPTIONS] 1>&2
echo Targets: build_all rebuild_all clean_all build_libs rebuild_lib clean_libs build_user rebuild_user clean_user 1>&2
exit /b 2
