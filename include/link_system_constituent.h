/**********************************************************
* 链接库文件
* 支持平台：Windows; Linux
* 编译环境：VS2010+; g++
***********************************************************/

#pragma once

#define _SYSTEM_CONSTITUENT_VERSION_   130


#ifndef _CRT_STRINGIZE
#define __CRT_STRINGIZE(_Value) #_Value
#define _CRT_STRINGIZE(_Value) __CRT_STRINGIZE(_Value)
#endif  /* _CRT_STRINGIZE */

#ifdef _MSC_VER

#ifdef _DEBUG       /* _DEBUG */
#  ifdef _M_X64     /* _M_X64 */
#    pragma comment(lib, "Debug/x64/system" _CRT_STRINGIZE(_SYSTEM_CONSTITUENT_VERSION_) ".lib")
#  else             /* _M_X86 */
#    pragma comment(lib, "Debug/system" _CRT_STRINGIZE(_SYSTEM_CONSTITUENT_VERSION_) ".lib")
#  endif
#else               /* NDEBUG */
#  ifdef _M_X64     /* _M_X64 */
#    pragma comment(lib, "x64/system" _CRT_STRINGIZE(_SYSTEM_CONSTITUENT_VERSION_) ".lib")
#  else             /* _M_X86 */
#    pragma comment(lib, "system" _CRT_STRINGIZE(_SYSTEM_CONSTITUENT_VERSION_) ".lib")
#  endif
#endif

#endif /* _MSC_VER */
