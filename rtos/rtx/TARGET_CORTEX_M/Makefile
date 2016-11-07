
cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj_s := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)TARGET_M4/TOOLCHAIN_GCC/*.S))
obj_c := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c))
obj_cpp := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.cpp))

obj-y := $(obj_c:.c=.o) $(obj_s:.S=.o) $(obj_cpp:.cpp=.o)



CFLAGS_RTX_Conf_CM.o := -DOS_TASKCNT=$(OS_TASKCNT) -DOS_SCHEDULERSTKSIZE=$(OS_SCHEDULERSTKSIZE) \
    -DOS_CLOCK=$(OS_CLOCK)

CFLAGS_rt_System.o := -DOS_CLOCK=$(OS_CLOCK)


ifeq ($(DEBUG_SLEEP),1)
CFLAGS_RTX_Conf_CM.o += -DDEBUG_SLEEP
CFLAGS_rt_System.o += -DDEBUG_SLEEP
endif
