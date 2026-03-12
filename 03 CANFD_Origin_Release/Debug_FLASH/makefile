################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include Sources/uds/services/subdir.mk
-include Sources/uds/subdir.mk
-include Sources/subdir.mk
-include SDK/rtos/osif/subdir.mk
-include SDK/platform/pal/src/can/subdir.mk
-include SDK/platform/drivers/src/pins/subdir.mk
-include SDK/platform/drivers/src/lpuart/subdir.mk
-include SDK/platform/drivers/src/lpit/subdir.mk
-include SDK/platform/drivers/src/interrupt/subdir.mk
-include SDK/platform/drivers/src/flexcan/subdir.mk
-include SDK/platform/drivers/src/edma/subdir.mk
-include SDK/platform/drivers/src/clock/S32K1xx/subdir.mk
-include SDK/platform/devices/S32K144/startup/subdir.mk
-include SDK/platform/devices/subdir.mk
-include Project_Settings/Startup_Code/subdir.mk
-include Generated_Code/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
SECONDARY_FLASH += \
s32k144_Demo_CAN.srec \

SECONDARY_SIZE += \
s32k144_Demo_CAN.siz \


# All Target
all: s32k144_Demo_CAN.elf secondary-outputs

# Tool invocations
s32k144_Demo_CAN.elf: $(OBJS) D:/a_blog/08\ repo/AI/S32k/03\ CANFD_Origin_Release/Project_Settings/Linker_Files/S32K144_64_flash.ld $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: Standard S32DS C Linker'
	arm-none-eabi-gcc -o "s32k144_Demo_CAN.elf" "@s32k144_Demo_CAN.args"  $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

s32k144_Demo_CAN.srec: s32k144_Demo_CAN.elf
	@echo 'Invoking: Standard S32DS Create Flash Image'
	arm-none-eabi-objcopy -O srec s32k144_Demo_CAN.elf  "s32k144_Demo_CAN.srec"
	@echo 'Finished building: $@'
	@echo ' '

s32k144_Demo_CAN.siz: s32k144_Demo_CAN.elf
	@echo 'Invoking: Standard S32DS Print Size'
	arm-none-eabi-size --format=berkeley s32k144_Demo_CAN.elf
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) ./*
	-@echo ' '

secondary-outputs: $(SECONDARY_FLASH) $(SECONDARY_SIZE)

.PHONY: all clean dependents

-include ../makefile.targets
