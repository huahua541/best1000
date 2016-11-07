export DEBUG_PORT                               ?= 1
# usb:0
# uart0:1
# uart1:2

export FLASH_CHIP                                ?= GD25Q32C
# GD25Q80C
# GD25Q32C

export AUDIO_OUTPUT_MONO                    ?= 0
# enable:1
# disable:0

export AUDIO_OUTPUT_DIFF                    ?= 1
# enable:1
# disable:0

export AUDIO_OUTPUT_VOLUME_DEFAULT  ?= 13
# range:1~16

export AUDIO_INPUT_CAPLESSMODE          ?= 0
# enable:1
# disable:0

export AUDIO_INPUT_LARGEGAIN               ?= 0
# enable:1
# disable:0

export ULTRA_LOW_POWER                  ?= 1
# enable:1
# disable:0

KBUILD_CPPFLAGS += \
                                    -D_AUTO_SWITCH_POWER_MODE__ \
                                    -D_BEST1000_QUAL_DCDC_ \
                                    
                                    
#-D_AUTO_SWITCH_POWER_MODE__ 
#-D_BEST1000_QUAL_DCDC_
#-D__FORCE_DISABLE_SLEEP__
#-D__BEST_FLASH_VIA_ANA__
#-DAPP_TEST_MODE
#-D_BEST1101_ECO_   
