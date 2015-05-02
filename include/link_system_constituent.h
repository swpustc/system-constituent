/**********************************************************
* 链接库文件
* 支持平台：Windows; Linux
* 编译环境：VS2010+; g++
* 创建时间：2015-05-01 （宋万鹏）
* 最后修改：2015-05-02 （宋万鹏）
***********************************************************/

#pragma once

#ifdef _MSC_VER

#ifdef _DEBUG       /* _DEBUG */
#  ifdef _M_X64     /* _M_X64 */
#    pragma comment(lib, "Debug/x64/system.lib")
#  else             /* _M_X86 */
#    pragma comment(lib, "Debug/system.lib")
#  endif
#else               /* NDEBUG */
#  ifdef _M_X64     /* _M_X64 */
#    pragma comment(lib, "x64/system.lib")
#  else             /* _M_X86 */
#    pragma comment(lib, "system.lib")
#  endif
#endif

#endif /* _MSC_VER */
