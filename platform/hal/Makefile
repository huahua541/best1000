cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj-y := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c))
obj-y := $(obj-y:.c=.o)

ccflags-y +=  \
	-Iplatform/drivers/ana


CFLAGS_hal_key.o += -Iutils/hwtimer_list

CFLAGS_hal_norflash.o += -Itests/programmer -Iplatform/drivers/norflash

ifeq ($(SIMU_FAST_FLASH),1)
CFLAGS_hal_norflash.o += -DSIMU_FAST_FLASH
endif

CFLAGS_hal_usb.o += -Iplatform/drivers/usb/usb_dev/inc

CFLAGS_hal_usbhost.o += -Iutils/hwtimer_list

CFLAGS_hal_sleep.o += -Iplatform/drivers/norflash

ifeq ($(FORCE_TRACE_UART1),1)
CFLAGS_hal_trace.o := -DFORCE_TRACE_UART1
endif

ifeq ($(NO_LPU_26M),1)
CFLAGS_hal_cmu.o += -DNO_LPU_26M
endif

ifneq (${FLASH_CHIP},)
KBUILD_CPPFLAGS += -D__NORFLASH_${FLASH_CHIP}__
endif

ifeq ($(USB_ISO),1)
CFLAGS_hal_usb.o += -DUSB_ISO
endif

ifneq ($(ROM_BUILD),1)
ifneq ($(PROGRAMMER),1)
USB_SUSPEND ?= 1
endif
endif
ifeq ($(USB_SUSPEND),1)
CFLAGS_hal_usb.o += -DUSB_SUSPEND
endif
