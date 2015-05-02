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
mutex g_log_lock;
unique_ptr<ofstream> g_log_ofstream;
unique_ptr<wofstream> g_log_wofstream;

// ��̬����
static streambuf* m_clog_rdbuf;
static wstreambuf* m_wclog_rdbuf;

struct log_stream_manager_t
{
    log_stream_manager_t()
    {
        m_clog_rdbuf = clog.rdbuf();
        m_wclog_rdbuf = wclog.rdbuf();
        g_log_ofstream = make_unique<ofstream>();
        g_log_wofstream = make_unique<wofstream>();
    }
    ~log_stream_manager_t()
    {
        clog.rdbuf(m_clog_rdbuf);
        wclog.rdbuf(m_wclog_rdbuf);
#if _MSC_VER <= 1800
        g_log_ofstream.release();
        g_log_wofstream.release();
#endif /* _MSC_VER <= 1800 */
    }
} m_log_stream_manager;
