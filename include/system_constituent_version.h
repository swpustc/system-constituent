/**********************************************************
* 版本号
* 支持平台：Windows; Linux
* 编译环境：VS2010+; g++
***********************************************************/

#pragma once

#define FILE_VERSION_MAJOR  1
#define FILE_VERSION_MINOR  3
#define FILE_VERSION_POINT  0
#define FILE_VERSION_POINT2 1


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

#define SYSTEM_CONSTITUENT_VERSION _CRT_APPEND(FILE_VERSION_MAJOR, _CRT_APPEND(FILE_VERSION_MINOR, FILE_VERSION_POINT))
