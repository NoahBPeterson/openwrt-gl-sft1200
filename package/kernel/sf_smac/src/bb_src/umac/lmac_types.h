#ifndef _LMAC_INT_H_
#define _LMAC_INT_H_ 
#include <linux/types.h>
#ifdef CONFIG_SIWIFI_TL4
typedef uint16_t u8_l;
typedef int16_t s8_l;
typedef uint16_t bool_l;
#else
typedef uint8_t u8_l;
typedef int8_t s8_l;
typedef bool bool_l;
#endif
typedef uint16_t u16_l;
typedef int16_t s16_l;
typedef uint32_t u32_l;
typedef int32_t s32_l;
typedef uint64_t u64_l;
#endif
