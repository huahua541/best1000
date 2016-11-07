cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj-y := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c $(cur_dir)*.cpp $(cur_dir)*.S))
obj-y := $(obj-y:.c=.o)
obj-y := $(obj-y:.cpp=.o)
obj-y := $(obj-y:.S=.o)

subdir-ccflags-y += \
	-Iutils/cqueue/ \
	-Iservices/multimedia/audio/process/anc/include \
	-Iplatform/drivers/ana/ \
	-Iservices/multimedia/rbcodec/ \
	-Iservices/multimedia/rbcodec/dsp/ \
	-Iservices/multimedia/rbcodec/dsp/common \
	-Iservices/multimedia/rbcodec/codecs/ \
	-Iservices/multimedia/rbcodec/codecs/lib \
	-Iservices/multimedia/rbcodec/test/ \
	-Iservices/multimedia/rbcodec/metadata/ \
	-Iservices/multimedia/audio/process/anc/include \
	-Iplatform/drivers/usb/usb_dev/inc \
	-Itests/anc_usb

ifeq ($(APP_TEST_AUDIO),1)
ccflags-y += -DAPP_TEST_AUDIO
endif

ifeq ($(USB_AUDIO_96K),1)
CFLAGS_app_audtest.o += -DUSB_AUDIO_96K
endif

ifeq ($(USB_AUDIO_44_1K),1)
CFLAGS_app_audtest.o += -DUSB_AUDIO_44_1K
endif

ifeq ($(USB_AUDIO_24BIT),1)
CFLAGS_app_audtest.o += -DUSB_AUDIO_24BIT
endif
