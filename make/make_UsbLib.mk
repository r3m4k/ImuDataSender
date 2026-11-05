.PHONY: __build_stm32f3x_usb_lib __rebuild_stm_usb_lib __clean_usb_lib

# ---------------------------
# Компоненты USB
# ---------------------------

OBJECTS += ${USB_OBJ}

SUBDIRS_OBJ += ${USB_OBJ_DIR}

# ---------------------------
# Исходные файлы
# ---------------------------

USB_SRC_DIR = ${SOURCE_DIR}/system/USB_LIB/src

USB_SRC_C = \
${USB_SRC_DIR}/hw_config.c \
${USB_SRC_DIR}/usb_istr.c \
${USB_SRC_DIR}/usb_pwr.c \
${USB_SRC_DIR}/usb_init.c \
${USB_SRC_DIR}/usb_prop.c \
${USB_SRC_DIR}/usb_regs.c \
${USB_SRC_DIR}/usb_desc.c \
${USB_SRC_DIR}/usb_sil.c \
${USB_SRC_DIR}/usb_core.c \
${USB_SRC_DIR}/usb_int.c \
${USB_SRC_DIR}/usb_mem.c \
${USB_SRC_DIR}/usb_endp.c

USB_SRC_CPP = \
${USB_SRC_DIR}/VCP_F3.cpp

# ---------------------------
# Объектные файлы
# ---------------------------
USB_OBJ_DIR = ${BIN_PLACE}/usb

USB_OBJ := \
$(patsubst ${USB_SRC_DIR}/%.c, ${USB_OBJ_DIR}/%.o,${USB_SRC_C}) \
$(patsubst ${USB_SRC_DIR}/%.cpp,${USB_OBJ_DIR}/%.opp,${USB_SRC_CPP})


# --------------------------------
# Правила для сборки STM32_USB_LIB
# --------------------------------

__rebuild_stm_usb_lib: __clean_usb_lib
	+@"$(MAKE)" -f "$(MAKEFILE_ENTRY)" __build_stm32f3x_usb_lib
	@echo # ---------------------------
	@echo Rebuilding USB objects completed successfully
	@echo # ---------------------------

# ---------------------------

__build_stm32f3x_usb_lib: ${USB_OBJ}

# ---------------------------
${USB_OBJ_DIR}/%.o: ${USB_SRC_DIR}/%.c | ${USB_OBJ_DIR}
	@echo Compiling $@ from $<
	@"${CC}" ${GCC_FLAGS} ${DEPFLAGS} ${DEFINES} ${INCLUDES} "$<" -o "$@"

${USB_OBJ_DIR}/%.opp: ${USB_SRC_DIR}/%.cpp | ${USB_OBJ_DIR}
	@echo Compiling $@ from $<
	@"${CP}" ${GPP_FLAGS} ${DEPFLAGS} ${DEFINES} ${INCLUDES} "$<" -o "$@"

# ---------------------------

__clean_usb_lib:
	@echo # ---------------------------
	@echo Deleting USB objects and dependencies
	@$(call delete_files,${USB_OBJ} $(addsuffix .d,${USB_OBJ}))
	@echo # ---------------------------

# ---------------------------
