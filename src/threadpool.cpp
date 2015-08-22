/**********************************************************
* 线程池控制类（生成宏）
* 支持平台：Windows
* 编译环境：VS2013+
***********************************************************/

#include "threadpool.h"

using namespace std;

// 线程运行前准备，捕获异常
template<> inline size_t threadpool<true>::pre_run(HANDLE pause_event, HANDLE resume_event)
{
    while (true)
    {
        try
        {
            return run(pause_event, resume_event);
        }
        catch (function<void()>& function_object)
        {
            debug_output<true>(_T(__FILE__), _T('('), __LINE__, _T("): "), function_object.target_type().name());
            m_exception_tasks.push_back(move(function_object));
            m_task_exception++;
        }
    }
}

// 线程运行前准备，不捕获异常
template<> inline size_t threadpool<false>::pre_run(HANDLE pause_event, HANDLE resume_event)
{
    return run(pause_event, resume_event);
}


// 运行一条任务，返回任务队列中是否还有任务[true:有任务; false:没任务]，捕获异常
template<> inline bool threadpool<true>::run_task(pair<function<void()>, size_t>&& task_val)
{
    // 任务队列中有任务未处理，发送线程启动通知
    if (task_val.second > 1)
        notify();
    if (task_val.second)
    {
        try
        {
            task_val.first();
        }
        catch (exception& e)
        {
            debug_output<true>(_T(__FILE__), _T('('), __LINE__, _T("): "), e.what(), " | ", task_val.first.target_type().name());
            throw move(task_val.first);
            return false;
        }
        catch (...)
        {
            throw move(task_val.first);
            return false;
        }
        m_task_completed++;
    }
    return task_val.second > 1;
}

// 运行一条任务，返回任务队列中是否还有任务[true:有任务; false:没任务]，不捕获异常
template<> inline bool threadpool<false>::run_task(pair<function<void()>, size_t>&& task_val)
{
    // 任务队列中有任务未处理，发送线程启动通知
    if (task_val.second > 1)
        notify();
    if (task_val.second)
    {
        task_val.first();
        m_task_completed++;
    }
    return task_val.second > 1;
}


#define HANDLE_EXCEPTION true
#include "xxthreadpool.h"
#undef HANDLE_EXCEPTION

#define HANDLE_EXCEPTION false
#include "xxthreadpool.h"
