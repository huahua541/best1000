cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj-y := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c $(cur_dir)*.cpp $(cur_dir)*.S))

obj-y := $(obj-y:.c=.o)
obj-y := $(obj-y:.cpp=.o)
obj-y := $(obj-y:.S=.o)

ifeq ($(NOSTD),1)
asflags-y += -DNOSTD
endif

ifeq ($(INTSRAM_RUN),1)
asflags-y += -DINTSRAM_RUN
ccflags-y += -DINTSRAM_RUN
endif

ifeq ($(HW_FIR_EQ_PROCESS),1)
KBUILD_CPPFLAGS += -D__HW_FIR_EQ_PROCESS__
endif
ccflags-y +=  \
	-Iservices/multimedia/rbcodec \
	-Iservices/multimedia/audio/process/resample/include \
	-Iservices/multimedia/audio/process/eq/include \
	-Iutils/hwtimer_list \
	-Iplatform/drivers/ana \
	-Iservices/multimedia/audio/process/adp/include \
	-Iservices/multimedia/audio/process/anc/include \
	-Iplatform/drivers/usb/usb_dev/inc
ifneq ($(DEBUG_PORT),)
CFLAGS_main.o += -DDEBUG_PORT=$(DEBUG_PORT)
endif

ifeq ($(ANC_TEST),1)
CFLAGS_main.o += -DANC_TEST
endif

ifeq ($(USB_AUDIO_TEST),1)
CFLAGS_main.o += -DUSB_AUDIO_TEST
endif

ifeq ($(USB_AUDIO_96K),1)
CFLAGS_main.o += -DUSB_AUDIO_96K
CFLAGS_usb_audio_test.o += -DUSB_AUDIO_96K
endif

ifeq ($(USB_AUDIO_44_1K),1)
CFLAGS_main.o += -DUSB_AUDIO_44_1K
CFLAGS_usb_audio_test.o += -DUSB_AUDIO_44_1K
endif

ifeq ($(USB_AUDIO_24BIT),1)
CFLAGS_main.o += -DUSB_AUDIO_24BIT
CFLAGS_usb_audio_test.o += -DUSB_AUDIO_24BIT
endif

ifeq ($(DELAY_STREAM_OPEN),1)
CFLAGS_usb_audio_test.o += -DDELAY_STREAM_OPEN
endif

FADING_IN ?= 1
ifeq ($(FADING_IN),1)
CFLAGS_usb_audio_test.o += -DFADING_IN
endif
