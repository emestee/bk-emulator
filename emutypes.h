#ifndef _EMUTYPES_H_
#define _EMUTYPES_H_

#if defined(_MSC_VER)
#include <crtdbg.h>
#endif
#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned int e_uint;
typedef signed int e_int;

typedef unsigned char e_uint8 ;
typedef signed char e_int8 ;

typedef unsigned short e_uint16 ;
typedef signed short e_int16 ;

typedef unsigned int e_uint32 ;
typedef signed int e_int32 ;

#ifdef __cplusplus
}
#endif
#endif
