/**********************************************************
* 线程池控制类（生成宏）
* 支持平台：Windows
* 编译环境：VS2013+
***********************************************************/

#include "threadpool.h"

using namespace std;

// 线程运行前准备，捕获异常
template<> inline size_t threadpool<true>::pre_run(HANDLE exit_event)
{
    while (true)
    {
        try
        {
            return run(exit_event);
        }
        catch (::std::function<void()>& function_object)
        {
            debug_output<true>(_T(__FILE__), _T('('), __LINE__, _T("): "), function_object.target_type().name());
            m_exception_tasks.push_back(::std::move(function_object));
        }
    }
}

// 线程运行前准备，不捕获异常
template<> inline size_t threadpool<false>::pre_run(HANDLE exit_event)
{
    return run(exit_event);
}


#define HANDLE_EXCEPTION true
#include "xxthreadpool.h"
#undef HANDLE_EXCEPTION

#define HANDLE_EXCEPTION false
#include "xxthreadpool.h"
