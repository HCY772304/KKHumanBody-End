################################################################################
# Automatically-generated file. Do not edit!
################################################################################

export WINEDEBUG=-all

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include zcl-framework-core/subdir.mk
-include update-tc-link-key/subdir.mk
-include strong-random/subdir.mk
-include simple-main/subdir.mk
-include serial/EFR32/subdir.mk
-include serial/subdir.mk
-include scan-dispatch/subdir.mk
-include reporting/subdir.mk
-include packet-handoff/subdir.mk
-include ota-storage-simple-eeprom/subdir.mk
-include ota-storage-simple/subdir.mk
-include ota-storage-common/subdir.mk
-include ota-common/subdir.mk
-include ota-client-policy/subdir.mk
-include ota-client/subdir.mk
-include ota-bootload/subdir.mk
-include nvm3/EFR32/subdir.mk
-include nvm3/subdir.mk
-include network-steering/subdir.mk
-include mbedtls/EFR32/subdir.mk
-include idle-sleep/subdir.mk
-include identify/subdir.mk
-include ias-zone-server/subdir.mk
-include heartbeat/subdir.mk
-include hal-library/EFR32_2/subdir.mk
-include hal-library/EFR32/subdir.mk
-include generic-interrupt-control/EFR32/subdir.mk
-include form-and-join/subdir.mk
-include external-generated-files/subdir.mk
-include end-device-support/subdir.mk
-include emlib/subdir.mk
-include emdrv/subdir.mk
-include ember-minimal-printf/subdir.mk
-include efr32/subdir.mk
-include eeprom/subdir.mk
-include diagnostic-server/subdir.mk
-include counters/subdir.mk
-include coexistence-stub/EFR32/subdir.mk
-include ccm-encryption/subdir.mk
-include basic/subdir.mk
-include antenna-stub/subdir.mk
-include address-table/subdir.mk
-include 00ikonke-app/tools/subdir.mk
-include 00ikonke-app/general/subdir.mk
-include 00ikonke-app/driver/subdir.mk
-include 00ikonke-app/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(ASM_UPPER_DEPS)),)
-include $(ASM_UPPER_DEPS)
endif
ifneq ($(strip $(MSA_UPPER_DEPS)),)
-include $(MSA_UPPER_DEPS)
endif
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif
ifneq ($(strip $(S79_DEPS)),)
-include $(S79_DEPS)
endif
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(MSA_DEPS)),)
-include $(MSA_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
ifneq ($(strip $(S79_UPPER_DEPS)),)
-include $(S79_UPPER_DEPS)
endif
endif

-include ../makefile.defs

OPTIONAL_TOOL_DEPS := \
$(wildcard ../makefile.defs) \
$(wildcard ../makefile.init) \
$(wildcard ../makefile.targets) \


BUILD_ARTIFACT_NAME := Z3SwitchSoc
BUILD_ARTIFACT_EXTENSION := out
BUILD_ARTIFACT_PREFIX :=
BUILD_ARTIFACT := $(BUILD_ARTIFACT_PREFIX)$(BUILD_ARTIFACT_NAME)$(if $(BUILD_ARTIFACT_EXTENSION),.$(BUILD_ARTIFACT_EXTENSION),)

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all:
	+@$(MAKE) --no-print-directory pre-build && $(MAKE) --no-print-directory main-build && $(MAKE) --no-print-directory post-build

# Main-build Target
main-build: Z3SwitchSoc.out

# Tool invocations
Z3SwitchSoc.out: $(OBJS) $(USER_OBJS) makefile objects.mk $(OPTIONAL_TOOL_DEPS)
	@echo 'Building target: $@'
	@echo 'Invoking: IAR Linker for ARM'
	ilinkarm $(LIBS) $(OBJS) $(USER_OBJS) -o "Z3SwitchSoc.out" --config "D:/simplicity studio5/s5/developer/sdks/gecko_sdk_suite/v3.1//platform/base/hal/micro/cortexm3/efm32/iar-cfg.icf" --config_def APP_BTL=1 --config_def FLASH_SIZE=786432 --config_def RAM_SIZE=65536 --config_def SIMEEPROM_SIZE=65536 --config_def LOCKBITS_IN_MAINFLASH_SIZE=8192 --config_def EMBER_MALLOC_HEAP_SIZE=0 --config_def HEADER_SIZE=512 --config_def PSSTORE_SIZE=0 --config_def LONGTOKEN_SIZE=0 --config_def BTL_SIZE=16384 --no_wrap_diagnostics --entry __iar_program_start --map "D:/simplicity studio5/s5/workspaces/Z3SwitchSoc/IAR ARM - Default/Z3SwitchSoc.map" --log_file "D:/simplicity studio5/s5/workspaces/Z3SwitchSoc/IAR ARM - Default/Z3SwitchSoc.log" --log initialization,modules,sections,veneers --diag_suppress=Lp012 --redirect _Printf=_PrintfSmallNoMb --redirect _Scanf=_ScanfFullNoMb  
	ielftool --srec --verbose "Z3SwitchSoc.out" "Z3SwitchSoc.s37"  
	ielftool --ihex --verbose "Z3SwitchSoc.out" "Z3SwitchSoc.hex"  
	ielftool --bin --verbose "Z3SwitchSoc.out" "Z3SwitchSoc.bin"
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(ASM_UPPER_DEPS)$(MSA_UPPER_DEPS)$(EXECUTABLES)$(OBJS)$(ASM_DEPS)$(S79_DEPS)$(S_DEPS)$(S_UPPER_DEPS)$(MSA_DEPS)$(C_DEPS)$(S79_UPPER_DEPS) Z3SwitchSoc.out
	-@echo ' '

pre-build:
	-cmd //c 'D:\SIMPLI~1\s5\WORKSP~1\Z3SWIT~1/KKTemplateCodeProject-prebuild.bat' 'Z3SwitchSoc' 'D:/simplicity studio5/s5/workspaces/Z3SwitchSoc' 'D:/IAR7_8/arm' 'D:\simplicity studio5\s5'
	-@echo ' '

post-build:
	-cmd //c 'D:\SIMPLI~1\s5\WORKSP~1\Z3SWIT~1/KKTemplateCodeProject-postbuild.bat' 'Z3SwitchSoc' 'D:/simplicity studio5/s5/workspaces/Z3SwitchSoc' 'D:/IAR7_8/arm' 'D:\simplicity studio5\s5'
	-@echo ' '

.PHONY: all clean dependents main-build pre-build post-build

-include ../makefile.targets
