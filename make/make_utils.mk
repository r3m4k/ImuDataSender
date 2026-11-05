# ---------------------------
# Вспомогательные правила сборки
# ---------------------------

ifeq ($(SYSTEM), windows)
SHELL := cmd.exe
.SHELLFLAGS := /D /C
create_dir = if not exist "$(subst /,\,$(1))" mkdir "$(subst /,\,$(1))"
delete_files = for %f in ($(foreach file,$(1),"$(subst /,\,$(file))")) do @if exist "%~f" del /f /q "%~f"
else ifeq ($(SYSTEM), linux)
create_dir = mkdir -p "$(1)"
delete_files = rm -f $(foreach file,$(1),"$(file)")
endif

.PHONY: __info __linking __create_dirs __create_bin_dir __create_subdirs

__info:
	@echo # ---------------------------
	@echo # Building information
	@echo # ---------------------------
	@echo DEFINES: ${DEFINES}
	@echo User_Defines: ${USER_DEFINES}
	@echo Includes: ${INCLUDES}
	@echo GCC_FLAGS: ${GCC_FLAGS}
	@echo GPP_FLAGS: ${GPP_FLAGS}
	@echo LINK_FLAGS: ${LINK_FLAGS}
	@echo LIBS: ${LIBS}
	@echo Obj dirs: ${SUBDIRS_OBJ}
	@echo # ---------------------------

# --------------------------------
# Линковка и формирование HEX
# --------------------------------

__linking: ${BIN_PLACE}/${PROGRAM_NAME}.hex

${BIN_PLACE}/${BINARY}: | ${BIN_PLACE}
	@echo Building "$@", map "${BIN_PLACE}/${PROGRAM_NAME}.map"
	@"${CC}" ${LINK_FLAGS} -o "$@" ${OBJECTS} ${LIBS}
	@"${GCC_PLACE}/bin/arm-none-eabi-size" --format=berkeley "$@"

${BIN_PLACE}/${PROGRAM_NAME}.hex: ${BIN_PLACE}/${BINARY}
	@echo Forming "$@"
	@"${GCC_PLACE}/bin/arm-none-eabi-objcopy" -O ihex "$<" "$@"

# --------------------------------
# Создание необходимых каталогов
# --------------------------------

__create_dirs: __create_bin_dir __create_subdirs

__create_subdirs:

__create_bin_dir: ${BIN_PLACE}

${BIN_PLACE}:
	@$(call create_dir,$@)
