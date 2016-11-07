
add_if_exists = $(foreach d,$(1),$(if $(wildcard $(srctree)/$(d)),$(d) ,))

# -------------------------------------------
# Standard C library
# -------------------------------------------

export NOSTD

ifeq ($(NOSTD),1)

ifeq ($(MBED),1)
$(error Invalid configuratioin: MBED needs standard C library support)
endif
ifeq ($(RTOS),1)
$(error Invalid configuratioin: RTOS needs standard C library support)
endif

core-y += utils/libc/

SPECS_CFLAGS :=

LIB_LDFLAGS := $(filter-out -lstdc++ -lsupc++ -lm -lc -lgcc -lnosys,$(LIB_LDFLAGS))

KBUILD_CPPFLAGS += -nostdinc -ffreestanding -Iutils/libc/inc

CFLAGS_IMAGE += -nostdlib

CPPFLAGS_${LDS_FILE} += -DNOSTD

else # NOSTD != 1

SPECS_CFLAGS := --specs=nano.specs

LIB_LDFLAGS += -lm -lc -lgcc -lnosys

endif # NOSTD != 1

# -------------------------------------------
# RTOS library
# -------------------------------------------

export RTOS

export EQ_PROCESS

ifeq ($(RTOS),1)

core-y += rtos/

KBUILD_CPPFLAGS += \
	-Irtos/rtos \
	-Irtos/rtx/TARGET_CORTEX_M \

OS_TASKCNT ?= 12
OS_SCHEDULERSTKSIZE ?= 512
OS_CLOCK ?= 32000

export OS_TASKCNT
export OS_SCHEDULERSTKSIZE
export OS_CLOCK

endif

# -------------------------------------------
# MBED library
# -------------------------------------------

export MBED

ifeq ($(MBED),1)

core-y += mbed/

KBUILD_CPPFLAGS += \
	-Imbed/api \
	-Imbed/common \
	-Imbed/targets/hal/TARGET_BEST/TARGET_BEST100X/TARGET_MBED_BEST1000 \
	-Imbed/targets/hal/TARGET_BEST/TARGET_BEST100X \
	-Imbed/hal \

endif

# -------------------------------------------
# SIMU functions
# -------------------------------------------

export SIMU

ifeq ($(SIMU),1)

KBUILD_CPPFLAGS += -DSIMU

endif

# -------------------------------------------
# FPGA functions
# -------------------------------------------

export FPGA

ifeq ($(FPGA),1)

KBUILD_CPPFLAGS += -DFPGA

endif

# -------------------------------------------
# General
# -------------------------------------------

export LIBC_ROM

core-y += $(call add_if_exists,utils/boot_struct/)

CPU_CFLAGS := -mcpu=cortex-m4 -mthumb

ifeq ($(NO_FPU),1)

CPU_CFLAGS += -mfloat-abi=soft

else

CPU_CFLAGS += -mfpu=fpv4-sp-d16 -mfloat-abi=softfp

endif

KBUILD_CPPFLAGS += $(CPU_CFLAGS) $(SPECS_CFLAGS) -DTOOLCHAIN_GCC -D__CORTEX_M4F -DTARGET_BEST1000
LINK_CFLAGS += $(CPU_CFLAGS) $(SPECS_CFLAGS)
CFLAGS_IMAGE += $(CPU_CFLAGS) $(SPECS_CFLAGS)
