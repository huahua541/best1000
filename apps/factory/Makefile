cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj-y := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c $(cur_dir)*.cpp $(cur_dir)*.S))
obj-y := $(obj-y:.c=.o)
obj-y := $(obj-y:.cpp=.o)
obj-y := $(obj-y:.S=.o)

ifeq ($(RTOS),1)
ccflags-y += -DRTOS
endif

ccflags-y +=  \
    -Iutils/boot_struct \
    -Iservices/bt_app \
    -Iservices/nvrecord \
    -Iservices/bt_profiles/inc \
    -Iservices/resources \
    -Iservices/overlay \
    -Iservices/multimedia/algorithm/fft/include \
    -Iapps/key \
    -Iplatform/drivers/bt \
    -Iplatform/drivers/ana
