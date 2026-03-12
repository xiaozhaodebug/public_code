################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Sources/can_fd_wrapper.c \
../Sources/delay.c \
../Sources/key.c \
../Sources/main.c \
../Sources/sys_cfg.c \
../Sources/uart.c 

OBJS += \
./Sources/can_fd_wrapper.o \
./Sources/delay.o \
./Sources/key.o \
./Sources/main.o \
./Sources/sys_cfg.o \
./Sources/uart.o 

C_DEPS += \
./Sources/can_fd_wrapper.d \
./Sources/delay.d \
./Sources/key.d \
./Sources/main.d \
./Sources/sys_cfg.d \
./Sources/uart.d 


# Each subdirectory must supply rules for building sources it contributes
Sources/%.o: ../Sources/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Standard S32DS C Compiler'
	arm-none-eabi-gcc "@Sources/can_fd_wrapper.args" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


