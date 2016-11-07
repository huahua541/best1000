
#define BUILD_TIME_LOCATION __attribute__((section(".build_time")))

const char BUILD_TIME_LOCATION sys_build_time[] = __DATE__   " "   __TIME__;

