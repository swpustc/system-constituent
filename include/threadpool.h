/**********************************************************
 * 线程池控制类
 * 支持平台：Windows
 * 编译环境：VS2013+
 * 创建时间：2015-04-05 （宋万鹏）
 * 最后修改：2015-04-12 （宋万鹏）
 **********************************************************/

#ifndef __THREADPOOL_H__
#define __THREADPOOL_H__
#pragma once

#include "common.h"
#include "safe_object.h"
#include <list>
#include <queue>
#include <mutex>
#include <atomic>
#include <future>
#include <thread>
#include <cassert>
#include <functional>
#include <process.h>


// 线程池类 thread_number初始化线程数量
template<int thread_number = 2> class threadpool
{
private:
    // 线程数
    ::std::atomic<int> m_thread_started{ 0 };
    // 线程句柄队列
    ::std::list<::std::pair<::std::thread, SAFE_HANDLE_OBJECT>> m_thread_object;
    // 任务队列 (VS2010+)
    ::std::queue<::std::function<void()>> m_tasks;
    ::std::atomic<size_t> m_task_completed{ 0 };
    ::std::atomic<size_t> m_task_all{ 0 };
    // 任务队列读写锁 (VS2012+)
    ::std::mutex m_task_lock;
    // 线程创建、销毁事件锁
    ::std::mutex m_thread_lock;
    // 通知事件
    SAFE_HANDLE_OBJECT m_stop;       // 退出事件，关闭所有线程
    SAFE_HANDLE_OBJECT m_continue;   // 暂停事件，控制线程挂起
    SAFE_HANDLE_OBJECT m_notify_one; // 通知一条线程有新任务
    SAFE_HANDLE_OBJECT m_notify_all; // 通知所有线程有新任务
    // 退出任务事件 (VS2013+)
    enum class exit_event_t {
        NORMAL,
        STOP_IMMEDIATELY,
        WAIT_TASK_COMPLETE
    } m_exit_event{ exit_event_t::NORMAL };

    // 线程入口函数
    size_t thread_entry(::std::list<::std::pair<::std::thread, SAFE_HANDLE_OBJECT>>::iterator thread_iter)
    {
        try
        {
            return run(thread_iter->second.get_safe_handle());
        }
        catch (...)
        {
            // 线程创建、销毁事件锁
            ::std::lock_guard<::std::mutex> lck(m_thread_lock);
            auto iter = m_thread_object.insert(m_thread_object.end(), ::std::make_pair(
                ::std::thread(), CreateEvent(nullptr, FALSE, FALSE, nullptr))); // 自动复位，无信号
            iter->first = ::std::thread(&threadpool::thread_entry, this, iter);
            m_thread_object.erase(thread_iter);
            return success_code - 0xff;
        }
    }
    /* 线程任务调度函数
     * 返回值 >= success_code 表示正常退出，< success_code 为非正常退出
     */
    size_t run(HANDLE exit_event)
    {
        // 线程控制事件
        HANDLE handle_control[] = { exit_event, m_stop, m_continue };
        // 线程通知事件
        HANDLE handle_notify[] = { exit_event, m_stop, m_notify_all, m_notify_one };
        // 任务内容
        decltype(get_task()) task_val;
        while (true)
        {
            // 监听线程控制事件
            switch (WaitForMultipleObjects(sizeof(handle_control) / sizeof(HANDLE), handle_control, FALSE, INFINITE))
            {
            case WAIT_OBJECT_0:     // 退出当前线程
                return success_code + 0;
            case WAIT_OBJECT_0 + 1: // 正常退出事件
                switch (m_exit_event)
                {
                case exit_event_t::NORMAL:                // 未设置退出事件
                    return success_code + 1;
                case exit_event_t::STOP_IMMEDIATELY:      // 立即退出
                    return success_code + 2;
                case exit_event_t::WAIT_TASK_COMPLETE: // 等待任务队列清空
                default:
                    break;
                }
            case WAIT_OBJECT_0 + 2: // 继续运行
                break;
            case WAIT_FAILED:       // 错误
                return  success_code - 1;
            default:
                return success_code - 2;
            }
            // 监听线程通知事件
            switch (WaitForMultipleObjects(sizeof(handle_notify) / sizeof(HANDLE), handle_notify, FALSE, INFINITE))
            {
            case WAIT_OBJECT_0:     // 退出当前线程
                return success_code + 3;
            case WAIT_OBJECT_0 + 1: // 正常退出事件
                switch (m_exit_event)
                {
                case exit_event_t::NORMAL:                // 未设置退出事件
                    return success_code + 4;
                case exit_event_t::STOP_IMMEDIATELY:      // 立即退出
                    return success_code + 5;
                case exit_event_t::WAIT_TASK_COMPLETE: // 等待任务队列清空
                default:
                    break;
                }
            case WAIT_OBJECT_0 + 2: // 当前激活了所有线程
            case WAIT_OBJECT_0 + 3: // 当前激活了单个线程
                task_val = get_task();
                // 任务队列中仍然有任务未处理，发送线程启动通知
                if (task_val.second > 1)
                {
                    notify<1>();
                    task_val.first();
                    m_task_completed++;
                } // 这是当前任务队列中最后一项任务
                else if (task_val.second)
                {
                    task_val.first();
                    if (m_exit_event == exit_event_t::WAIT_TASK_COMPLETE)
                        return success_code + 6;
                } // 任务队列中没有任务
                else if (m_exit_event == exit_event_t::WAIT_TASK_COMPLETE)
                    return success_code + 7;
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
    { // 通知所有线程
        PulseEvent(m_notify_all);
        SetEvent(m_notify_one);
    }
    void notify(size_t attach_count)
    { // 如果添加任务数超过线程数的一半，则通知所有线程，否则只通知一条线程
        if (attach_count)
        {
            if (attach_count > auto_max((size_t)m_thread_started.load() / 2, 1))
                PulseEvent(m_notify_all);
            SetEvent(m_notify_one);
        }
    }
    template<size_t attach_count> void notify()
    { // 和上一个通知函数一致，通过模板以优化确定添加数量时的性能
        if (attach_count)
        {
            if (attach_count > auto_max((size_t)m_thread_started.load() / 2, 1))
                PulseEvent(m_notify_all);
            SetEvent(m_notify_one);
        }
    }

    // 获取任务队列中的任务，返回当前任务和未执行任务总数
    ::std::pair<::std::function<void()>, size_t> get_task()
    {
        ::std::function<void()> task;
        ::std::lock_guard<::std::mutex> lck(m_task_lock); // 任务队列读写锁
        size_t task_num = m_tasks.size();
        if (task_num)
        {
            ::std::swap(task, m_tasks.front());
            m_tasks.pop();
            return ::std::make_pair(::std::move(task), task_num);
        }
        else
            return ::std::make_pair(::std::move(task), 0);
    }

public:
    // 标准线程结束代码
    static const size_t success_code = 0x00001000;

    threadpool() : threadpool(thread_number)
    {
        static_assert(thread_number >= 0 && thread_number < 255, "Thread number must greater than or equal 0 and less than 255");
    }
    threadpool(int _thread_number)
    {
        assert(_thread_number >= 0 && _thread_number < 255);
        m_stop = CreateEvent(nullptr, TRUE, FALSE, nullptr);        // 手动复位，无信号
        m_continue = CreateEvent(nullptr, TRUE, TRUE, nullptr);     // 手动复位，有信号
        m_notify_one = CreateEvent(nullptr, FALSE, FALSE, nullptr); // 自动复位，无信号
        m_notify_all = CreateEvent(nullptr, TRUE, FALSE, nullptr);  // 手动复位，无信号

        // 线程创建、销毁事件锁
        ::std::lock_guard<::std::mutex> lck(m_thread_lock);
        for (int i = 0; i < _thread_number; i++) // 线程对象
        {
            auto iter = m_thread_object.insert(m_thread_object.end(), ::std::make_pair(
                ::std::thread(), CreateEvent(nullptr, FALSE, FALSE, nullptr))); // 自动复位，无信号
            iter->first = ::std::thread(&threadpool::thread_entry, this, ::std::move(iter));
            m_thread_started++;
        }
    }
    ~threadpool()
    {
        if (m_exit_event == exit_event_t::NORMAL)
            stop_on_completed(); // 退出时等待任务清空
        for (auto& handle_obj : m_thread_object) // VS2013+
            handle_obj.first.join(); // 等待所有打开的线程退出
    }
    // 复制构造函数
    threadpool(const threadpool&) = delete;
    template<size_t _thread_number> threadpool(const threadpool<_thread_number>&) = delete;
    // 复制赋值语句
    threadpool& operator=(const threadpool&) = delete;
    template<size_t _thread_number> threadpool& operator=(const threadpool<_thread_number>&) = delete;
    // 移动构造函数
    threadpool(threadpool&&) = delete;
    template<size_t _thread_number> threadpool(threadpool<_thread_number>&&) = delete;
    // 移动赋值语句
    threadpool& operator=(threadpool&&) = delete;
    template<size_t _thread_number> threadpool& operator=(threadpool<_thread_number>&&) = delete;

    // 启动暂停pause的线程
    bool start()
    {
        // 退出流程中禁止操作线程控制事件
        if (m_exit_event != exit_event_t::NORMAL)
            return false;
        SetEvent(m_continue);
        return true;
    }
    // 暂停线程执行，直到退出stop或者启动start
    bool pause()
    {
        // 退出流程中禁止操作线程控制事件
        if (m_exit_event != exit_event_t::NORMAL)
            return false;
        ResetEvent(m_continue);
        notify();
        return true;
    }
    // 立即结束任务，未处理的任务将丢弃
    void stop()
    {
        m_exit_event = exit_event_t::STOP_IMMEDIATELY;
        SetEvent(m_stop);
    }
    // 当任务队列执行完毕后退出线程，如果线程池正在退出则失败
    bool stop_on_completed()
    {
        if (m_exit_event == exit_event_t::WAIT_TASK_COMPLETE)
            return true;
        else if (m_exit_event != exit_event_t::NORMAL)
            return false;
        m_exit_event = exit_event_t::WAIT_TASK_COMPLETE;
        SetEvent(m_continue); // 如果线程已暂停，则启动
        SetEvent(m_stop);
        return true;
    }

    // 添加一个任务 VS2013+
    template<class Fn, class... Args> bool push(Fn&& fn, Args&&... args)
    {
        // 退出流程中禁止添加新任务
        if (m_exit_event != exit_event_t::NORMAL)
            return false;
        else
        {
            ::std::lock_guard<::std::mutex> lck(m_task_lock); // 任务队列读写锁
            m_tasks.push(::std::bind(function_wapper(), // 绑定函数 -> 生成任务（仿函数）
                decay_type(::std::forward<Fn>(fn)), decay_type(::std::forward<Args>(args))...));
            m_task_all++;
        }
        notify<1>();
        return true;
    }
    // 添加多个任务 VS2013+
    template<class Fn, class... Args> bool push_multi(size_t count, Fn&& fn, Args&&... args)
    {
        assert(count >= 0 && count < USHRT_MAX);
        // 退出流程中禁止添加新任务
        if (m_exit_event != exit_event_t::NORMAL)
            return false;
        else if (count)
        {
            size_t _count = count;
            if (_count)
            {
                ::std::lock_guard<::std::mutex> lck(m_task_lock); // 任务队列读写锁
                ::std::function<void()> bind_function = ::std::bind(function_wapper(), // 绑定函数 -> 生成任务（仿函数）
                    decay_type(::std::forward<Fn>(fn)), decay_type(::std::forward<Args>(args))...);
                while (--_count)
                    m_tasks.push(bind_function);
                m_tasks.push(::std::move(bind_function));
                m_task_all += count;
            }
            notify(count);
        }
        return true;
    }

    // 分离所有任务，分离的线程池对象线程数不变
    bool detach()
    {
        return detach(m_thread_started);
    }
    // 分离所有任务，设置分离的线程池对象线程数为thread_number_new
    bool detach(int thread_number_new)
    {
        ::std::lock_guard<::std::mutex> lck(m_task_lock); // 当前线程池任务队列读写锁
        if (!m_tasks.size())
            return false;
        assert(thread_number_new); // 如果线程数为0，则分离的线程池中未处理的任务将丢弃
        auto pThreadPool = new threadpool(thread_number_new);
        ::std::lock_guard<::std::mutex> lck_new(pThreadPool->m_task_lock); // 新线程池任务队列读写锁
        ::std::swap(m_tasks, pThreadPool->m_tasks); // 交换任务队列
        ::std::async([](decltype(pThreadPool) pClass){delete pClass; return success_code + 0xff; }, pThreadPool);
        return true;
    }

    // 获取线程数量
    size_t get_thread_number()
    {
        return m_thread_started.load();
    }
    // 获取任务队列数量
    size_t get_tasks_number()
    {
        ::std::lock_guard<::std::mutex> lck(m_task_lock); // 任务队列读写锁
        return m_tasks.size();
    }
    // 获取已完成任务数
    size_t get_tasks_completed_number()
    {
        return m_task_completed.load();
    }

    // 增加新的处理线程，如果线程池进入中止流程则无动作
    bool set_new_thread_number(int thread_num_set)
    {
        static_assert(thread_num_set >= 0 && thread_num_set < 255, "Thread number must greater than or equal 0 and less than 255");
        // 线程退出
        if (thread_num_set && m_exit_event == exit_event_t::NORMAL)
        {
            // 线程创建、销毁事件锁
            ::std::lock_guard<::std::mutex> lck(m_thread_lock);
            for (register size_t i = 0; i < thread_num_set; i++)
            {
                HANDLE handle = (HANDLE)_beginthreadex(NULL, 0, (uint32_t(WINAPI*)(void*))ThreadProc, this, NORMAL_PRIORITY_CLASS, &threadID);
                m_thread_object.push_back(handle);
                m_thread_started++;
            }
            return true;
        }
        else
            return false;
    }
};


#endif // #ifndef __THREADPOOL_H__
