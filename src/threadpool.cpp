/**********************************************************
* 线程池控制类（生成宏）
* 支持平台：Windows
* 编译环境：VS2013+
***********************************************************/

#include "threadpool.h"

using namespace std;


template<> size_t threadpool<true>::thread_entry(threadpool* object, HANDLE exit_event)
{
    debug_output(_T("Thread Start: ["), this_type().name(), _T("](0x"), object, _T(')'));
    size_t result;
    while (true)
    {
        try
        {
            result = object->run(exit_event);
            break;
        }
        catch (::std::function<void()>& function_object)
        {
            debug_output<true>(_T(__FILE__), _T('('), __LINE__, _T("): "), function_object.target_type().name());
            object->m_exception_tasks.push_back(::std::move(function_object));
        }
    }
    debug_output(_T("Thread Result: ["), (void*)result, _T("] ["), this_type().name(), _T("](0x"), object, _T(')'));
#if _MSC_VER <= 1800 // Fix std::thread deadlock bug on VS2012,VS2013 (when call join on exit)
    ::ExitThread((DWORD)result);
#endif // #if _MSC_VER <= 1800
    return result;
}

template<> size_t threadpool<false>::thread_entry(threadpool* object, HANDLE exit_event)
{
    debug_output(_T("Thread Start: ["), this_type().name(), _T("](0x"), object, _T(')'));
    size_t result = object->run(exit_event);
    debug_output(_T("Thread Result: ["), (void*)result, _T("] ["), this_type().name(), _T("](0x"), object, _T(')'));
#if _MSC_VER <= 1800 // Fix std::thread deadlock bug on VS2012,VS2013 (when call join on exit)
    ::ExitThread((DWORD)result);
#endif // #if _MSC_VER <= 1800
    return result;
}


#define HANDLE_EXCEPTION true
#include "xxthreadpool.h"
#undef HANDLE_EXCEPTION

#define HANDLE_EXCEPTION false
#include "xxthreadpool.h"
