obj-y := audioplayers/ common/ main/ key/ pwl/ battery/ factory/ usbaudio/

ifeq ($(APP_TEST_AUDIO),1)
obj-y += apptester/
endif

ifeq ($(APP_TEST_SDMMC),1)
obj-y += sdmmc/
endif

subdir-ccflags-y += -Iapps/apptester \
					-Iapps/audioplayers \
					-Iapps/common \
					-Iapps/sdmmc \
					-Iapps/main \
					-Iapps/key \
					-Iapps/pwl \
					-Iapps/battery \
					-Iservices/resources \
					-Iutils/list
