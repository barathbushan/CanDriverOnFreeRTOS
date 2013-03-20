################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Source/debug_frmwrk.c \
../Source/lpc17xx_adc.c \
../Source/lpc17xx_can.c \
../Source/lpc17xx_clkpwr.c \
../Source/lpc17xx_dac.c \
../Source/lpc17xx_emac.c \
../Source/lpc17xx_exti.c \
../Source/lpc17xx_gpdma.c \
../Source/lpc17xx_gpio.c \
../Source/lpc17xx_i2c.c \
../Source/lpc17xx_i2s.c \
../Source/lpc17xx_libcfg_default.c \
../Source/lpc17xx_mcpwm.c \
../Source/lpc17xx_nvic.c \
../Source/lpc17xx_pinsel.c \
../Source/lpc17xx_pwm.c \
../Source/lpc17xx_qei.c \
../Source/lpc17xx_rit.c \
../Source/lpc17xx_rtc.c \
../Source/lpc17xx_spi.c \
../Source/lpc17xx_ssp.c \
../Source/lpc17xx_systick.c \
../Source/lpc17xx_timer.c \
../Source/lpc17xx_uart.c \
../Source/lpc17xx_wdt.c 

OBJS += \
./Source/debug_frmwrk.o \
./Source/lpc17xx_adc.o \
./Source/lpc17xx_can.o \
./Source/lpc17xx_clkpwr.o \
./Source/lpc17xx_dac.o \
./Source/lpc17xx_emac.o \
./Source/lpc17xx_exti.o \
./Source/lpc17xx_gpdma.o \
./Source/lpc17xx_gpio.o \
./Source/lpc17xx_i2c.o \
./Source/lpc17xx_i2s.o \
./Source/lpc17xx_libcfg_default.o \
./Source/lpc17xx_mcpwm.o \
./Source/lpc17xx_nvic.o \
./Source/lpc17xx_pinsel.o \
./Source/lpc17xx_pwm.o \
./Source/lpc17xx_qei.o \
./Source/lpc17xx_rit.o \
./Source/lpc17xx_rtc.o \
./Source/lpc17xx_spi.o \
./Source/lpc17xx_ssp.o \
./Source/lpc17xx_systick.o \
./Source/lpc17xx_timer.o \
./Source/lpc17xx_uart.o \
./Source/lpc17xx_wdt.o 

C_DEPS += \
./Source/debug_frmwrk.d \
./Source/lpc17xx_adc.d \
./Source/lpc17xx_can.d \
./Source/lpc17xx_clkpwr.d \
./Source/lpc17xx_dac.d \
./Source/lpc17xx_emac.d \
./Source/lpc17xx_exti.d \
./Source/lpc17xx_gpdma.d \
./Source/lpc17xx_gpio.d \
./Source/lpc17xx_i2c.d \
./Source/lpc17xx_i2s.d \
./Source/lpc17xx_libcfg_default.d \
./Source/lpc17xx_mcpwm.d \
./Source/lpc17xx_nvic.d \
./Source/lpc17xx_pinsel.d \
./Source/lpc17xx_pwm.d \
./Source/lpc17xx_qei.d \
./Source/lpc17xx_rit.d \
./Source/lpc17xx_rtc.d \
./Source/lpc17xx_spi.d \
./Source/lpc17xx_ssp.d \
./Source/lpc17xx_systick.d \
./Source/lpc17xx_timer.d \
./Source/lpc17xx_uart.d \
./Source/lpc17xx_wdt.d 


# Each subdirectory must supply rules for building sources it contributes
Source/%.o: ../Source/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -D__REDLIB__ -DDEBUG -D__CODE_RED -I"C:\Documents and Settings\DESD30\My Documents\LPCXpresso_5.0.14_1109\sat_irq\FreeRTOS-Plus-Demo-1\Source" -I"C:\Documents and Settings\DESD30\My Documents\LPCXpresso_5.0.14_1109\sat_irq\FreeRTOS-Products\FreeRTOS\include" -I"C:\Documents and Settings\DESD30\My Documents\LPCXpresso_5.0.14_1109\sat_irq\FreeRTOS-Products\FreeRTOS\portable\GCC\ARM_CM3" -I"C:\Documents and Settings\DESD30\My Documents\LPCXpresso_5.0.14_1109\sat_irq\lpc17xx.cmsis.driver.library\Include" -I"C:\Documents and Settings\DESD30\My Documents\LPCXpresso_5.0.14_1109\sat_irq\CMSISv2p00_LPC17xx\inc" -O0 -g3 -Wall -c -fmessage-length=0 -fno-builtin -ffunction-sections -fdata-sections -mcpu=cortex-m3 -mthumb -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


