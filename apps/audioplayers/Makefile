
cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj_s := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.s))
obj_c := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c))
obj_cpp := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.cpp))

ifeq ($(RB_CODEC),1)
obj_cpp += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)rbplay/*.cpp))
endif

obj-y := $(obj_c:.c=.o) $(obj_s:.S=.o) $(obj_cpp:.cpp=.o)

CFLAGS_a2dpplay.o := -O3
		
ccflags-y := \
	-Iservices/fs/fat \
	-Iservices/fs/sd \
	-Iservices/resources \
	-Iservices/fs/fat/ChaN \
	-Iservices/bt_profiles \
	-Iservices/bt_app \
	-Iservices/bt_profiles/inc \
	-Iservices/multimedia/rbcodec \
	-Iplatform/drivers/uarthci \
	-Iutils/cqueue \
	-Iservices/multimedia/speech/codec/codecs \
	-Iservices/multimedia/speech/process \
	-Iservices/multimedia/speech/process/ns_and_ec/include \
	-Iservices/multimedia/speech/process/plc/include \
	-Iservices/multimedia/speech/application/Pcm8kCvsd/include \
	-Iservices/multimedia/rbcodec \
	-Iservices/multimedia/rbcodec/codecs \
	-Iservices/multimedia/rbcodec/codecs/lib \
	-Iservices/multimedia/rbcodec/dsp \
	-Iservices/multimedia/rbcodec/dsp/common \
	-Iservices/multimedia/rbcodec/test \
	-Iservices/multimedia/rbcodec/metadata \
	-Iservices/multimedia/fm/inc \
	-Iservices/multimedia/webrtc/include \
	-Iservices/multimedia/audio/process/eq/include \
	-Iservices/overlay \
	-Iapps/main \
	-Iutils/list \
	-Iplatform/drivers/ana