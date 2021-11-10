
OUT_DIR += /source

OBJS += \
$(OUT_PATH)/source/utils.o \
$(OUT_PATH)/source/app.o \
$(OUT_PATH)/source/app_att.o \
$(OUT_PATH)/source/battery.o \
$(OUT_PATH)/source/cmd_parser.o \
$(OUT_PATH)/source/ble.o \
$(OUT_PATH)/source/main.o


# Each subdirectory must supply rules for building sources it contributes
$(OUT_PATH)/source/%.o: $(PROJECT_PATH)/%.c
	@echo 'Building file: $<'
	@tc32-elf-gcc $(GCC_FLAGS) $(INCLUDE_PATHS) -c -o"$@" "$<"