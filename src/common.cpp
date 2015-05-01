/**********************************************************
* STL扩展对象定义
* 支持平台：Windows; Linux
* 编译环境：VS2013+; g++ -std=c++11
* 创建时间：2015-04-16 （宋万鹏）
* 最后修改：2015-05-01 （宋万鹏）
***********************************************************/

#include "common.h"

::std::mutex g_log_lock;
::std::ofstream g_log_ofstream;
::std::wofstream g_log_wofstream;
