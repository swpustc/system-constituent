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
    // 线程是否已启动
    bool m_is_start = false;
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
    ::std::atomic<size_t> m_task_exception{ 0 };
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
    { // 最多通知3个线程
        auto&& i = auto_min(3UL, (size_t)m_thread_started.load(), attach_tasks_number);
        while (i--)
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

    // 设置新的处理线程数，退出流程和未初始化的线程池则失败，线程启动时先执行一次启动函数
    SYSCONAPI bool _set_new_thread_number(int thread_number_new, ::std::function<void()>&& startup_fn);

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
    // 添加一个任务集合
    size_t push_tasks(decltype(m_tasks)&& tasks)
    {
        switch (m_exit_event.load())
        {
        case exit_event_t::NORMAL:
        case exit_event_t::PAUSE:
        case exit_event_t::INITIALIZATION: // 未初始化的线程池仍然可以添加任务
            break;
        default: // 退出流程中禁止操作线程控制事件
            return false;
        }
        auto&& count = tasks.size();
        if (count)
        {
            // 任务队列读写锁
            ::std::unique_lock<decltype(m_task_lock)> lck(m_task_lock);
            while (!tasks.empty())
            {
                m_push_tasks->push_back(::std::move(tasks.back()));
                tasks.pop_back();
            }
            lck.unlock();
            m_task_all += count;
            notify(count);
        }
        return count;
    }

    // 清理任务队列
    void clear()
    {
        ::std::lock_guard<decltype(m_task_lock)> lck(m_task_lock);
        // 清理的任务从添加的任务总数中减去
        m_task_all -= m_tasks.size();
        m_task_all -= m_pause_tasks.size();
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
    // 销毁线程池。WARNING: 线程会被直接分离，可能会造成资源泄露!!!
    SYSCONAPI void destroy();

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
    // 获取异常任务数
    size_t get_tasks_exception_number() const
    {
        return m_task_exception.load();
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
        decltype(m_tasks) exception_tasks;
        m_exception_tasks.swap(exception_tasks);
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
        // 绑定函数
        auto task_obj = ::std::make_shared<decltype(::std::bind(::std::forward<Fn>(startup_fn), ::std::forward<Args>(args)...))>(
            ::std::bind(::std::forward<Fn>(startup_fn), ::std::forward<Args>(args)...));
        // 生成任务（仿函数）
        ::std::function<void()> bind_function(::std::bind(function_wapper(), ::std::move(task_obj)));
        return _set_new_thread_number(thread_number_new, ::std::move(bind_function));
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
        if (::std::any_of(m_thread_object.cbegin(), m_thread_object.cend(), [&](decltype(*m_thread_object.cend()) th_obj){ return thread_id == ::std::get<0>(th_obj).get_id(); }))
            return true;
        if (::std::any_of(m_thread_destroy.cbegin(), m_thread_destroy.cend(), [&](decltype(*m_thread_destroy.cend()) th_obj){ return thread_id == ::std::get<0>(th_obj).get_id(); }))
            return true;
        return false;
    }
    bool is_start() const
    {
        return m_is_start;
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

    void clear()
    {
        future_set.clear();
        shared_future_set.clear();
    }
    void clear_on_complete()
    {
        wait();
        clear();
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

    void clear()
    {
        future_set.clear();
    }
    void clear_on_complete()
    {
        wait();
        clear();
    }
};



// 线程池展示
class threadpool_view
{
private:
    threadpool<true>* m_thpool_true = nullptr;
    threadpool<false>* m_thpool_false = nullptr;

public:
    threadpool_view() = default;
    template<bool handle_exception> threadpool_view(threadpool<handle_exception>* pointer)
    {
        set_pointer(pointer);
    }
    // 设置线程池指针
    template<bool handle_exception> void set_pointer(threadpool<handle_exception>* pointer);
    // 清空管理的指针
    void clear_pointer()
    {
        m_thpool_true = nullptr;
        m_thpool_false = nullptr;
    }
    // 是否有效
    bool valid() const
    {
        return m_thpool_true || m_thpool_false;
    }
    // 清理任务队列
    void clear()
    {
        if (m_thpool_true)
            m_thpool_true->clear();
        else if (m_thpool_false)
            m_thpool_false->clear();
    }
    // 获取线程数量
    int get_thread_number() const
    {
        if (m_thpool_true)
            return m_thpool_true->get_thread_number();
        else if (m_thpool_false)
            return m_thpool_false->get_thread_number();
        else
            return 0;
    }
    // 获取空闲线程数
    int get_free_thread_number() const
    {
        if (m_thpool_true)
            return m_thpool_true->get_free_thread_number();
        else if (m_thpool_false)
            return m_thpool_false->get_free_thread_number();
        else
            return 0;
    }
    // 获取任务队列数量
    size_t get_tasks_number() const
    {
        if (m_thpool_true)
            return m_thpool_true->get_tasks_number();
        else if (m_thpool_false)
            return m_thpool_false->get_tasks_number();
        else
            return 0;
    }
    // 获取异常任务数
    size_t get_tasks_exception_number() const
    {
        if (m_thpool_true)
            return m_thpool_true->get_tasks_exception_number();
        else if (m_thpool_false)
            return m_thpool_false->get_tasks_exception_number();
        else
            return 0;
    }
    // 获取已完成任务数
    size_t get_tasks_completed_number() const
    {
        if (m_thpool_true)
            return m_thpool_true->get_tasks_completed_number();
        else if (m_thpool_false)
            return m_thpool_false->get_tasks_completed_number();
        else
            return 0;
    }
    // 获取已添加任务总数
    size_t get_tasks_total_number() const
    {
        if (m_thpool_true)
            return m_thpool_true->get_tasks_total_number();
        else if (m_thpool_false)
            return m_thpool_false->get_tasks_total_number();
        else
            return 0;
    }
    // 获取初始化线程数
    int get_default_thread_number() const
    {
        if (m_thpool_true)
            return m_thpool_true->get_default_thread_number();
        else if (m_thpool_false)
            return m_thpool_false->get_default_thread_number();
        else
            return 0;
    }
};

// 设置线程池指针(threadpool<true>)
template<> inline void threadpool_view::set_pointer(threadpool<true>* pointer)
{
    m_thpool_true = pointer;
    m_thpool_false = nullptr;
}

// 设置线程池指针(threadpool<false>)
template<> inline void threadpool_view::set_pointer(threadpool<false>* pointer)
{
    m_thpool_true = nullptr;
    m_thpool_false = pointer;
}



// 多个线程池展示
class threadpool_multi_view
{
private:
    ::std::vector<threadpool_view> m_thpool;

public:
    threadpool_multi_view() = default;
    template<bool handle_exception> threadpool_multi_view(threadpool<handle_exception>* pointer)
    {
        set_pointer(pointer);
    }
    // 添加线程池指针
    template<bool handle_exception> void set_pointer(threadpool<handle_exception>* pointer)
    {
        if (pointer)
            m_thpool.push_back(threadpool_view(pointer));
    }
    // 是否有效
    bool valid() const
    {
        return !m_thpool.empty();
    }
    // 清空管理的指针
    void clear_pointer()
    {
        m_thpool.clear();
    }
    // 清理任务队列
    void clear()
    {
        for (auto& th : m_thpool)
            th.clear();
    }
    // 获取线程数量
    int get_thread_number() const
    {
        int result = 0;
        for (auto& th : m_thpool)
            result += th.get_thread_number();
        return result;
    }
    // 获取空闲线程数
    int get_free_thread_number() const
    {
        int result = 0;
        for (auto& th : m_thpool)
            result += th.get_free_thread_number();
        return result;
    }
    // 获取任务队列数量
    size_t get_tasks_number() const
    {
        size_t result = 0;
        for (auto& th : m_thpool)
            result += th.get_tasks_number();
        return result;
    }
    // 获取异常任务数
    size_t get_tasks_exception_number() const
    {
        size_t result = 0;
        for (auto& th : m_thpool)
            result += th.get_tasks_exception_number();
        return result;
    }
    // 获取已完成任务数
    size_t get_tasks_completed_number() const
    {
        size_t result = 0;
        for (auto& th : m_thpool)
            result += th.get_tasks_completed_number();
        return result;
    }
    // 获取已添加任务总数
    size_t get_tasks_total_number() const
    {
        size_t result = 0;
        for (auto& th : m_thpool)
            result += th.get_tasks_total_number();
        return result;
    }
    // 获取初始化线程数
    int get_default_thread_number() const
    {
        int result = 0;
        for (auto& th : m_thpool)
            result += th.get_default_thread_number();
        return result;
    }
};
