export DEBUG                                            ?= 1
# enable:1
# disable:0

WATCHER_DOG                                            ?= 1
# enable:1
# disable:0

export MBED                                             ?= 1
# enable:1
# disable:0

export RTOS                                             ?= 1
# enable:1
# disable:0

export AUDIO_CODEC_ASYNC_CLOSE             ?= 1
# enable:1
# disable:0

export AUDIO_EQ_PROCESS                         ?= 0
# enable:1
# disable:0

export HW_FIR_EQ_PROCESS                         ?= 1
# enable:1
# disable:0
export AUDIO_RESAMPLE                         ?= 0
# enable:1
# disable:0

export SPEECH_ECHO_CANCEL                      ?= 1
# enable:1
# disable:0

export SPEECH_NOISE_SUPPRESS                  ?= 1
# enable:1
# disable:0

export SPEECH_PACKET_LOSS_CONCEALMENT                  ?= 1
# enable:1
# disable:0

export VOICE_PROMPT                                 ?= 1
# enable:1
# disable:0

export LED_STATUS_INDICATION                   ?= 0
# enable:1
# disable:0

export BLE                                                 ?= 0
# enable:1
# disable:0

export BTADDR_GEN                                    ?= 1
# enable:1
# disable:0

export FACTORY_MODE                                 ?= 1
# enable:1
# disable:0

export ENGINEER_MODE                                 ?= 1
# enable:1
# disable:0

export AUDIO_SCO_BTPCM_CHANNEL               ?= 1
# enable:1
# disable:0

export USB_AUDIO_MODE ?= 1
# usb audio switch

# usb audio options
export USB_ISO ?= 1
# enable:1
# disable:0
#

export USB_AUDIO_96K    := 1
# enable:1
# disable:0
#

export USB_AUDIO_24BIT  := 0
# enable:1
# disable:0
#

export CODEC_HIGH_QUALITY := 0
# enable:1
# disable:0
#


LETV		?= 1
ifeq ($(LETV),1)
ifeq ($(USB_AUDIO_96K),1)
export USB_AUDIO_VENDOR_ID	?= 0x2611
export USB_AUDIO_PRODUCT_ID	?= 0x1511
else
export USB_AUDIO_VENDOR_ID	?= 0x2622
export USB_AUDIO_PRODUCT_ID	?= 0x1522
endif
endif

export POWER_MODE	?= DIG_DCDC

include $(KBUILD_ROOT)/config/$(T)/customize.mk
include $(KBUILD_ROOT)/config/$(T)/hardware.mk
include $(KBUILD_ROOT)/config/convert.mk
