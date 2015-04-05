/**********************************************************
 * 线程池控制类
 * 支持平台：Windows
 * 编译环境：VS2013+
 * 创建时间：2015-04-05 （宋万鹏）
 * 最后修改：2015-04-05 （宋万鹏）
 **********************************************************/

#ifndef __THREADPOOL_H__
#define __THREADPOOL_H__
#pragma once

#include "common.h"
#include "safe_object.h"
#include <vector>
#include <deque>
#include <mutex>
#include <cassert>
#include <functional>
#include <process.h>


// 初始化线程数量
template<size_t ThreadNum = 2> class CThreadPool
{
private:
    // 线程数
    size_t m_threadNum;
    // 线程句柄数组
    ::std::vector<SAFE_HANDLE_OBJECT> m_hThread;
    // 任务队列 (VS2010+)
    ::std::deque<::std::function<void()>> m_mission;
    // 任务队列读写锁 (VS2012+)
    ::std::mutex m_mutex;
    // 通知事件
    SAFE_HANDLE_OBJECT m_stop;       // 退出事件，关闭所有线程
    SAFE_HANDLE_OBJECT m_continue;   // 暂停事件，控制线程挂起
    SAFE_HANDLE_OBJECT m_notify_one; // 通知一条线程有新任务
    SAFE_HANDLE_OBJECT m_notify_all; // 通知所有线程有新任务
    // 退出任务类型 (VS2013+)
    enum class exit_event_t {
        NORMAL,
        STOP_IMMEDIATELY,
        WAIT_MISSION_COMPLETE
    } m_exit_event;

    // 静态线程入口函数
    static size_t WINAPI ThreadProc(CThreadPool* pThis)
    {
        return pThis->Run();
    }
    /* 线程任务调度函数
     * 返回值 >= 0 表示正常退出，< 0 为非正常退出
     */
    size_t Run()
    {
        // 线程控制事件
        HANDLE handle_control[] = { m_stop, m_continue };
        // 线程通知事件
        HANDLE handle_notify[] = { m_stop, m_notify_all, m_notify_one };
        // 任务内容
        decltype(getMission()) mission_val;
        while (true)
        {
            // 监听线程控制事件
            switch (WaitForMultipleObjects(sizeof(handle_control) / sizeof(HANDLE), handle_control, FALSE, INFINITE))
            {
            case WAIT_OBJECT_0:     // 正常退出事件
                switch (m_exit_event)
                {
                case exit_event_t::NORMAL:                // 未设置退出事件
                case exit_event_t::STOP_IMMEDIATELY:      // 立即退出
                    return 0;
                case exit_event_t::WAIT_MISSION_COMPLETE: // 等待任务队列清空
                default:
                    break;
                }
            case WAIT_OBJECT_0 + 1: // 继续运行
                break;
            case WAIT_FAILED:       // 错误
            default:
                return -1;
            }
            // 监听线程通知事件
            switch (WaitForMultipleObjects(sizeof(handle_notify) / sizeof(HANDLE), handle_notify, FALSE, INFINITE))
            {
            case WAIT_OBJECT_0:     // 正常退出事件
                switch (m_exit_event)
                {
                case exit_event_t::NORMAL:                // 未设置退出事件
                case exit_event_t::STOP_IMMEDIATELY:      // 立即退出
                    return 1;
                case exit_event_t::WAIT_MISSION_COMPLETE: // 等待任务队列清空
                default:
                    break;
                }
            case WAIT_OBJECT_0 + 1: // 当前激活了所有线程
            case WAIT_OBJECT_0 + 2: // 当前激活了单个线程
                mission_val = getMission();
                // 任务队列中仍然有任务未处理，发送线程启动通知
                if (mission_val.second > 1)
                {
                    notify<1>();
                    mission_val.first();
                } // 这是当前任务队列中最后一项任务
                else if (mission_val.second)
                {
                    mission_val.first();
                    if (m_exit_event == exit_event_t::WAIT_MISSION_COMPLETE)
                        return 2;
                } // 任务队列中没有任务
                else if (m_exit_event == exit_event_t::WAIT_MISSION_COMPLETE)
                    return 3;
                break;
            case WAIT_FAILED:       // 错误
            default:
                return -2;
            }
        }
    }

    // 新任务添加通知
    void notify()
    { // 通知所有线程
        PulseEvent(m_notify_all);
        SetEvent(m_notify_one);
    }
    void notify(size_t attachCount)
    { // 如果添加任务数超过线程数的一半，则通知所有线程，否则只通知一条线程
        if (attachCount)
        {
            if (attachCount > max(m_threadNum / 2, 1))
                PulseEvent(m_notify_all);
            SetEvent(m_notify_one);
        }
    }
    template<size_t attachCount> void notify()
    { // 和上一个通知函数一致，通过模板以优化确定添加数量时的性能
        if (attachCount)
        {
            if (attachCount > max(m_threadNum / 2, 1))
                PulseEvent(m_notify_all);
            SetEvent(m_notify_one);
        }
    }

    // 获取任务队列中的任务，返回当前任务和未执行任务总数
    ::std::pair<::std::function<void()>, size_t> getMission()
    {
        ::std::function<void()> mission;
        // 任务队列读写锁
        ::std::lock_guard<::std::mutex> lck(m_mutex);
        size_t mission_num = m_mission.size();
        if (mission_num)
        {
            ::std::swap(mission, m_mission.front());
            m_mission.pop_front();
            return ::std::make_pair(::std::move(mission), mission_num);
        }
        else
            return ::std::make_pair(::std::move(mission), 0);
    }

public:
    CThreadPool()
        : m_threadNum(ThreadNum), m_hThread(ThreadNum), m_exit_event(exit_event_t::NORMAL)
    {
        static_assert(ThreadNum < 255, "Thread must less than 255");
        m_stop = CreateEvent(NULL, TRUE, FALSE, nullptr);        // 手动复位，无信号
        m_continue = CreateEvent(NULL, TRUE, TRUE, nullptr);     // 手动复位，有信号
        m_notify_one = CreateEvent(NULL, FALSE, FALSE, nullptr); // 自动复位，无信号
        m_notify_all = CreateEvent(NULL, TRUE, FALSE, nullptr);  // 手动复位，无信号

        uint32_t threadID; // 线程ID值，这里不记录
        for (auto& handle_obj : m_hThread) // VS2013+
        {
            HANDLE handle = (HANDLE)_beginthreadex(NULL, 0, (uint32_t(WINAPI*)(void*))ThreadProc, this, NORMAL_PRIORITY_CLASS, &threadID);
            handle_obj = handle;
        }
    }
    ~CThreadPool()
    {
        // 退出时等待任务清空
        if (m_exit_event == exit_event_t::NORMAL)
            StopOnComplete();
        // 已打开的线程数组
        ::std::vector<HANDLE> hThreadOpen;
        DWORD threadOpenNum = 0;
        hThreadOpen.reserve(m_threadNum);
        for (auto& handle_obj : m_hThread) // VS2013+
        {
            HANDLE handle = handle_obj;
            if(handle && handle != INVALID_HANDLE_VALUE)
            {
                hThreadOpen.push_back(handle);
                threadOpenNum++;
            }
        }
        // 等待所有打开的线程退出
        if (threadOpenNum)
            WaitForMultipleObjects(threadOpenNum, hThreadOpen.data(), TRUE, INFINITE);
    }
    CThreadPool(const CThreadPool&) = delete; // 复制构造函数
    CThreadPool& operator=(const CThreadPool&) = delete; // 复制赋值语句
    template<size_t _ThreadNum> CThreadPool(CThreadPool<_ThreadNum>&& _Other) = delete; // 移动构造函数
    template<size_t _ThreadNum> CThreadPool& operator=(CThreadPool<_ThreadNum>&& _Other) = delete; // 移动赋值语句

    // 添加一个任务 VS2013+
    template<class Fn, class... Args> bool Attach(Fn&& fn, Args&&... args)
    {
        // 退出流程中禁止添加新任务
        if (m_exit_event != exit_event_t::NORMAL)
            return false;
        // 任务队列读写锁
        ::std::lock_guard<::std::mutex> lck(m_mutex);
        // 绑定函数 -> 生成任务（仿函数）
        m_mission.push_back(
            ::std::bind<void>(decay_type(::std::forward<Fn>(fn)), decay_type(::std::forward<Args>(args))...)
            );
        notify<1>();
        return true;
    }
    // 添加多个任务 VS2013+
    template<class Fn, class... Args> bool AttachMulti(size_t Count, Fn&& fn, Args&&... args)
    {
        assert(Count >= 0 && Count < USHRT_MAX);
        // 退出流程中禁止添加新任务
        if (m_exit_event != exit_event_t::NORMAL)
            return false;
        if (Count)
        {
            // 任务队列读写锁
            ::std::lock_guard<::std::mutex> lck(m_mutex);
            m_mission.insert(m_mission.end(), Count,
                ::std::bind<void>(decay_type(::std::forward<Fn>(fn)), decay_type(::std::forward<Args>(args))...)
                );
            notify(Count);
        }
        return true;
    }

    // 启动暂停Pause()的线程
    bool Start()
    {
        // 退出流程中禁止操作线程控制事件
        if (m_exit_event != exit_event_t::NORMAL)
            return false;
        SetEvent(m_continue);
        return true;
    }
    // 暂停线程执行，直到退出Stop()或者启动Start()
    bool Pause()
    {
        // 退出流程中禁止操作线程控制事件
        if (m_exit_event != exit_event_t::NORMAL)
            return false;
        ResetEvent(m_continue);
        notify();
        return true;
    }
    // 立即结束任务，未处理的任务将丢弃
    void Stop()
    {
        m_exit_event = exit_event_t::STOP_IMMEDIATELY;
        SetEvent(m_stop);
    }
    // 当任务队列执行完毕后退出线程，如果线程池正在退出则失败
    bool StopOnComplete()
    {
        if (m_exit_event == exit_event_t::WAIT_MISSION_COMPLETE)
            return true;
        else if (m_exit_event != exit_event_t::NORMAL)
            return false;
        m_exit_event = exit_event_t::WAIT_MISSION_COMPLETE;
        SetEvent(m_continue); // 如果线程已暂停，则启动
        SetEvent(m_stop);
        return true;
    }

    // 获取线程数量
    size_t GetThreadNum()
    {
        return m_threadNum;
    }
    // 增加新的处理线程，如果线程池进入中止流程则无动作
    bool AddThreadNum(size_t thread_num_add)
    {
        // 线程退出
        if (thread_num_add && m_exit_event == exit_event_t::NORMAL)
        {
            uint32_t threadID;
            for (register size_t i = 0; i < thread_num_add; i++)
            {
                HANDLE handle = (HANDLE)_beginthreadex(NULL, 0, (uint32_t(WINAPI*)(void*))ThreadProc, this, NORMAL_PRIORITY_CLASS, &threadID);
                m_hThread.push_back(handle);
            }
            m_threadNum += thread_num_add;
            return true;
        }
        else
            return false;
    }
};


#endif // #ifndef __THREADPOOL_H__
