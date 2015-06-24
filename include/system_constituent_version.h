/**********************************************************
* 版本号
* 支持平台：Windows; Linux
* 编译环境：VS2010+; g++
***********************************************************/

#pragma once

#define SYSTEM_CONSTITUENT_VERSION_MAJOR  1
#define SYSTEM_CONSTITUENT_VERSION_MINOR  3
#define SYSTEM_CONSTITUENT_VERSION_POINT  1
#define SYSTEM_CONSTITUENT_VERSION_POINT2 4


#ifdef __cplusplus
#  define EXTERN_C  extern "C"
#else
#  define EXTERN_C
#endif

#ifndef _CRT_STRINGIZE
#define __CRT_STRINGIZE(_Value) #_Value
#define _CRT_STRINGIZE(_Value) __CRT_STRINGIZE(_Value)
#endif  /* _CRT_STRINGIZE */

#ifndef _CRT_WIDE
#define __CRT_WIDE(_String) L ## _String
#define _CRT_WIDE(_String) __CRT_WIDE(_String)
#endif  /* _CRT_WIDE */

#ifndef _CRT_APPEND
#define __CRT_APPEND(_Value1, _Value2) _Value1 ## _Value2
#define _CRT_APPEND(_Value1, _Value2) __CRT_APPEND(_Value1, _Value2)
#endif  /* _CRT_APPEND */

#ifndef _T
#ifdef _UNICODE
#define __TXT(x)    L ## x
#else  /* _UNICODE */
#define __TXT(x)    x
#endif  /* _UNICODE */
#define _T(x)       __TXT(x)
#endif  /* _T */


#define SYSTEM_CONSTITUENT_VERSION                  \
    _CRT_APPEND(SYSTEM_CONSTITUENT_VERSION_MAJOR,   \
    _CRT_APPEND(SYSTEM_CONSTITUENT_VERSION_MINOR,   \
                SYSTEM_CONSTITUENT_VERSION_POINT))

#ifdef _WIN32
#  ifdef SYSCON_EXPORT
#    define SYSCONAPI_EXTERN    extern __declspec(dllexport)
#    define SYSCONAPI           __declspec(dllexport)
#  else
#    define SYSCONAPI_EXTERN    extern __declspec(dllimport)
#    define SYSCONAPI           __declspec(dllimport)
#  endif
#endif
#ifndef SYSCONAPI
#  define SYSCONAPI_EXTERN      extern
#  define SYSCONAPI
#endif
