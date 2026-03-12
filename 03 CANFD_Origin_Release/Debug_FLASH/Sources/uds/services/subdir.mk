################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Sources/uds/services/uds_svc_10.c \
../Sources/uds/services/uds_svc_22.c \
../Sources/uds/services/uds_svc_27.c \
../Sources/uds/services/uds_svc_31.c 

OBJS += \
./Sources/uds/services/uds_svc_10.o \
./Sources/uds/services/uds_svc_22.o \
./Sources/uds/services/uds_svc_27.o \
./Sources/uds/services/uds_svc_31.o 

C_DEPS += \
./Sources/uds/services/uds_svc_10.d \
./Sources/uds/services/uds_svc_22.d \
./Sources/uds/services/uds_svc_27.d \
./Sources/uds/services/uds_svc_31.d 


# Each subdirectory must supply rules for building sources it contributes
Sources/uds/services/%.o: ../Sources/uds/services/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Standard S32DS C Compiler'
	arm-none-eabi-gcc "@Sources/uds/services/uds_svc_10.args" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


