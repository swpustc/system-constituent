/**********************************************************
* 线程池控制类
* 支持平台：Windows
* 编译环境：VS2013+
***********************************************************/

#pragma once

#include "common.h"
#include "safe_object.h"
#include <list>
#include <deque>
#include <mutex>
#include <memory>
#include <atomic>
#include <future>
#include <thread>
#include <vector>
#include <cassert>
#include <typeinfo>
#include <functional>
#include <Windows.h>


// 线程池类; handle_exception: 是否处理捕获任务异常
template<bool handle_exception = true> class threadpool
{
private:
    // 线程数
    ::std::atomic<int> m_thread_number{ 0 };
    ::std::atomic<int> m_thread_started{ 0 };
    // 线程队列
    ::std::list<::std::pair<::std::thread, SAFE_HANDLE_OBJECT>> m_thread_object;
    // 已销毁分离的线程对象
    ::std::vector<::std::pair<::std::thread, SAFE_HANDLE_OBJECT>> m_thread_destroy;
    // 任务队列
    ::std::deque<::std::function<void()>> m_tasks;
    decltype(m_tasks) m_pause_tasks;
    decltype(m_tasks)* m_push_tasks{ &m_tasks };
    decltype(m_tasks) m_exception_tasks;
    ::std::atomic<size_t> m_task_completed{ 0 };
    ::std::atomic<size_t> m_task_all{ 0 };
    // 任务队列读写锁
    mutable spin_mutex m_task_lock;
    // 线程创建、销毁事件锁
    ::std::mutex m_thread_lock;
    // 通知事件
    SAFE_HANDLE_OBJECT m_stop_thread; // 退出事件，关闭所有线程
    SAFE_HANDLE_OBJECT m_notify_task; // 通知线程有新任务

    enum class exit_event_t {
        INITIALIZATION,
        NORMAL,
        PAUSE,
        STOP_IMMEDIATELY,
        WAIT_TASK_COMPLETE,
    };
    // 退出任务事件
    ::std::atomic<exit_event_t> m_exit_event{ exit_event_t::INITIALIZATION };

    // 线程入口函数
    static size_t thread_entry(threadpool* object, HANDLE exit_event);
    /* 线程任务调度函数
    * 返回值 >= success_code 表示正常退出，< success_code 为非正常退出
    * run函数体本身堆栈中没有对象，移动ebp/rbp寄存器安全，可以不处理异常
    **/
    size_t run(HANDLE exit_event)
    {
        // 线程通知事件
        HANDLE handle_notify[] = { exit_event, m_stop_thread, m_notify_task };
        while (true)
        {
            // 监听线程通知事件
            switch (WaitForMultipleObjects(sizeof(handle_notify) / sizeof(HANDLE), handle_notify, FALSE, INFINITE))
            {
            case WAIT_OBJECT_0:     // 退出当前线程
                return success_code + 0;
            case WAIT_OBJECT_0 + 1: // 正常退出事件
                switch (m_exit_event.load())
                {
                case exit_event_t::INITIALIZATION:      // 线程池对象未准备好
                    return success_code - 0xff;
                case exit_event_t::NORMAL:              // 未设置退出事件
                    return success_code + 1;
                case exit_event_t::PAUSE:               // 线程暂停中
                    return success_code + 2;
                case exit_event_t::STOP_IMMEDIATELY:    // 立即退出
                    return success_code + 3;
                case exit_event_t::WAIT_TASK_COMPLETE:  // 等待任务队列清空
                default:
                    break;
                }
            case WAIT_OBJECT_0 + 2: // 当前线程激活
                while (true)
                {
                    if (!run_task(get_task())) // 任务队列中没有任务
                    {
                        if (m_exit_event == exit_event_t::WAIT_TASK_COMPLETE)
                            return success_code + 4;
                        break;
                    }
                }
                break;
            case WAIT_FAILED:       // 错误
                return success_code - 3;
            default:
                return success_code - 4;
            }
        }
    }

    // 新任务添加通知
    void notify()
    { // 通知一个线程
        ::SetEvent(m_notify_task);
    }
    void notify(size_t attach_tasks_number)
    { // 通知一个线程
        int i = auto_min(3, m_thread_started.load());
        while (attach_tasks_number-- && i--)
            notify();
    }

    // 获取任务队列中的任务，返回当前任务和未执行任务总数
    ::std::pair<::std::function<void()>, size_t> get_task()
    {
        ::std::function<void()> task;
        ::std::unique_lock<decltype(m_task_lock)> lck(m_task_lock); // 任务队列读写锁
        size_t task_num = m_tasks.size();
        if (task_num)
        {
            ::std::swap(task, m_tasks.front());
            m_tasks.pop_front();
            lck.unlock();
            return ::std::make_pair(::std::move(task), task_num);
        }
        else
        {
            lck.unlock();
            return ::std::make_pair(::std::move(task), 0);
        }
    }
    // 运行一条任务，返回任务队列中是否还有任务[true:有任务; false:没任务]
    bool run_task(::std::pair<::std::function<void()>, size_t>&& task_val);

public:
    // 标准线程结束代码
    static const size_t success_code = 0x00001000;

    threadpool(){}
    threadpool(int thread_number){ set_thread_number(thread_number); }
    SYSCONAPI ~threadpool();
    // 复制构造函数
    threadpool(const threadpool&) = delete;
    // 复制赋值语句
    threadpool& operator=(const threadpool&) = delete;
    // 移动构造函数
    threadpool(threadpool&&) = delete;
    // 移动赋值语句
    threadpool& operator=(threadpool&&) = delete;

    // 启动暂停pause的线程
    bool start()
    {
        // 任务队列读写锁
        ::std::unique_lock<decltype(m_task_lock)> lck(m_task_lock);
        switch (m_exit_event.load())
        {
        case exit_event_t::PAUSE:
            m_exit_event = exit_event_t::NORMAL;
            ::std::swap(m_tasks, m_pause_tasks);
            m_push_tasks = &m_tasks;
            lck.unlock();
            notify(m_tasks.size());
            assert(m_pause_tasks.size() == 0);
        case exit_event_t::NORMAL:
            return true;
            // 退出流程中禁止操作线程控制事件
        case exit_event_t::INITIALIZATION:
        default:
            return false;
        }
    }
    // 暂停线程执行，直到退出stop或者启动start
    bool pause()
    {
        // 任务队列读写锁
        ::std::unique_lock<decltype(m_task_lock)> lck(m_task_lock);
        switch (m_exit_event.load())
        {
        case exit_event_t::NORMAL:
            m_exit_event = exit_event_t::PAUSE;
            ::std::swap(m_tasks, m_pause_tasks);
            m_push_tasks = &m_pause_tasks;
            lck.unlock();
            assert(m_tasks.size() == 0);
        case exit_event_t::PAUSE:
            return true;
        case exit_event_t::INITIALIZATION:
        default: // 退出流程中禁止操作线程控制事件
            return false;
        }
    }
    // 立即结束任务，未处理的任务将丢弃
    void stop()
    {
        switch (m_exit_event.load())
        {
        case exit_event_t::NORMAL:
        case exit_event_t::WAIT_TASK_COMPLETE:
            // 任务队列读写锁
            m_task_lock.lock();
            ::std::swap(m_tasks, m_pause_tasks);
            m_push_tasks = &m_pause_tasks;
            m_task_lock.unlock();
            assert(m_tasks.size() == 0);
        case exit_event_t::PAUSE:
            m_exit_event = exit_event_t::STOP_IMMEDIATELY;
            ::SetEvent(m_stop_thread);
        case exit_event_t::INITIALIZATION:
        default:
            break;
        }
    }
    // 当任务队列执行完毕后退出线程，如果线程池正在退出则失败
    bool stop_on_completed()
    {
        switch (m_exit_event.load())
        {
        case exit_event_t::PAUSE:
            start();
        case exit_event_t::NORMAL:
            m_exit_event = exit_event_t::WAIT_TASK_COMPLETE;
            ::SetEvent(m_stop_thread);
        case exit_event_t::WAIT_TASK_COMPLETE:
            return true;
        case exit_event_t::INITIALIZATION:
        default:
            return false;
        }
    }

    // 添加一个任务
    template<class Fn, class... Args> bool push(Fn&& fn, Args&&... args)
    {
        typedef decltype(decay_type(::std::forward<Fn>(fn))(decay_type(::std::forward<Args>(args))...)) result_type;
        switch (m_exit_event.load())
        {
        case exit_event_t::NORMAL:
        case exit_event_t::PAUSE:
        case exit_event_t::INITIALIZATION: // 未初始化的线程池仍然可以添加任务
            break;
        default: // 退出流程中禁止操作线程控制事件
            return false;
        }
        // 绑定函数
        auto task_obj = ::std::make_shared<decltype(::std::bind(::std::forward<Fn>(fn), ::std::forward<Args>(args)...))>(
            ::std::bind(::std::forward<Fn>(fn), ::std::forward<Args>(args)...));
        // 生成任务（仿函数）
        ::std::function<void()> bind_function(::std::bind(function_wapper(), ::std::move(task_obj)));
        // 任务队列读写锁
        ::std::unique_lock<decltype(m_task_lock)> lck(m_task_lock);
        m_push_tasks->push_back(::std::move(bind_function));
        lck.unlock();
        m_task_all++;
        notify();
        return true;
    }
    // 添加一个任务并返回返回值对象pair<future,bool>，使用future::get获取返回值（若未完成会等待完成）
    template<class Fn, class... Args> auto push_future(Fn&& fn, Args&&... args)
        -> ::std::pair<::std::future<decltype(decay_type(::std::forward<Fn>(fn))(decay_type(::std::forward<Args>(args))...))>, bool>
    {
        typedef decltype(decay_type(::std::forward<Fn>(fn))(decay_type(::std::forward<Args>(args))...)) result_type;
        ::std::future<result_type> future_obj;
        switch (m_exit_event.load())
        {
        case exit_event_t::NORMAL:
        case exit_event_t::PAUSE:
        case exit_event_t::INITIALIZATION: // 未初始化的线程池仍然可以添加任务
            break;
        default: // 退出流程中禁止操作线程控制事件
            return ::std::make_pair(::std::move(future_obj), false);
        }
        // 绑定函数
        auto task_obj = ::std::make_shared<::std::packaged_task<result_type()>>(
            ::std::bind(::std::forward<Fn>(fn), ::std::forward<Args>(args)...));
        future_obj = task_obj->get_future();
        // 生成任务（仿函数）
        ::std::function<void()> bind_function(::std::bind(function_wapper(), ::std::move(task_obj)));
        // 任务队列读写锁
        ::std::unique_lock<decltype(m_task_lock)> lck(m_task_lock);
        m_push_tasks->push_back(::std::move(bind_function));
        lck.unlock();
        m_task_all++;
        notify();
        return ::std::make_pair(::std::move(future_obj), true);
    }
    // 添加多个任务
    template<class Fn, class... Args> bool push_multi(size_t count, Fn&& fn, Args&&... args)
    {
        typedef decltype(decay_type(::std::forward<Fn>(fn))(decay_type(::std::forward<Args>(args))...)) result_type;
        assert(count >= 0 && count < USHRT_MAX);
        switch (m_exit_event.load())
        {
        case exit_event_t::NORMAL:
        case exit_event_t::PAUSE:
        case exit_event_t::INITIALIZATION: // 未初始化的线程池仍然可以添加任务
            break;
        default: // 退出流程中禁止操作线程控制事件
            return false;
        }
        if (count)
        {
            // 绑定函数
            auto task_obj = ::std::make_shared<decltype(::std::bind(::std::forward<Fn>(fn), ::std::forward<Args>(args)...))>(
                ::std::bind(::std::forward<Fn>(fn), ::std::forward<Args>(args)...));
            // 生成任务（仿函数）
            ::std::function<void()> bind_function(::std::bind(function_wapper(), ::std::move(task_obj)));
            // 任务队列读写锁
            ::std::unique_lock<decltype(m_task_lock)> lck(m_task_lock);
            m_push_tasks->insert(m_push_tasks->end(), count, ::std::move(bind_function));
            lck.unlock();
            m_task_all += count;
            notify(count);
        }
        return true;
    }
    // 添加多个任务并返回返回值对象pair<vector<future>,bool>，使用future::get获取返回值（若未完成会等待完成）
    template<class Fn, class... Args> auto push_multi_future(size_t count, Fn&& fn, Args&&... args)
        -> ::std::pair<::std::vector<::std::future<decltype(decay_type(::std::forward<Fn>(fn))(decay_type(::std::forward<Args>(args))...))>>, bool>
    {
        typedef decltype(decay_type(::std::forward<Fn>(fn))(decay_type(::std::forward<Args>(args))...)) result_type;
        assert(count >= 0 && count < USHRT_MAX);
        ::std::vector<::std::future<result_type>> future_obj;
        switch (m_exit_event.load())
        {
        case exit_event_t::NORMAL:
        case exit_event_t::PAUSE:
        case exit_event_t::INITIALIZATION: // 未初始化的线程池仍然可以添加任务
            break;
        default: // 退出流程中禁止操作线程控制事件
            return ::std::make_pair(::std::move(future_obj), false);
        }
        if (count)
        {
            future_obj.reserve(count);
            while (count--)
            {
                // 绑定函数
                auto task_obj = ::std::make_shared<::std::packaged_task<result_type()>>(
                    ::std::bind(::std::forward<Fn>(fn), ::std::forward<Args>(args)...));
                future_obj.push_back(task_obj->get_future());
                // 生成任务（仿函数）
                ::std::function<void()> bind_function(::std::bind(function_wapper(), ::std::move(task_obj)));
                // 任务队列读写锁
                ::std::lock_guard<decltype(m_task_lock)> lck(m_task_lock);
                m_push_tasks->push_back(::std::move(bind_function));
            }
            m_task_all += count;
            notify(count);
        }
        return ::std::make_pair(::std::move(future_obj), true);
    }

    // 分离所有任务，分离的线程池对象线程数不变
    bool detach()
    {
        return detach(m_thread_started);
    }
    // 分离所有任务，设置分离的线程池对象线程数为thread_number_new
    SYSCONAPI bool detach(int thread_number_new);
    // 分离任务，并得到分离任务执行情况的future
    ::std::future<size_t> detach_future()
    {
        return detach_future(m_thread_started);
    }
    // 分离任务，设置分离的线程池对象线程数为thread_number_new，并得到分离任务执行情况的future
    SYSCONAPI ::std::future<size_t> detach_future(int thread_number_new);

    // 获取线程数量
    int get_thread_number() const
    {
        return m_thread_started.load();
    }
    // 获取空闲线程数
    int get_free_thread_number() const
    {
        if (get_tasks_number())
            return 0;
        else
        {
            size_t run_tasks = get_tasks_total_number() - get_tasks_completed_number();
            int thread_num = get_thread_number();
            return (int)run_tasks >= thread_num ? 0 : thread_num - (int)run_tasks;
        }
    }
    // 获取任务队列数量
    size_t get_tasks_number() const
    {
        ::std::lock_guard<decltype(m_task_lock)> lck(m_task_lock); // 任务队列读写锁
        return m_push_tasks->size();
    }
    // 获取已完成任务数
    size_t get_tasks_completed_number() const
    {
        return m_task_completed.load();
    }
    // 获取已添加任务总数
    size_t get_tasks_total_number() const
    {
        return m_task_all.load();
    }

    // 获取异常任务队列
    decltype(m_tasks) get_exception_tasks()
    {
        decltype(m_tasks) exception_tasks = ::std::move(m_exception_tasks);
        return ::std::move(exception_tasks);
    }
    // 获取初始化线程数
    int get_default_thread_number() const
    {
        return m_thread_number.load();
    }
    // 获取类型信息
    static const type_info& this_type()
    {
        return typeid(threadpool);
    }

    // 初始化线程池，设置处理线程数，已初始化则失败
    SYSCONAPI bool set_thread_number(int thread_number);
    // 设置新的处理线程数，退出流程和未初始化的线程池则失败
    SYSCONAPI bool set_new_thread_number(int thread_number_new);
    // 重置线程池数量为初始数量
    bool reset_thread_number()
    {
        return set_new_thread_number(get_default_thread_number());
    }
};


template<> inline // 运行一条任务，返回任务队列中是否还有任务[true:有任务; false:没任务]，捕获异常
bool threadpool<true>::run_task(::std::pair<::std::function<void()>, size_t>&& task_val)
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
        catch (...)
        {
            throw ::std::move(task_val.first);
            return false;
        }
        m_task_completed++;
    }
    return task_val.second > 1;
}

template<> inline // 运行一条任务，返回任务队列中是否还有任务[true:有任务; false:没任务]，不捕获异常
bool threadpool<false>::run_task(::std::pair<::std::function<void()>, size_t>&& task_val)
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
