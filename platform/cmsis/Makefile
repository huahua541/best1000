
obj-y := system_ARMCM4.o cmsis_nvic.o

ifeq ($(DSP_LIB),1)
obj-y += DSP_Lib/
endif

ccflags-y := -I$(obj)/inc \
	-Iplatform/cmsis/inc \
	-Iplatform/drivers/ana \
	-DARMCM4_FP
