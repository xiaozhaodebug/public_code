# STM32F407 UDS CAN Project Makefile
# Target: STM32F407VG (STM32F40_41xxx)

# Toolchain
PREFIX = $(HOME)/arm-gnu-toolchain/bin/arm-none-eabi-
CC = $(PREFIX)gcc
CXX = $(PREFIX)g++
LD = $(PREFIX)gcc
AR = $(PREFIX)ar
AS = $(PREFIX)as
OBJCOPY = $(PREFIX)objcopy
OBJDUMP = $(PREFIX)objdump
SIZE = $(PREFIX)size

# Project Name
TARGET = uds_can_stm32f4

# Define paths
SRC_DIR = .
BUILD_DIR = build

# CMSIS and Startup
CMSIS_DIR = Startup_config
STARTUP_FILE = $(CMSIS_DIR)/startup_stm32f407xx_gcc.s
SYSTEM_FILE = $(CMSIS_DIR)/system_stm32f4xx.c

# SPL Library
SPL_DIR = STM32F4_FWLIB
SPL_SRC_DIR = $(SPL_DIR)/src
SPL_INC_DIR = $(SPL_DIR)/inc

# Source files
C_SOURCES = \
$(SYSTEM_FILE) \
$(SRC_DIR)/Main/main.c \
$(SRC_DIR)/Main/stm32f4xx_it.c \
$(SRC_DIR)/Common/common.c \
$(SRC_DIR)/USER/CAN/can1.c \
$(SRC_DIR)/USER/CAN/can2.c \
$(SRC_DIR)/USER/KEY/key.c \
$(SRC_DIR)/USER/LED/led.c \
$(SRC_DIR)/USER/LCD/lcd.c \
$(SRC_DIR)/USER/usart1/usart1.c \
$(SRC_DIR)/USER/UDS/iso15765_tp.c \
$(SRC_DIR)/USER/UDS/uds_app.c \
$(SPL_SRC_DIR)/misc.c \
$(SPL_SRC_DIR)/stm32f4xx_gpio.c \
$(SPL_SRC_DIR)/stm32f4xx_rcc.c \
$(SPL_SRC_DIR)/stm32f4xx_usart.c \
$(SPL_SRC_DIR)/stm32f4xx_can.c \
$(SPL_SRC_DIR)/stm32f4xx_flash.c \
$(SPL_SRC_DIR)/stm32f4xx_dma.c \
$(SPL_SRC_DIR)/stm32f4xx_exti.c \
$(SPL_SRC_DIR)/stm32f4xx_syscfg.c \
$(SPL_SRC_DIR)/stm32f4xx_tim.c \
$(SPL_SRC_DIR)/stm32f4xx_pwr.c \
$(SPL_SRC_DIR)/stm32f4xx_i2c.c \
$(SPL_SRC_DIR)/stm32f4xx_spi.c \
$(SPL_SRC_DIR)/stm32f4xx_adc.c \
$(SPL_SRC_DIR)/stm32f4xx_dac.c \
$(SPL_SRC_DIR)/stm32f4xx_crc.c \
$(SPL_SRC_DIR)/stm32f4xx_dbgmcu.c \
$(SPL_SRC_DIR)/stm32f4xx_dcmi.c \
$(SPL_SRC_DIR)/stm32f4xx_fsmc.c \
$(SPL_SRC_DIR)/stm32f4xx_hash.c \
$(SPL_SRC_DIR)/stm32f4xx_cryp.c \
$(SPL_SRC_DIR)/stm32f4xx_rng.c

# ASM sources
ASM_SOURCES = $(STARTUP_FILE)

# Include paths
C_INCLUDES = \
-I$(CMSIS_DIR) \
-I$(SPL_INC_DIR) \
-I$(SRC_DIR)/Main \
-I$(SRC_DIR)/Common \
-I$(SRC_DIR)/USER/CAN \
-I$(SRC_DIR)/USER/KEY \
-I$(SRC_DIR)/USER/LED \
-I$(SRC_DIR)/USER/LCD \
-I$(SRC_DIR)/USER/usart1 \
-I$(SRC_DIR)/USER/UDS

# Defines
DEFS = -DSTM32F40_41xxx -DUSE_STDPERIPH_DRIVER

# MCU flags
MCU = -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard

# Compiler flags
CFLAGS = $(MCU) $(DEFS) $(C_INCLUDES) -O2 -Wall -fdata-sections -ffunction-sections -g
CFLAGS += -D__FPU_USED=1

# Assembler flags
ASFLAGS = $(MCU) $(DEFS)

# Linker flags
LDFLAGS = $(MCU) -specs=nano.specs -T$(SRC_DIR)/STM32F407VGTx_FLASH.ld -lc -lm -lnosys
LDFLAGS += -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref -Wl,--gc-sections

# Generate object list
OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES:.s=.o)))

# VPATH for source files
vpath %.c $(sort $(dir $(C_SOURCES)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))

# Default target
all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Compile C files
$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR)
	$(CC) -c $(CFLAGS) -Wa,-a,-ad,-alms=$(BUILD_DIR)/$(notdir $(<:.c=.lst)) $< -o $@

# Compile ASM files
$(BUILD_DIR)/%.o: %.s Makefile | $(BUILD_DIR)
	$(CC) -x assembler-with-cpp -c $(ASFLAGS) $< -o $@

# Link
$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) Makefile
	$(LD) $(OBJECTS) $(LDFLAGS) -o $@
	$(SIZE) $@

# Generate HEX
$(BUILD_DIR)/$(TARGET).hex: $(BUILD_DIR)/$(TARGET).elf | $(BUILD_DIR)
	$(OBJCOPY) -O ihex $< $@

# Generate BIN
$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf | $(BUILD_DIR)
	$(OBJCOPY) -O binary -S $< $@

# Clean
clean:
	rm -rf $(BUILD_DIR)

# Flash with JLink (requires JLink to be installed)
flash: $(BUILD_DIR)/$(TARGET).bin
	JLinkExe -device STM32F407VG -if SWD -speed 4000 -autoconnect 1 -CommanderScript flash.jlink

# Create JLink script
flash-script:
	@echo "device STM32F407VG" > flash.jlink
	@echo "si 1" >> flash.jlink
	@echo "speed 4000" >> flash.jlink
	@echo "connect" >> flash.jlink
	@echo "r" >> flash.jlink
	@echo "h" >> flash.jlink
	@echo "loadbin $(BUILD_DIR)/$(TARGET).bin, 0x08000000" >> flash.jlink
	@echo "r" >> flash.jlink
	@echo "g" >> flash.jlink
	@echo "exit" >> flash.jlink

.PHONY: all clean flash flash-script
