#ifndef STDINT_H
#define STDINT_H

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_INT8                    127
#define MIN_INT8                    (-MAX_INT8 - 1)
#define MAX_UINT8                   255
#define MIN_UINT8                   0
#define MAX_INT16                   32767
#define MIN_INT16                   (-MAX_INT16 - 1)
#define MAX_UINT16                  65535
#define MIN_UINT16                  0
#define MAX_INT32                   2147483647L
#define MIN_INT32                   (-MAX_INT32 - 1)
#define MAX_UINT32                  4294967295U
#define MIN_UINT32                  0U
#ifdef WITH_LONG_LONG
#define INTMAX_MAX                  0x7fffffffffffffff
#define INTMAX_MIN                  -(0x7fffffffffffffff-1)
#define UINTMAX_MAX                 0xffffffffffffffff
#else
#define INTMAX_MAX                  0x7fffffff
#define INTMAX_MIN                  -(0x7fffffff-1)
#define UINTMAX_MAX                 0xffffffff
#endif
#define INT_MAX                     (char *)(1<<31)
#define LONG_MAX                    0x7fffffff
#define LONG_MIN                    -(0x7fffffff-1)
#define ULONG_MAX                   0xffffffff


typedef unsigned char               uint8_t;
typedef signed char                 int8_t;
typedef unsigned short int          uint16_t;
typedef signed short int            int16_t;
typedef unsigned int                uint32_t;
typedef signed int                  int32_t;
typedef char                        boolean;

typedef unsigned long long          uint64_t;
typedef signed long long            int64_t;

#ifndef size_t
typedef unsigned int                size_t;
#endif

#ifndef ssize_t
typedef int                         ssize_t;
#endif

#ifdef WITH_LONG_LONG
typedef unsigned long long          uintmax_t;
typedef long long                   intmax_t;
#else
typedef unsigned long               uintmax_t;
typedef long                        intmax_t;
#endif

#ifdef __cplusplus
}
#endif

#endif

