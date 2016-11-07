cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj-y := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c $(cur_dir)*.S))
obj-y := $(obj-y:.c=.o)
obj-y := $(obj-y:.S=.o)

obj-y += ../../utils/hexdump/

#ccflags-y := -I$(obj)/../../utils/hexdump -I$(obj)/../../utils/boot_struct -I$(obj)/../../tests/rom
ccflags-y := -Iutils/hexdump -Iutils/boot_struct -Itests/rom

CFLAGS_sys_api_programmer.o := -Iplatform/drivers/usb/usb_dev -Iplatform/drivers/norflash -Iplatform/drivers/ana

ifeq ($(PROGRAMMER_LOAD_SIMU),1)
CFLAGS_sys_api_programmer.o += -DPROGRAMMER_LOAD_SIMU
endif

ifeq ($(NO_UART_IOMUX),1)
CFLAGS_sys_api_programmer.o += -DNO_UART_IOMUX
endif
