/**********************************************************
* STL扩展对象定义
* 支持平台：Windows; Linux
* 编译环境：VS2013+; g++ -std=c++11
* 创建时间：2015-04-16 （宋万鹏）
* 最后修改：2015-05-02 （宋万鹏）
***********************************************************/

#include "common.h"

using namespace std;

log_stream_manager_t::log_stream_manager_t()
    : m_log_ofstream(new ofstream), m_log_wofstream(new wofstream)
{
    m_clog_rdbuf = clog.rdbuf();
    m_wclog_rdbuf = wclog.rdbuf();
}

log_stream_manager_t::~log_stream_manager_t()
{
    clog.rdbuf(m_clog_rdbuf);
    wclog.rdbuf(m_wclog_rdbuf);
#if _MSC_VER <= 1800
    m_log_ofstream.release();
    m_log_wofstream.release();
#endif /* _MSC_VER <= 1800 */
}

log_stream_manager_t g_log_stream_manager;
