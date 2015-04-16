/**********************************************************
 * STL扩展对象定义
 * 支持平台：Windows; Linux
 * 编译环境：VS2013+; g++ -std=c++11
 * 创建时间：2015-04-16 （宋万鹏）
 * 最后修改：2015-04-16 （宋万鹏）
 **********************************************************/

#include "common.h"

using namespace std;


mutex g_log_lock;
ofstream g_log_ofstream;
wofstream g_log_wofstream;
