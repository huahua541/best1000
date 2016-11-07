cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj-y := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c $(cur_dir)*.cpp $(cur_dir)*.S))
obj-y := $(obj-y:.c=.o)
obj-y := $(obj-y:.cpp=.o)
obj-y := $(obj-y:.S=.o)

ccflags-y +=  \
    -Iservices/bt_profiles/inc \
    -Iservices/bt_profiles \
    -Iservices/bt_app \
    -Iservices/audioflinger \
    -Iservices/nvrecord \
    -Iservices/overlay \
    -Iservices/resources \
    -Iapps/apptester \
    -Iapps/factory \
    -Iapps/usbaudio \
    -Iutils/crc32 \
    -Iplatform/drivers/bt \
    -Iapps/audioplayers/rbplay \
    -Iapps/pwl 

ifeq ($(USB_AUDIO_96K),1)
	CFLAGS_apps.o += -DUSB_AUDIO_96K
	CFLAGS_apps_tester.o += -DUSB_AUDIO_96K
endif

ifeq ($(USB_AUDIO_44_1K),1)
	CFLAGS_apps.o += -DUSB_AUDIO_44_1K
	CFLAGS_apps_tester.o += -DUSB_AUDIO_44_1K
endif

ifeq ($(USB_AUDIO_24BIT),1)
	CFLAGS_apps.o += -DUSB_AUDIO_24BIT
	CFLAGS_apps_tester.o += -DUSB_AUDIO_24BIT
endif

ifeq ($(APP_TEST_AUDIO),1)
CFLAGS_apps_tester.o += -DAPP_TEST_AUDIO
endif
