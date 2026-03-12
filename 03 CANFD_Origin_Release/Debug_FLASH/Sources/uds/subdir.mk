################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Sources/uds/uds_core.c \
../Sources/uds/uds_nrc.c \
../Sources/uds/uds_pending.c \
../Sources/uds/uds_routine.c \
../Sources/uds/uds_security.c \
../Sources/uds/uds_session.c \
../Sources/uds/uds_tp.c \
../Sources/uds/uds_user_config_example.c 

OBJS += \
./Sources/uds/uds_core.o \
./Sources/uds/uds_nrc.o \
./Sources/uds/uds_pending.o \
./Sources/uds/uds_routine.o \
./Sources/uds/uds_security.o \
./Sources/uds/uds_session.o \
./Sources/uds/uds_tp.o \
./Sources/uds/uds_user_config_example.o 

C_DEPS += \
./Sources/uds/uds_core.d \
./Sources/uds/uds_nrc.d \
./Sources/uds/uds_pending.d \
./Sources/uds/uds_routine.d \
./Sources/uds/uds_security.d \
./Sources/uds/uds_session.d \
./Sources/uds/uds_tp.d \
./Sources/uds/uds_user_config_example.d 


# Each subdirectory must supply rules for building sources it contributes
Sources/uds/%.o: ../Sources/uds/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Standard S32DS C Compiler'
	arm-none-eabi-gcc "@Sources/uds/uds_core.args" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


