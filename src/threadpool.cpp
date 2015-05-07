/**********************************************************
* 线程池控制类
* 支持平台：Windows
* 编译环境：VS2013+
* 创建时间：2015-05-07 （宋万鹏）
* 最后修改：2015-05-07 （宋万鹏）
***********************************************************/

#include "threadpool.h"

using namespace std;


template<bool handle_exception>
threadpool<handle_exception>::~threadpool()
{
    stop_on_completed(); // 退出时等待任务清空
    for (auto& handle_obj : m_thread_object)
    {
#if _MSC_VER <= 1800 // Fix std::thread deadlock bug on VS2012,VS2013 (when call join on exit)
        WaitForSingleObject((HANDLE)handle_obj.first.native_handle(), INFINITE);
        handle_obj.first.detach();
#else // Other platform
        handle_obj.first.join(); // 等待所有打开的线程退出
#endif // #if _MSC_VER <= 1800
    }
    for (auto& handle_obj : m_thread_destroy)
    {
#if _MSC_VER <= 1800 // Fix std::thread deadlock bug on VS2012,VS2013 (when call join on exit)
        WaitForSingleObject((HANDLE)handle_obj.first.native_handle(), INFINITE);
        handle_obj.first.detach();
#else // Other platform
        handle_obj.first.join(); // 等待所有已销毁分离的线程退出
#endif // #if _MSC_VER <= 1800
    }
}

template<bool handle_exception> // 分离所有任务，设置分离的线程池对象线程数为thread_number_new
bool threadpool<handle_exception>::detach(int thread_number_new)
{
    lock_guard<mutex> lck(m_task_lock); // 当前线程池任务队列读写锁
    if (!m_push_tasks->size())
        return false;
    assert(thread_number_new); // 如果线程数为0，则分离的线程池中未处理的任务将丢弃
    auto detach_threadpool = new threadpool<handle_exception>(thread_number_new);
    lock_guard<mutex> lck_new(detach_threadpool->m_task_lock); // 新线程池任务队列读写锁
    swap(*m_push_tasks, detach_threadpool->m_tasks); // 交换任务队列
    // 通知分离的线程对象运行
    detach_threadpool->notify(detach_threadpool->m_tasks.size());
    async([](decltype(detach_threadpool) pClass){
        delete pClass;
        static const size_t result = success_code + 0xff;
        debug_output(_T("Thread Result: ["), (void*)result, _T("] ["), pClass->this_type().name(), _T("](0x"), pClass, _T(')'));
        return result;
    }, detach_threadpool);
    return true;
}

template<bool handle_exception> // 设置新的处理线程数，退出流程和未初始化的线程池则失败
bool threadpool<handle_exception>::set_new_thread_number(int thread_number_new)
{
    assert(thread_number_new >= 0 && thread_number_new < 255); // Thread number must greater than or equal 0 and less than 255
    switch (m_exit_event.load())
    {
    case exit_event_t::INITIALIZATION: // 未初始化的线程池将失败
        return false;
    case exit_event_t::NORMAL:
    case exit_event_t::PAUSE:
        break;
    default: // 退出流程中禁止操作线程控制事件
        return false;
    }
    if (thread_number_new < 0) // 线程数小于0则失败
        return false;
    if (m_thread_started.load() != thread_number_new)
    {
        // 线程创建、销毁事件锁
        unique_lock<mutex> lck(m_thread_lock);
        for (register int i = m_thread_started.load(); i < thread_number_new; i++)
        {
            HANDLE thread_exit_event = ::CreateEventW(nullptr, FALSE, FALSE, nullptr); // 自动复位，无信号
            auto iter = m_thread_object.insert(m_thread_object.end(), make_pair(
                thread(&threadpool::thread_entry, this, thread_exit_event), SAFE_HANDLE_OBJECT(thread_exit_event)));
            m_thread_started++;
        }
        for (register int i = m_thread_started.load(); i > thread_number_new; i--)
        {
            auto iter = m_thread_object.begin();
            SetEvent(iter->second);
            m_thread_destroy.push_back(move(*iter));
            m_thread_object.erase(iter);
            m_thread_started--;
        }
        lck.unlock();
        assert(m_thread_started.load() == thread_number_new);
    }
    return true;
}


SYSCONAPI threadpool<true>::~threadpool();
SYSCONAPI threadpool<false>::~threadpool();

SYSCONAPI bool threadpool<true>::detach(int thread_number_new);
SYSCONAPI bool threadpool<false>::detach(int thread_number_new);

SYSCONAPI bool threadpool<true>::set_new_thread_number(int thread_number_new);
SYSCONAPI bool threadpool<false>::set_new_thread_number(int thread_number_new);
