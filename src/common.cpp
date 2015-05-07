/**********************************************************
* STL扩展对象定义
* 支持平台：Windows; Linux
* 编译环境：VS2013+; g++ -std=c++11
* 创建时间：2015-04-16 （宋万鹏）
* 最后修改：2015-05-02 （宋万鹏）
***********************************************************/

#include "common.h"

using namespace std;

// 导出的变量
SYSCONAPI mutex g_log_lock;
SYSCONAPI ofstream g_log_ofstream;

#ifdef _MSC_VER
SYSCONAPI convert_cp_unicode_t<CP_UTF8, wchar_t> convert_utf8_unicode("bad conversion to utf8", L"bad conversion from utf8");
SYSCONAPI convert_cp_unicode_t<CP_ACP, wchar_t> convert_default_unicode("bad conversion to default", L"bad conversion from default");
#else  /* _MSC_VER */
SYSCONAPI wstring_convert<codecvt_utf8<wchar_t>, wchar_t> convert_utf8_unicode("bad conversion to utf8", L"bad conversion from utf8");
#endif  /* _MSC_VER */
