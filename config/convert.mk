LDS_FILE    := best1000.lds

LIB_LDFLAGS += -lstdc++ -lsupc++

CFLAGS_IMAGE += -u _printf_float -u _scanf_float
#LDFLAGS_IMAGE += --wrap main

CPPFLAGS_${LDS_FILE} += -DLINKER_SCRIPT

ifeq ($(DEBUG_PORT),0)
KBUILD_CPPFLAGS += -DDEBUG_PORT=0
endif

ifeq ($(DEBUG_PORT),1)
KBUILD_CPPFLAGS += -DDEBUG_PORT=1
endif

ifeq ($(DEBUG_PORT),2)
KBUILD_CPPFLAGS += -DDEBUG_PORT=2
endif

ifeq ($(BLE),1)
core-y      += utils/jansson/
KBUILD_CPPFLAGS += -DIAG_BLE_INCLUDE=1
endif

ifeq ($(BTADDR_GEN),1)
KBUILD_CPPFLAGS += -DRANDOM_BTADDR_GEN
endif

ifeq ($(AUDIO_OUTPUT_MONO),1) 
KBUILD_CPPFLAGS += -D__AUDIO_OUTPUT_MONO_MODE__
endif

export AUDIO_OUTPUT_DIFF ?= 1

ifeq ($(AUDIO_CODEC_ASYNC_CLOSE),1) 
KBUILD_CPPFLAGS += -D__CODEC_ASYNC_CLOSE__
endif

ifeq ($(WATCHER_DOG),1) 
KBUILD_CPPFLAGS += -D__WATCHER_DOG_RESET__
endif

ifeq ($(LED_STATUS_INDICATION),1) 
KBUILD_CPPFLAGS += -D__LED_STATUS_INDICATION__
endif

ifeq ($(SPEECH_ECHO_CANCEL),1) 
KBUILD_CPPFLAGS += -DSPEECH_ECHO_CANCEL
endif

ifeq ($(SPEECH_NOISE_SUPPRESS),1) 
KBUILD_CPPFLAGS += -DSPEECH_NOISE_SUPPRESS
endif

ifeq ($(SPEECH_PACKET_LOSS_CONCEALMENT),1) 
KBUILD_CPPFLAGS += -DSPEECH_PLC
endif

ifeq ($(AUDIO_EQ_PROCESS),1) 
# for dsp_process
KBUILD_CFLAGS += -DEQ_PROCESS
KBUILD_CFLAGS += -DCODEC
KBUILD_CFLAGS += -DROCKBOX
KBUILD_CFLAGS += -DROCKBOX_LITTLE_ENDIAN
KBUILD_CFLAGS += -D__PCTOOL__
CONFIG_CODEC=SWCODEC
endif

ifeq ($(HW_FIR_EQ_PROCESS),1) 
KBUILD_CPPFLAGS += -D__HW_FIR_EQ_PROCESS__
endif

ifeq ($(SW_IIR_EQ_PROCESS),1) 
KBUILD_CPPFLAGS += -D__SW_IIR_EQ_PROCESS__
endif

ifeq ($(AUDIO_RESAMPLE),1) 
KBUILD_CPPFLAGS += -D__AUDIO_RESAMPLE__
endif

ifeq ($(VOICE_PROMPT),1) 
KBUILD_CPPFLAGS += -DMEDIA_PLAYER_SUPPORT
KBUILD_CPPFLAGS += -D__AUDIO_QUEUE_SUPPORT__
endif

ifeq ($(VOICE_DETECT),1) 
KBUILD_CPPFLAGS += -DVOICE_DETECT
endif

ifeq ($(FACTORY_MODE),1) 
KBUILD_CPPFLAGS += -D__FACTORY_MODE_SUPPORT__
endif

ifeq ($(ENGINEER_MODE),1) 
KBUILD_CPPFLAGS += -D__FACTORY_MODE_SUPPORT__
KBUILD_CPPFLAGS += -D__ENGINEER_MODE_SUPPORT__
endif

ifeq ($(AUDIO_SCO_BTPCM_CHANNEL),1)
KBUILD_CPPFLAGS += -D_SCO_BTPCM_CHANNEL_
endif

export APP_TEST_MODE ?= 0
ifeq ($(APP_TEST_MODE),1)
KBUILD_CPPFLAGS += -DAPP_TEST_MODE
endif

ifeq ($(ULTRA_LOW_POWER),1)
KBUILD_CPPFLAGS += -DULTRA_LOW_POWER
endif

ifeq ($(USB_AUDIO_MODE),1)
KBUILD_CPPFLAGS += -DUSB_AUDIO_MODE=1
endif

export APP_TEST_AUDIO ?= 0

init-y      := 

core-y      := \
                    target/$(T)/ \
                    apps/ \
                    services/ \
                    services/multimedia/ \
                    platform/ \
                    utils/cqueue/ \
                    utils/intersyshci/ \
                    utils/list/

KBUILD_CPPFLAGS += \
                                -Itarget/$(T) \
                                -Iplatform/cmsis/inc \
                                -Iservices/audioflinger \
                                -Iplatform/drivers/codec \
                                -Iplatform/hal \
                                -Iplatform/drivers/norflash

KBUILD_CPPFLAGS += \
                                -D_BT_2A_1S_ \
                                -DAUDIO_OUTPUT_VOLUME_DEFAULT=$(AUDIO_OUTPUT_VOLUME_DEFAULT)
#-D_CVSD_BYPASS_
#-D_BT_2A_1S_ -D_BT_2A_2S_

IMAGE_FILE	:= $(T).elf
