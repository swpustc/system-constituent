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
#include <cassert>
#include <typeinfo>
#include <functional>
#include <Windows.h>

enum class thread_priority : uint16_t
{
    uninitialized,
    none,
    time_critical,
    highest,
    above_normal,
    normal,
    below_normal,
    lowest,
    idle,
};


// 线程池类; handle_exception: 是否处理捕获任务异常
template<bool handle_exception = true> class threadpool
{
private:
    // 线程数
    ::std::atomic<int> m_thread_number{ 0 };
    ::std::atomic<int> m_thread_started{ 0 };
    // 线程队列
    ::std::list<::std::tuple<::std::thread, SAFE_HANDLE_OBJECT, SAFE_HANDLE_OBJECT>> m_thread_object;
    // 已销毁分离的线程对象
    ::std::list<::std::tuple<::std::thread, SAFE_HANDLE_OBJECT, SAFE_HANDLE_OBJECT>> m_thread_destroy;
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
    ::std::recursive_mutex m_thread_lock;
    // 通知事件
    SAFE_HANDLE_OBJECT m_stop_thread; // 退出事件，关闭所有线程
    SAFE_HANDLE_OBJECT m_notify_task; // 通知线程有新任务
    // 线程优先级
    thread_priority m_priority = thread_priority::uninitialized;

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
    static size_t thread_entry(threadpool* object, HANDLE pause_event, HANDLE resume_event);
    // 线程入口函数，线程启动时先执行一次启动函数
    static size_t thread_entry_startup(threadpool* object, HANDLE pause_event, HANDLE resume_event, ::std::function<void()>& startup_fn);
    // 线程运行前准备
    size_t pre_run(HANDLE pause_event, HANDLE resume_event);
    /* 线程任务调度函数
    *  返回值 >= success_code 表示正常退出，< success_code 为非正常退出
    *  run函数体本身堆栈中没有对象，移动ebp/rbp寄存器安全，可以不处理异常
    **/
    size_t run(HANDLE pause_event, HANDLE resume_event);

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
    // 线程启动时先执行一次启动函数
    template<class Fn, class... Args> threadpool(int thread_number, Fn&& startup_fn, Args&&... args)
    {
        set_thread_number(thread_number, ::std::forward<Fn>(startup_fn), ::std::forward<Args>(args)...);
    }
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

    // 清理任务队列
    void clear()
    {
        ::std::lock_guard<decltype(m_task_lock)> lck(m_task_lock);
        m_tasks.clear();
        m_pause_tasks.clear();
    }

    // 分离所有任务，分离的线程池对象线程数不变
    void detach()
    {
        detach(m_thread_started);
    }
    // 分离所有任务，设置分离的线程池对象线程数为thread_number_new
    SYSCONAPI void detach(int thread_number_new);
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
    // 初始化线程池，设置处理线程数，已初始化则失败，线程启动时先执行一次启动函数
    template<class Fn, class... Args> bool set_thread_number(int thread_number, Fn&& startup_fn, Args&&... args)
    {
        assert(thread_number >= 0 && thread_number < 255); // Thread number must greater than or equal 0 and less than 255
        switch (m_exit_event.load())
        {
        case exit_event_t::INITIALIZATION: // 未初始化的线程池
            m_exit_event = exit_event_t::NORMAL;
            m_stop_thread = CreateEventW(nullptr, TRUE, FALSE, nullptr);  // 手动复位，无信号
            m_notify_task = CreateEventW(nullptr, FALSE, FALSE, nullptr); // 自动复位，无信号
            break;
        default: // 已初始化的线程池将失败
            return false;
        }
        if (thread_number < 0)
            return false;
        m_thread_number = thread_number;
        return set_new_thread_number(thread_number, ::std::forward<Fn>(startup_fn), ::std::forward<Args>(args)...);
    }
    // 设置新的处理线程数，退出流程和未初始化的线程池则失败
    SYSCONAPI bool set_new_thread_number(int thread_number_new);
    // 设置新的处理线程数，退出流程和未初始化的线程池则失败，线程启动时先执行一次启动函数
    template<class Fn, class... Args> bool set_new_thread_number(int thread_number_new, Fn&& startup_fn, Args&&... args)
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
            // 有创建新线程
            bool already_create_new_thread = false;
            // 线程创建、销毁事件锁
            ::std::unique_lock<decltype(m_thread_lock)> lck(m_thread_lock);
            for (register int i = m_thread_started.load(); i < thread_number_new; i++)
            {
                if (m_thread_destroy.size())
                {
                    auto iter = m_thread_destroy.begin();
                    ::ResetEvent(get<1>(*iter));    // 取消线程暂停事件
                    ::SetEvent(get<2>(*iter));      // 如果线程已暂停则恢复
                    m_thread_object.push_back(move(*iter));
                    m_thread_destroy.erase(iter);
                }
                else
                {
                    already_create_new_thread = true;
                    HANDLE thread_exit_event = CreateEventW(nullptr, FALSE, FALSE, nullptr); // 自动复位，无信号
                    HANDLE thread_resume_event = CreateEventW(nullptr, FALSE, FALSE, nullptr); // 自动复位，无信号
                    // 绑定函数
                    auto task_obj = ::std::make_shared<decltype(::std::bind(::std::forward<Fn>(startup_fn), ::std::forward<Args>(args)...))>(
                        ::std::bind(::std::forward<Fn>(startup_fn), ::std::forward<Args>(args)...));
                    // 生成任务（仿函数）
                    ::std::function<void()> bind_function(::std::bind(function_wapper(), ::std::move(task_obj)));
                    m_thread_object.push_back(make_tuple(
                        thread(thread_entry_startup, this, thread_exit_event, thread_resume_event, ::std::move(bind_function)),
                        SAFE_HANDLE_OBJECT(thread_exit_event),
                        SAFE_HANDLE_OBJECT(thread_resume_event)));
                }
                m_thread_started++;
            }
            // 只在创建新线程时设置优先级
            if (already_create_new_thread)
                set_thread_priority(m_priority);
            for (register int i = m_thread_started.load(); i > thread_number_new; i--)
            {
                auto iter = m_thread_object.begin();
                ::ResetEvent(get<2>(*iter));    // 取消线程恢复事件
                ::SetEvent(get<1>(*iter));      // 如果线程在运行则暂停
                m_thread_destroy.push_back(move(*iter));
                m_thread_object.erase(iter);
                m_thread_started--;
            }
            assert(m_thread_started.load() == thread_number_new);
            lck.unlock();
        }
        return true;
    }

    // 重置线程池数量为初始数量
    bool reset_thread_number()
    {
        return set_new_thread_number(get_default_thread_number());
    }

    // 设置线程优先级
    SYSCONAPI void set_thread_priority(thread_priority priority = thread_priority::uninitialized);
    // 检查运行的线程是否为线程池管理的线程
    bool is_owner()
    {
        return is_owner(::std::this_thread::get_id());
    }
    // 检查指定的线程ID是否为线程池管理的线程
    bool is_owner(const ::std::thread::id& thread_id)
    {
        // 线程创建、销毁事件锁
        ::std::unique_lock<decltype(m_thread_lock)> lck(m_thread_lock);
        if (::std::any_of(m_thread_object.cbegin(), m_thread_object.cend(), [&](decltype(*m_thread_object.cend()) th_obj){
            return thread_id == ::std::get<0>(th_obj).get_id();
        }))
            return true;
        if (::std::any_of(m_thread_destroy.cbegin(), m_thread_destroy.cend(), [&](decltype(*m_thread_destroy.cend()) th_obj){
            return thread_id == ::std::get<0>(th_obj).get_id();
        }))
            return true;
        return false;
    }
};


// 自动等待输入的future完成
template <class future_type>
class auto_wait_future
{
private:
    // 等待的future集合
    ::std::vector<::std::future<future_type>> future_set;
    // 等待的shared_future集合
    ::std::vector<::std::shared_future<future_type>> shared_future_set;

public:
    auto_wait_future() = default;
    auto_wait_future(const auto_wait_future&) = delete;
    auto_wait_future& operator=(const auto_wait_future&) = delete;
    template<typename future_obj>
    auto_wait_future(future_obj&& fut)
    {
        push(::std::forward<future_obj>(fut));
    }
    ~auto_wait_future()
    {   // 等待所有wait完成
        wait();
    }

    void push(const ::std::future<future_type>& fut) = delete;
    void push(::std::future<future_type>&& fut)
    {
        future_set.push_back(::std::move(fut));
    }
    void push(const ::std::pair<::std::future<future_type>, bool>& fut) = delete;
    void push(::std::pair<::std::future<future_type>, bool>&& fut)
    {
        if (fut.second)
            future_set.push_back(::std::move(fut.first));
    }

    ::std::shared_future<future_type> push(const ::std::shared_future<future_type>& fut)
    {
        ::std::shared_future<future_type> result = fut;
        shared_future_set.push_back(result);
        return ::std::move(result);
    }
    ::std::shared_future<future_type> push(::std::shared_future<future_type>&& fut)
    {
        ::std::shared_future<future_type> result = fut;
        shared_future_set.push_back(::std::move(fut));
        return ::std::move(result);
    }
    ::std::shared_future<future_type> push(const ::std::pair<::std::shared_future<future_type>, bool>& fut)
    {
        ::std::shared_future<future_type> result;
        if (fut.second)
        {
            result = fut.first;
            shared_future_set.push_back(result);
        }
        return ::std::move(result);
    }
    ::std::shared_future<future_type> push(::std::pair<::std::shared_future<future_type>, bool>&& fut)
    {
        ::std::shared_future<future_type> result;
        if (fut.second)
        {
            result = fut.first;
            shared_future_set.push_back(::std::move(fut.first));
        }
        return ::std::move(result);
    }

    void wait() const
    {
        for (auto& val : future_set)
            if (val.valid())
                val.wait();
        for (auto& val : shared_future_set)
            if (val.valid())
                val.wait();
    }
    template<class rep, class per>
    void wait_for(const ::std::chrono::duration<rep, per>& rel_time) const
    {
        for (auto& val : future_set)
            if (val.valid())
                val.wait_for(rel_time);
        for (auto& val : shared_future_set)
            if (val.valid())
                val.wait_for(rel_time);
    }
    template<class clock, class dur>
    void wait_until(const ::std::chrono::time_point<clock, dur>& abs_time) const
    {
        for (auto& val : future_set)
            if (val.valid())
                val.wait_until(abs_time);
        for (auto& val : shared_future_set)
            if (val.valid())
                val.wait_until(abs_time);
    }
};


// 自动等待输入的future完成（只支持shared_future）
template <class future_type>
class auto_wait_shared_future
{
private:
    // 等待的shared_future集合
    ::std::vector<::std::shared_future<future_type>> future_set;

public:
    auto_wait_shared_future() = default;
    auto_wait_shared_future(const auto_wait_shared_future&) = delete;
    auto_wait_shared_future& operator=(const auto_wait_shared_future&) = delete;
    template<typename future_obj>
    auto_wait_shared_future(future_obj&& fut)
    {
        push(::std::forward<future_obj>(fut));
    }
    ~auto_wait_shared_future()
    {   // 等待所有wait完成
        wait();
    }

    ::std::shared_future<future_type> push(const ::std::future<future_type>& fut) = delete;
    ::std::shared_future<future_type> push(::std::future<future_type>&& fut)
    {
        ::std::shared_future<future_type> result = fut.share();
        future_set.push_back(result);
        return ::std::move(result);
    }
    ::std::shared_future<future_type> push(const ::std::pair<::std::future<future_type>, bool>& fut) = delete;
    ::std::shared_future<future_type> push(::std::pair<::std::future<future_type>, bool>&& fut)
    {
        ::std::shared_future<future_type> result;
        if (fut.second)
        {
            result = fut.first.share();
            future_set.push_back(result);
        }
        return ::std::move(result);
    }

    ::std::shared_future<future_type> push(const ::std::shared_future<future_type>& fut)
    {
        ::std::shared_future<future_type> result = fut;
        future_set.push_back(result);
        return ::std::move(result);
    }
    ::std::shared_future<future_type> push(::std::shared_future<future_type>&& fut)
    {
        ::std::shared_future<future_type> result = fut;
        future_set.push_back(::std::move(fut));
        return ::std::move(result);
    }
    ::std::shared_future<future_type> push(const ::std::pair<::std::shared_future<future_type>, bool>& fut)
    {
        ::std::shared_future<future_type> result;
        if (fut.second)
        {
            result = fut.first;
            future_set.push_back(result);
        }
        return ::std::move(result);
    }
    ::std::shared_future<future_type> push(::std::pair<::std::shared_future<future_type>, bool>&& fut)
    {
        ::std::shared_future<future_type> result;
        if (fut.second)
        {
            result = fut.first;
            future_set.push_back(::std::move(fut.first));
        }
        return ::std::move(result);
    }

    void wait() const
    {
        for (auto& val : future_set)
            if (val.valid())
                val.wait();
    }
    template<class rep, class per>
    void wait_for(const ::std::chrono::duration<rep, per>& rel_time) const
    {
        for (auto& val : future_set)
            if (val.valid())
                val.wait_for(rel_time);
    }
    template<class clock, class dur>
    void wait_until(const ::std::chrono::time_point<clock, dur>& abs_time) const
    {
        for (auto& val : future_set)
            if (val.valid())
                val.wait_until(abs_time);
    }
};
