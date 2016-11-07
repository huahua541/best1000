cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

#obj-y := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c $(cur_dir)*.cpp $(cur_dir)*.S))
obj-y :=

ifeq ($(ROM_BUILD),1)
obj-y += ../../tests/rom/startup_ARMCM4.S
else
obj-y += startup_main.S
endif

ifeq ($(FACTORY_SUITE),1)
obj-y += ../../tests/factory_suite/
else
ifeq ($(RELIABILITY_TEST),1)
obj-y += ../../tests/reliability_test/
else
ifeq ($(NOSTD),1)
asflags-y += -DNOSTD
obj-y += nostd_main.c
else
ifeq ($(NOAPP),1)
obj-y += noapp_main.cpp
else
obj-y += main.cpp
endif
endif
endif
endif

obj-y := $(obj-y:.c=.o)
obj-y := $(obj-y:.cpp=.o)
obj-y := $(obj-y:.S=.o)

obj-y += ../../utils/hwtimer_list/

ccflags-y +=  \
	-Iutils/hwtimer_list \
	-Iplatform/drivers/ana \
	-Iapps/main \
	-Iapps/factory

ifeq ($(MBED),1)
ccflags-y += -DMBED
endif

ifeq ($(RTOS),1)
ccflags-y += -DRTOS
endif

ifneq ($(FLASH_FILL),)
ccflags-y += -DFLASH_FILL=$(FLASH_FILL)
endif
