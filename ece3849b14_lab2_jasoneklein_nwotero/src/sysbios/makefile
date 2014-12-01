
XOPTS = -I"C:/ti/xdctools_3_25_00_48/packages/" -Dxdc_target_types__=C:/ti/xdctools_3_25_00_48/packages/ti/targets/arm/elf/std.h -Dxdc_target_name__=M3

vpath % C:/ti/bios_6_35_01_29/packages/ti/sysbios/
vpath %.c C:/ti/xdctools_3_25_00_48/packages/

CCOPTS    = --endian=little -mv7M3 --abi=eabi -q -ms --opt_for_speed=2  --program_level_compile -o3 -g --optimize_with_debug 

XDC_ROOT  = C:/ti/xdctools_3_25_00_48/packages/

BIOS_ROOT = C:/ti/bios_6_35_01_29/packages/ti/sysbios/

BIOS_DEFS =  -Dti_sysbios_BIOS_swiEnabled__D=TRUE -Dti_sysbios_BIOS_taskEnabled__D=TRUE -Dti_sysbios_BIOS_clockEnabled__D=TRUE -Dti_sysbios_hal_Hwi_DISABLE_ALL_HOOKS -Dti_sysbios_knl_Swi_DISABLE_ALL_HOOKS -Dti_sysbios_knl_Swi_numPriorities__D=16 -Dti_sysbios_BIOS_smpEnabled__D=FALSE -Dti_sysbios_Build_useHwiMacros -Dti_sysbios_knl_Task_deleteTerminatedTasks__D=FALSE -Dti_sysbios_knl_Task_numPriorities__D=16 -Dti_sysbios_knl_Clock_TICK_SOURCE=ti_sysbios_knl_Clock_TickSource_TIMER -Dti_sysbios_knl_Clock_TICK_MODE=ti_sysbios_knl_Clock_TickMode_PERIODIC -Dti_sysbios_utils_Load_taskEnabled__D=TRUE -Dti_sysbios_utils_Load_swiEnabled__D=FALSE -Dti_sysbios_family_arm_m3_Hwi_dispatcherSwiSupport__D=TRUE -Dti_sysbios_family_arm_m3_Hwi_dispatcherTaskSupport__D=TRUE -Dti_sysbios_family_arm_m3_Hwi_dispatcherAutoNestingSupport__D=TRUE -Dti_sysbios_family_arm_m3_Hwi_dispatcherIrpTrackingSupport__D=TRUE -Dti_sysbios_knl_Semaphore_supportsEvents__D=FALSE -Dti_sysbios_family_arm_m3_Hwi_enableException__D=TRUE -Dti_sysbios_family_arm_m3_Hwi_disablePriority__D=32U

BIOS_INC  = -I"C:/ti/bios_6_35_01_29/packages/"

CC = C:/ti/ccsv5/tools/compiler/arm_5.0.4/bin/armcl -c $(CCOPTS) -I C:/ti/ccsv5/tools/compiler/arm_5.0.4/include
ASM = C:/ti/ccsv5/tools/compiler/arm_5.0.4/bin/armcl -c $(CCOPTS) -I C:/ti/ccsv5/tools/compiler/arm_5.0.4/include
AR = C:/ti/ccsv5/tools/compiler/arm_5.0.4/bin/armar rq

DEL = cmd.exe /q /c del /f
CP = cmd.exe /q /c copy /Y

define RM
	$(if $(wildcard $1),$(DEL) $1,:)
endef

define ASSEMBLE
	@echo asmem3 $< ...
	@$(ASM) $(BIOS_DEFS) $(XOPTS) $(BIOS_INC) $<
endef

all: sysbios.aem3

m3_Clobber_asm.obj: family/arm/m3/Clobber_asm.sv7M makefile
	@-$(call RM, $@)
	$(ASSEMBLE) --output_file=m3_Clobber_asm.obj

m3_IntrinsicsSupport_asm.obj: family/arm/m3/IntrinsicsSupport_asm.sv7M makefile
	@-$(call RM, $@)
	$(ASSEMBLE) --output_file=m3_IntrinsicsSupport_asm.obj

m3_TaskSupport_asm.obj: family/arm/m3/TaskSupport_asm.sv7M makefile
	@-$(call RM, $@)
	$(ASSEMBLE) --output_file=m3_TaskSupport_asm.obj

m3_Hwi_asm.obj: family/arm/m3/Hwi_asm.sv7M makefile
	@-$(call RM, $@)
	$(ASSEMBLE) --output_file=m3_Hwi_asm.obj

m3_Hwi_asm_switch.obj: family/arm/m3/Hwi_asm_switch.sv7M makefile
	@-$(call RM, $@)
	$(ASSEMBLE) --output_file=m3_Hwi_asm_switch.obj

ducati_CTM_asm.obj: family/arm/ducati/CTM_asm.sv7M makefile
	@-$(call RM, $@)
	$(ASSEMBLE) --output_file=ducati_CTM_asm.obj

ducati_Core_asm.obj: family/arm/ducati/Core_asm.sv7M makefile
	@-$(call RM, $@)
	$(ASSEMBLE) --output_file=ducati_Core_asm.obj

omap4430_Power_saveCpu.obj: family/arm/ducati/omap4430/Power_saveCpu.sv7M makefile
	@-$(call RM, $@)
	$(ASSEMBLE) --output_file=omap4430_Power_saveCpu.obj

omap4430_Power_resumeCpu.obj: family/arm/ducati/omap4430/Power_resumeCpu.sv7M makefile
	@-$(call RM, $@)
	$(ASSEMBLE) --output_file=omap4430_Power_resumeCpu.obj

smp_Power_saveCpu.obj: family/arm/ducati/smp/Power_saveCpu.sv7M makefile
	@-$(call RM, $@)
	$(ASSEMBLE) --output_file=smp_Power_saveCpu.obj

smp_Power_resumeCpu.obj: family/arm/ducati/smp/Power_resumeCpu.sv7M makefile
	@-$(call RM, $@)
	$(ASSEMBLE) --output_file=smp_Power_resumeCpu.obj

dmtimer_Timer_asm.obj: timers/dmtimer/Timer_asm.sv7M makefile
	@-$(call RM, $@)
	$(ASSEMBLE) --output_file=dmtimer_Timer_asm.obj


BIOS.obj: BIOS.c hal/Hwi.c hal/Hwi_stack.c hal/Hwi_startup.c hal/Timer.c knl/Clock.c knl/Idle.c knl/Intrinsics.c knl/Queue.c knl/Semaphore.c knl/Swi.c knl/Swi_andn.c knl/Task.c family/arm/m3/Hwi.c family/arm/m3/TaskSupport.c family/arm/lm3/TimestampProvider.c family/arm/lm3/Timer.c gates/GateHwi.c gates/GateMutex.c heaps/HeapMem.c makefile
	@-$(call RM, $@)
	@echo clem3 $< ...
	@$(CC) $(BIOS_DEFS) $(XOPTS) $(BIOS_INC) \
	$(BIOS_ROOT)BIOS.c \
	$(BIOS_ROOT)hal/Hwi.c \
	$(BIOS_ROOT)hal/Hwi_stack.c \
	$(BIOS_ROOT)hal/Hwi_startup.c \
	$(BIOS_ROOT)hal/Timer.c \
	$(BIOS_ROOT)knl/Clock.c \
	$(BIOS_ROOT)knl/Idle.c \
	$(BIOS_ROOT)knl/Intrinsics.c \
	$(BIOS_ROOT)knl/Queue.c \
	$(BIOS_ROOT)knl/Semaphore.c \
	$(BIOS_ROOT)knl/Swi.c \
	$(BIOS_ROOT)knl/Swi_andn.c \
	$(BIOS_ROOT)knl/Task.c \
	$(BIOS_ROOT)family/arm/m3/Hwi.c \
	$(BIOS_ROOT)family/arm/m3/TaskSupport.c \
	$(BIOS_ROOT)family/arm/lm3/TimestampProvider.c \
	$(BIOS_ROOT)family/arm/lm3/Timer.c \
	$(BIOS_ROOT)gates/GateHwi.c \
	$(BIOS_ROOT)gates/GateMutex.c \
	$(BIOS_ROOT)heaps/HeapMem.c \

sysbios.aem3: BIOS.obj m3_Clobber_asm.obj m3_IntrinsicsSupport_asm.obj m3_TaskSupport_asm.obj m3_Hwi_asm.obj m3_Hwi_asm_switch.obj ducati_CTM_asm.obj ducati_Core_asm.obj omap4430_Power_saveCpu.obj omap4430_Power_resumeCpu.obj smp_Power_saveCpu.obj smp_Power_resumeCpu.obj dmtimer_Timer_asm.obj
	@-$(call RM, $@)
	@echo arem3 $^ ...
	@$(AR) $@ $^


clean:
	@-$(call RM, sysbios.aem3)
	@-$(call RM, BIOS.obj)
	@-$(call RM, m3_Clobber_asm.obj)
	@-$(call RM, m3_IntrinsicsSupport_asm.obj)
	@-$(call RM, m3_TaskSupport_asm.obj)
	@-$(call RM, m3_Hwi_asm.obj)
	@-$(call RM, m3_Hwi_asm_switch.obj)
	@-$(call RM, ducati_CTM_asm.obj)
	@-$(call RM, ducati_Core_asm.obj)
	@-$(call RM, omap4430_Power_saveCpu.obj)
	@-$(call RM, omap4430_Power_resumeCpu.obj)
	@-$(call RM, smp_Power_saveCpu.obj)
	@-$(call RM, smp_Power_resumeCpu.obj)
	@-$(call RM, dmtimer_Timer_asm.obj)
