# ---------------------------
# Makefile
# ---------------------------

MAKEFILE_ENTRY := $(lastword $(MAKEFILE_LIST))
MAKE_DIR := $(dir $(MAKEFILE_ENTRY))

include $(MAKE_DIR)make_args.mk
include $(MAKE_DIR)make_utils.mk
include $(MAKE_DIR)make_StmLib.mk
include $(MAKE_DIR)make_UsbLib.mk
include $(MAKE_DIR)make_user.mk

# Эти зависимости задаются после формирования полного списка объектов.
${BIN_PLACE}/${BINARY}: ${OBJECTS} ${LINKER_SCRIPTS}

# Отсутствующий .d требует перекомпиляции объекта для восстановления зависимостей.
DEPFILES := $(addsuffix .d,${OBJECTS})
${OBJECTS}: %: %.d
${DEPFILES}:

-include $(wildcard ${DEPFILES})

# При ошибке рецепта Make удаляет изменённый целевой файл.
.DELETE_ON_ERROR:

.PHONY: build_all rebuild_all clean_all build_libs rebuild_lib clean_libs \
        build_user rebuild_user clean_user

# Время изменения каталогов не влияет на актуальность объектных файлов.
__create_subdirs: ${SUBDIRS_OBJ}

$(sort ${SUBDIRS_OBJ}): | ${BIN_PLACE}
	@$(call create_dir,$@)

# ---------------------------
# Общие правила сборки
# ---------------------------

build_all: build_libs build_user
	@echo # ---------------------------
	@echo The project building is completed
	@echo # ---------------------------

rebuild_all: clean_all
	+@"$(MAKE)" -f "$(MAKEFILE_ENTRY)" build_all

clean_all: clean_user clean_libs
	@echo The project has been cleared

# ---------------------------
# Правила сборки системных и USB-компонентов
# ---------------------------

build_libs: __build_stm32f3x_spl_lib __build_stm32f3x_usb_lib
	@echo # ---------------------------
	@echo Building system and USB objects is completed
	@echo # ---------------------------

rebuild_lib: clean_libs
	+@"$(MAKE)" -f "$(MAKEFILE_ENTRY)" build_libs

clean_libs: __clean_stm_std_lib __clean_usb_lib

# ---------------------------------
# Правила сборки файлов пользователя
# ---------------------------------

build_user: __linking

rebuild_user: clean_user
	+@"$(MAKE)" -f "$(MAKEFILE_ENTRY)" build_user

clean_user: __clean_user

# ---------------------------
