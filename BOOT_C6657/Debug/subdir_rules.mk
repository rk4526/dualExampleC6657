################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
%.obj: ../%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: C6000 Compiler'
	"C:/ti/ccs930/ccs/tools/compiler/ti-cgt-c6000_8.3.5/bin/cl6x" -mv6600 --include_path="C:/Users/eeric/workspace_v9/dual_core/BOOT_C6657" --include_path="C:/ti/pdk_c665x_2_0_16/packages" --include_path="C:/ti/pdk_c665x_2_0_16/packages/ti/platform" --include_path="C:/ti/ccs930/ccs/tools/compiler/ti-cgt-c6000_8.3.5/include" --define=__TI_NO_PARALLEL_LOADS=1 --define=SOC_C6657 -g --diag_warning=225 --diag_wrap=off --display_error_number --preproc_with_compile --preproc_dependency="$(basename $(<F)).d_raw" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


