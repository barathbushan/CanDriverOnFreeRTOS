################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include Source/lwIP/netif/subdir.mk
-include Source/lwIP/lwIP_Apps/apps/httpserver_raw/subdir.mk
-include Source/lwIP/lwIP_Apps/apps/BasicSocketCommandServer/subdir.mk
-include Source/lwIP/lwIP_Apps/subdir.mk
-include Source/lwIP/core/ipv4/subdir.mk
-include Source/lwIP/core/subdir.mk
-include Source/lwIP/api/subdir.mk
-include Source/MemMang/subdir.mk
-include Source/FreeRTOS-Products/FreeRTOS/portable/GCC/ARM_CM3/subdir.mk
-include Source/FreeRTOS-Products/FreeRTOS-Plus-IO/Device/LPC17xx/subdir.mk
-include Source/FreeRTOS-Products/FreeRTOS-Plus-IO/Common/subdir.mk
-include Source/FreeRTOS-Products/FreeRTOS-Plus-CLI/subdir.mk
-include Source/FreeRTOS-Products/FreeRTOS/subdir.mk
-include Source/FatFS/subdir.mk
-include Source/Examples/SSP/subdir.mk
-include Source/Examples/GPIO/subdir.mk
-include Source/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: FreeRTOS-Plus-Demo-2.axf

# Tool invocations
FreeRTOS-Plus-Demo-2.axf: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: MCU Linker'
	arm-none-eabi-gcc -nostdlib -L"C:\Documents and Settings\DESD30\My Documents\LPCXpresso_5.0.14_1109\irq\lpc17xx.cmsis.driver.library\Debug" -L"C:\Documents and Settings\DESD30\My Documents\LPCXpresso_5.0.14_1109\irq\CMSISv2p00_LPC17xx\Debug" -Xlinker -Map=FreeRTOS-Plus-Demo-2.map -Xlinker --gc-sections -mcpu=cortex-m3 -mthumb -T "FreeRTOS-Plus-Demo-2_Debug.ld" -o "FreeRTOS-Plus-Demo-2.axf" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '
	$(MAKE) --no-print-directory post-build

# Other Targets
clean:
	-$(RM) $(OBJS)$(C_DEPS)$(EXECUTABLES) FreeRTOS-Plus-Demo-2.axf
	-@echo ' '

post-build:
	-@echo 'Performing post-build steps'
	-arm-none-eabi-size FreeRTOS-Plus-Demo-2.axf; # arm-none-eabi-objcopy -O binary FreeRTOS-Plus-Demo-2.axf FreeRTOS-Plus-Demo-2.bin ; checksum -p LPC1769 -d FreeRTOS-Plus-Demo-2.bin;
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY: post-build

-include ../makefile.targets
