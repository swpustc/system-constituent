/**********************************************************
* 链接库文件
* 支持平台：Windows; Linux
* 编译环境：VS2010+; g++
***********************************************************/

#pragma once

#include "system_constituent_version.h"


#ifdef _MSC_VER

#ifdef _DEBUG       /* _DEBUG */
#  ifdef _M_X64     /* _M_X64 */
#    pragma comment(lib, "Debug/x64/system" _CRT_STRINGIZE(SYSTEM_CONSTITUENT_VERSION) ".lib")
#  else             /* _M_X86 */
#    pragma comment(lib, "Debug/system" _CRT_STRINGIZE(SYSTEM_CONSTITUENT_VERSION) ".lib")
#  endif
#else               /* NDEBUG */
#  ifdef _M_X64     /* _M_X64 */
#    pragma comment(lib, "x64/system" _CRT_STRINGIZE(SYSTEM_CONSTITUENT_VERSION) ".lib")
#  else             /* _M_X86 */
#    pragma comment(lib, "system" _CRT_STRINGIZE(SYSTEM_CONSTITUENT_VERSION) ".lib")
#  endif
#endif

#endif /* _MSC_VER */
