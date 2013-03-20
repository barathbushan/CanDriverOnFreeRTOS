################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Source/Examples/NewUART/newuart.c 

OBJS += \
./Source/Examples/NewUART/newuart.o 

C_DEPS += \
./Source/Examples/NewUART/newuart.d 


# Each subdirectory must supply rules for building sources it contributes
Source/Examples/NewUART/%.o: ../Source/Examples/NewUART/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -D__REDLIB__ -DDEBUG -D__CODE_RED -D__USE_CMSIS=CMSISv2p00_LPC17xx -I"C:\Documents and Settings\DESD30\My Documents\LPCXpresso_5.0.14_1109\CAN\FreeRTOS-Products\FreeRTOS-Plus-IO\Device\LPC17xx\SupportedBoards" -I"C:\Documents and Settings\DESD30\My Documents\LPCXpresso_5.0.14_1109\CAN\FreeRTOS-Products\FreeRTOS-Plus-IO\Include" -I"C:\Documents and Settings\DESD30\My Documents\LPCXpresso_5.0.14_1109\CAN\FreeRTOS-Products\FreeRTOS-Plus-CLI" -I"C:\Documents and Settings\DESD30\My Documents\LPCXpresso_5.0.14_1109\CAN\FreeRTOS-Products\FreeRTOS\portable\GCC\ARM_CM3" -I"C:\Documents and Settings\DESD30\My Documents\LPCXpresso_5.0.14_1109\CAN\FreeRTOS-Products\FreeRTOS\include" -I"C:\Documents and Settings\DESD30\My Documents\LPCXpresso_5.0.14_1109\CAN\CMSISv2p00_LPC17xx\inc" -I"C:\Documents and Settings\DESD30\My Documents\LPCXpresso_5.0.14_1109\CAN\FreeRTOS-Plus-Demo-1\Source\Examples\Include" -I"C:\Documents and Settings\DESD30\My Documents\LPCXpresso_5.0.14_1109\CAN\FreeRTOS-Plus-Demo-1\Source" -I"C:\Documents and Settings\DESD30\My Documents\LPCXpresso_5.0.14_1109\CAN\lpc17xx.cmsis.driver.library\Include" -O0 -g3 -Wall -c -fmessage-length=0 -fno-builtin -ffunction-sections -fdata-sections -Wextra -mcpu=cortex-m3 -mthumb -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


