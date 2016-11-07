cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj-y := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c $(cur_dir)*.S))
obj-y := $(obj-y:.c=.o)
obj-y := $(obj-y:.S=.o)

ifeq ($(NO_SLEEP),1)
CFLAGS_pmu.o += -DNO_SLEEP
endif

ifeq ($(POWER_MODE),LDO)
CFLAGS_pmu.o += -DLDO_MODE
else
ifeq ($(POWER_MODE),DIG_DCDC)
CFLAGS_pmu.o += -DDIG_DCDC_MODE
else
ifeq ($(POWER_MODE),ANA_DCDC)
CFLAGS_pmu.o += -DANA_DCDC_MODE
else
ifneq ($(POWER_MODE),)
$(error Invalid POWER_MODE: $(POWER_MODE))
endif
endif
endif
endif

ifeq ($(AUDIO_INPUT_CAPLESSMODE),1)
CFLAGS_analog.o += -D_MIC_CAPLESSMODE_
endif

ifeq ($(AUDIO_INPUT_LARGEGAIN),1)
CFLAGS_analog.o += -D_MIC_LARGEGAIN_
endif

ifeq ($(AUDIO_OUTPUT_DIFF),1)
CFLAGS_analog.o += -DAUDIO_OUTPUT_DIFF
endif

ifeq ($(CODEC_HIGH_QUALITY),1)
CFLAGS_analog.o += -DCODEC_HIGH_QUALITY
CFLAGS_pmu.o += -DCODEC_HIGH_QUALITY
endif

ifeq ($(AUDIO_CODEC_ASYNC_CLOSE),1) 
CFLAGS_analog.o += -D__CODEC_ASYNC_CLOSE__
endif

ifeq ($(AUDIO_RESAMPLE),1)
CFLAGS_analog.o += -D__AUDIO_RESAMPLE__
endif
