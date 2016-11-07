cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

src_obj := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)src/*.c $(cur_dir)src/*.S))
src_obj := $(src_obj:.c=.o)
src_obj := $(src_obj:.S=.o)

cfg_obj := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)cfg/*.c $(cur_dir)cfg/*.S))
cfg_obj := $(cfg_obj:.c=.o)
cfg_obj := $(cfg_obj:.S=.o)

libusbdev-y := $(src_obj)

obj-y := libusbdev.a $(cfg_obj)

ccflags-y += -Iplatform/drivers/usb/usb_dev/inc

CFLAGS_usb_cdc.o += -Itests/programmer

CFLAGS_usb_audio.o += -Iutils/hwtimer_list

CFLAGS_usb_audio_sync.o += -Iplatform/drivers/ana

ifneq ($(USB_CDC_VENDOR_ID),)
CFLAGS_usb_dev_desc.o += -DUSB_CDC_VENDOR_ID=$(USB_CDC_VENDOR_ID)
endif

ifneq ($(USB_CDC_PRODUCT_ID),)
CFLAGS_usb_dev_desc.o += -DUSB_CDC_PRODUCT_ID=$(USB_CDC_PRODUCT_ID)
endif

ifneq ($(USB_AUDIO_VENDOR_ID),)
CFLAGS_usb_dev_desc.o += -DUSB_AUDIO_VENDOR_ID=$(USB_AUDIO_VENDOR_ID)
endif

ifneq ($(USB_AUDIO_PRODUCT_ID),)
CFLAGS_usb_dev_desc.o += -DUSB_AUDIO_PRODUCT_ID=$(USB_AUDIO_PRODUCT_ID)
endif

ifeq ($(USB_AUDIO_96K),1)
CFLAGS_usb_audio.o += -DUSB_AUDIO_96K
endif

ifeq ($(USB_AUDIO_44_1K),1)
CFLAGS_usb_audio.o += -DUSB_AUDIO_44_1K
endif

ifeq ($(USB_AUDIO_24BIT),1)
CFLAGS_usb_audio.o += -DUSB_AUDIO_24BIT
endif

ifeq ($(USB_AUDIO_MIC_MONO),1)
CFLAGS_usb_audio.o += -DUSB_AUDIO_MIC_MONO
endif
