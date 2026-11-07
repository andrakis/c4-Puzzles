// stdint.h -- the fixed-width types, for c4lc on a 32-bit C4IX (C4IX's
// libc has none yet; Wolf3D and Doom carry the same, with the fast types OpenTyrian uses)
#ifndef C4TYR_STDINT_H
#define C4TYR_STDINT_H 1
typedef signed char        int8_t;
typedef unsigned char      uint8_t;
typedef short              int16_t;
typedef unsigned short     uint16_t;
typedef int                int32_t;
typedef unsigned int       uint32_t;
typedef long long          int64_t;
typedef unsigned long long uint64_t;
typedef int                intptr_t;
typedef unsigned int       uintptr_t;
#define INT8_MIN   (-128)
#define INT16_MIN  (-32768)
#define INT32_MIN  (-2147483647 - 1)
#define INT8_MAX   127
#define INT16_MAX  32767
#define INT32_MAX  2147483647
#define UINT8_MAX  255
#define UINT16_MAX 65535
#define UINT32_MAX 4294967295u
#define UINT64_MAX 18446744073709551615ull
#define UINTPTR_MAX 4294967295u
#define INT64_MAX  9223372036854775807ll
typedef int8_t   int_fast8_t;
typedef uint8_t  uint_fast8_t;
typedef int      int_fast16_t;
typedef unsigned uint_fast16_t;
typedef int      int_fast32_t;
typedef unsigned uint_fast32_t;
typedef int      int_least16_t;
typedef unsigned uint_least16_t;
#define SIZE_MAX 4294967295u
#endif
