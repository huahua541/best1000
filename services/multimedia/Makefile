
cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj_c := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)speech/codec/codecs/cvsd/src/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)speech/process/resample/src/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)speech/process/plc/src/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)speech/process/ns_and_ec/src/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)speech/application/Pcm8kCvsd/src/*.c))

obj_cpp := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)speech/codec/codecs/cvsd/src/*.cpp))
obj_cpp += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)speech/process/resample/src/*.cpp))
obj_cpp += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)speech/process/plc/src/*.cpp))
obj_cpp += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)speech/process/ns_and_ec/src/*.cpp))
obj_cpp += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)speech/application/Pcm8kCvsd/src/*.cpp))


obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)audio/process/anc/src/*.c))
obj_cpp += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)audio/process/anc/src/src/*.cpp))

ifeq ($(VOICE_DETECT),1)
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)webrtc/src/common_audio/vad/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)webrtc/src/common_audio/signal_processing/division_operations.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)webrtc/src/common_audio/signal_processing/energy.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)webrtc/src/common_audio/signal_processing/get_scaling_square.c))
endif

ifeq ($(AUDIO_RESAMPLE),1)
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)audio/process/resample/src/*.c))
obj_cpp += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)audio/process/resample/src/*.cpp))
endif

ifeq ($(HW_FIR_EQ_PROCESS),1)
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)audio/process/eq/src/*.c))
obj_cpp += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)audio/process/eq/src/*.cpp))
else
ifeq ($(SW_IIR_EQ_PROCESS),1)
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)audio/process/eq/src/*.c))
obj_cpp += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)audio/process/eq/src/*.cpp))
endif
endif

ifeq ($(AUDIO_EQ_PROCESS),1)
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)rbcodec/dsp/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)rbcodec/dsp/common/*.c))
obj_cpp += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)rbcodec/dsp/common/*.cpp))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)rbcodec/metadata/*.c))
obj_cpp += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)rbcodec/eq.cpp))
obj_s := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)rbcodec/dsp/filter_process.S))
endif

ifeq ($(RB_CODEC),1)
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)rbcodec/codecs.c))
# codec lib
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)rbcodec/codecs/lib/codeclib.c))
# mpa
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)rbcodec/codecs/mpa.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)rbcodec/codecs/libmad/*.c))
# wav
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)rbcodec/codecs/wav.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)rbcodec/codecs/libpcm/*.c))
endif

obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)fm/src/*.c))
obj_cpp += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)algorithm/fft/src/*.cpp))

libmultimedia-y := $(obj_c:.c=.o) $(obj_s:.S=.o) $(obj_cpp:.cpp=.o)

obj-y := libmultimedia.a

ccflags-y := \
	-Irtos/rtx/TARGET_CORTEX_M/ \
	-Imbed/api/ \
	-Imbed/common \
	-Imbed/targets/hal/TARGET_BEST/TARGET_BEST100X/TARGET_MBED_BEST1000/ \
	-Icmsis/inc/ \
	-Imbed/targets/hal/TARGET_BEST/TARGET_BEST100X/ \
	-Imbed/hal/ \
	-Ifs/fat/ \
	-Ifs/sd/ \
	-Ifs/fat/ChaN \
	-Irtos/rtos/ \
	-Iiany/inc/ \
	-Iservices/multimedia/speech/codec/codecs/cvsd/include \
	-Iservices/multimedia/speech/process/resample/include \
	-Iservices/multimedia/speech/process/plc/include \
	-Iservices/multimedia/speech/process/ns_and_ec/include \
	-Iservices/multimedia/audio/process/anc/include \
	-Iservices/multimedia/speech/application/Pcm8kCvsd/include \
	-Iservices/multimedia/rbcodec \
	-Iservices/multimedia/rbcodec/codecs \
	-Iservices/multimedia/rbcodec/codecs/lib \
	-Iservices/multimedia/rbcodec/dsp \
	-Iservices/multimedia/rbcodec/dsp/common \
	-Iservices/multimedia/rbcodec/test \
	-Iservices/multimedia/rbcodec/metadata \
	-Iservices/multimedia/audio/process/resample/include \
	-Iservices/multimedia/audio/process/eq/include \
	-Iservices/multimedia/algorithm/fft/include \
	-Iservices/multimedia/fm/inc \
	-Iservices/multimedia/webrtc/include \
	-Iservices/multimedia/webrtc/src \
	-Iservices/multimedia/webrtc/src/common_audio/vad/include  \
	-Iservices/multimedia/webrtc/src/common_audio/signal_processing/include \
	-Iservices/bt_profiles/inc

ccflags-y += -DFLAC__INTEGER_ONLY_LIBRARY -DVERSION="\"1.3.1\"" -DHAVE_CONFIG_H -w

