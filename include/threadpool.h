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
#include <thread>
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
        return pThis->run();
    }
    /* 线程任务调度函数
     * 返回值 >= success_code 表示正常退出，< success_code 为非正常退出
     */
    size_t run()
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
                    return success_code + 0;
                case exit_event_t::STOP_IMMEDIATELY:      // 立即退出
                    return success_code + 1;
                case exit_event_t::WAIT_MISSION_COMPLETE: // 等待任务队列清空
                default:
                    break;
                }
            case WAIT_OBJECT_0 + 1: // 继续运行
                break;
            case WAIT_FAILED:       // 错误
                return  success_code - 1;
            default:
                return success_code - 2;
            }
            // 监听线程通知事件
            switch (WaitForMultipleObjects(sizeof(handle_notify) / sizeof(HANDLE), handle_notify, FALSE, INFINITE))
            {
            case WAIT_OBJECT_0:     // 正常退出事件
                switch (m_exit_event)
                {
                case exit_event_t::NORMAL:                // 未设置退出事件
                    return success_code + 2;
                case exit_event_t::STOP_IMMEDIATELY:      // 立即退出
                    return success_code + 3;
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
                        return success_code + 4;
                } // 任务队列中没有任务
                else if (m_exit_event == exit_event_t::WAIT_MISSION_COMPLETE)
                    return success_code + 5;
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
    // 标准线程结束代码
    static const size_t success_code = 0x00001000;

    CThreadPool() :
        CThreadPool(ThreadNum)
    {
    }
    CThreadPool(size_t threadNum) :
        m_threadNum(threadNum), m_hThread(threadNum), m_exit_event(exit_event_t::NORMAL)
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
            if (handle && handle != INVALID_HANDLE_VALUE)
            {
                hThreadOpen.push_back(handle);
                threadOpenNum++;
            }
        }
        // 等待所有打开的线程退出
        if (threadOpenNum)
            WaitForMultipleObjects(threadOpenNum, hThreadOpen.data(), TRUE, INFINITE);
    }
    // 复制构造函数
    CThreadPool(const CThreadPool&) = delete;
    template<size_t _ThreadNum> CThreadPool(const CThreadPool<_ThreadNum>&) = delete;
    // 复制赋值语句
    CThreadPool& operator=(const CThreadPool&) = delete;
    template<size_t _ThreadNum> CThreadPool& operator=(const CThreadPool<_ThreadNum>&) = delete;
    // 移动构造函数
    template<size_t _ThreadNum> CThreadPool(CThreadPool<_ThreadNum>&& _Other) :
        CThreadPool(_Other.m_threadNum)
    {
        *this = ::std::move(_Other);
    }
    // 移动赋值语句
    template<size_t _ThreadNum> CThreadPool& operator=(CThreadPool<_ThreadNum>&& _Other)
    {
        switch (_Other.m_exit_event)
        {
        case exit_event_t::NORMAL:
            // 为了锁的正确释放，需要放在花括号中
            if (true)
            {
                // 任务队列读写锁
                ::std::lock_guard<::std::mutex> lck(m_mutex);
                ::std::lock_guard<::std::mutex> lck_other(_Other.m_mutex);
                // 交换任务队列
                ::std::swap(m_mission, _Other.m_mission);
            }
            break;
        case exit_event_t::STOP_IMMEDIATELY:
            Stop();
            break;
        case exit_event_t::WAIT_MISSION_COMPLETE:
            StopOnComplete();
            break;
        }
        return *this;
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

    // 添加一个任务 VS2013+
    template<class Fn, class... Args> bool Attach(Fn&& fn, Args&&... args)
    {
        // 退出流程中禁止添加新任务
        if (m_exit_event != exit_event_t::NORMAL)
            return false;
        // 任务队列读写锁
        ::std::lock_guard<::std::mutex> lck(m_mutex);
        // 绑定函数 -> 生成任务（仿函数）
        auto original = function_wapper_void(::std::bind(decay_type(::std::forward<Fn>(fn)), decay_type(::std::forward<Args>(args))...));
        m_mission.push_back(::std::bind([](decltype(original)& fn){fn(); }, ::std::move(original)));
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
            auto original = function_wapper_void(::std::bind(decay_type(::std::forward<Fn>(fn)), decay_type(::std::forward<Args>(args))...));
            m_mission.insert(m_mission.end(), Count, ::std::bind([](decltype(original)& fn){fn(); }, ::std::move(original)));
            notify(Count);
        }
        return true;
    }

    // 分离线程池对象，分离的线程池对象线程数不变
    bool Detach()
    {
        // 已经停止的线程池分离操作将失败
        if (m_exit_event != exit_event_t::NORMAL)
            return false;
        assert(m_threadNum); // 如果线程数为0，则分离的线程池中未处理的任务将丢弃
        auto pThreadPool = new CThreadPool(::std::move(*this));
        ::std::thread delete_thread([](decltype(pThreadPool) pClass){delete pClass; }, pThreadPool);
        delete_thread.detach();
        return true;
    }
    // 分离线程池对象，设置分离的线程池对象线程数为ThreadNumNew
    bool Detach(size_t threadNumNew)
    {
        // 已经停止的线程池分离操作将失败
        if (m_exit_event != exit_event_t::NORMAL)
            return false;
        assert(threadNumNew); // 如果线程数为0，则分离的线程池中未处理的任务将丢弃
        auto pThreadPool = new CThreadPool(threadNumNew);
        *pThreadPool = ::std::move(*this);
        ::std::thread delete_thread([](decltype(pThreadPool) pClass){delete pClass; }, pThreadPool);
        delete_thread.detach();
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
