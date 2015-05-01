/**********************************************************
* �̳߳ؿ�����
* ֧��ƽ̨��Windows
* ���뻷����VS2013+
* ����ʱ�䣺2015-04-05 ����������
* ����޸ģ�2015-05-01 ����������
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


// �̳߳���; handle_exception: �Ƿ����������쳣
template<bool handle_exception = true> class threadpool
{
private:
    // �߳���
    ::std::atomic<int> m_thread_number{ 0 };
    ::std::atomic<int> m_thread_started{ 0 };
    // �̶߳���
    ::std::list<::std::pair<::std::thread, SAFE_HANDLE_OBJECT>> m_thread_object;
    // �����ٷ�����̶߳���
    ::std::vector<::std::pair<::std::thread, SAFE_HANDLE_OBJECT>> m_thread_destroy;
    // �������
    ::std::deque<::std::function<void()>> m_tasks;
    decltype(m_tasks) m_pause_tasks;
    decltype(m_tasks)* m_push_tasks{ &m_tasks };
    decltype(m_tasks) m_exception_tasks;
    ::std::atomic<size_t> m_task_completed{ 0 };
    ::std::atomic<size_t> m_task_all{ 0 };
    // ������ж�д��
    ::std::mutex m_task_lock;
    // �̴߳����������¼���
    ::std::mutex m_thread_lock;
    // ֪ͨ�¼�
    SAFE_HANDLE_OBJECT m_stop_thread; // �˳��¼����ر������߳�
    SAFE_HANDLE_OBJECT m_notify_task; // ֪ͨ�߳���������

    enum class exit_event_t {
        INITIALIZATION,
        NORMAL,
        PAUSE,
        STOP_IMMEDIATELY,
        WAIT_TASK_COMPLETE,
    };
    // �˳������¼�
    ::std::atomic<exit_event_t> m_exit_event{ exit_event_t::INITIALIZATION };

    // �߳���ں���
    size_t thread_entry(HANDLE exit_event)
    {
        debug_output("Thread Start: [", this_type().name(), "](0x", this, ")");
        size_t result = pre_run(exit_event);
        debug_output("Thread Result: [", (void*)result, "] [", this_type().name(), "](0x", this, ")");
#if _MSC_VER <= 1800 // Fix std::thread deadlock bug on VS2012,VS2013 (when call join on exit)
        ::ExitThread((DWORD)result);
#endif // #if _MSC_VER <= 1800
        return result;
    }
    // �߳�����ǰ׼��
    size_t pre_run(HANDLE exit_event)
    {
        if (handle_exception)
        {
            while (true)
            {
                try
                {
                    return run(exit_event);
                }
                catch (::std::function<void()>& function_object)
                {
                    debug_output<true>(__FILE__, '(', __LINE__, "): ", function_object.target_type().name());
                    m_exception_tasks.push_back(::std::move(function_object));
                }
            }
        }
        else
        {
            return run(exit_event);
        }
    }
    /* �߳�������Ⱥ���
    * ����ֵ >= success_code ��ʾ�����˳���< success_code Ϊ�������˳�
    * run�����屾���ջ��û�ж����ƶ�ebp/rbp�Ĵ�����ȫ�����Բ������쳣
    **/
    size_t run(HANDLE exit_event)
    {
        // �߳�֪ͨ�¼�
        HANDLE handle_notify[] = { exit_event, m_stop_thread, m_notify_task };
        while (true)
        {
            // �����߳�֪ͨ�¼�
            switch (WaitForMultipleObjects(sizeof(handle_notify) / sizeof(HANDLE), handle_notify, FALSE, INFINITE))
            {
            case WAIT_OBJECT_0:     // �˳���ǰ�߳�
                return success_code + 0;
            case WAIT_OBJECT_0 + 1: // �����˳��¼�
                switch (m_exit_event.load())
                {
                case exit_event_t::INITIALIZATION:      // �̳߳ض���δ׼����
                    return success_code - 0xff;
                case exit_event_t::NORMAL:              // δ�����˳��¼�
                    return success_code + 1;
                case exit_event_t::PAUSE:               // �߳���ͣ��
                    return success_code + 2;
                case exit_event_t::STOP_IMMEDIATELY:    // �����˳�
                    return success_code + 3;
                case exit_event_t::WAIT_TASK_COMPLETE:  // �ȴ�����������
                default:
                    break;
                }
            case WAIT_OBJECT_0 + 2: // ��ǰ�̼߳���
                while (true)
                {
                    if (!run_task(get_task())) // ���������û������
                    {
                        if (m_exit_event == exit_event_t::WAIT_TASK_COMPLETE)
                            return success_code + 4;
                        break;
                    }
                }
                break;
            case WAIT_FAILED:       // ����
                return success_code - 3;
            default:
                return success_code - 4;
            }
        }
    }

    // ���������֪ͨ
    void notify()
    { // ֪ͨһ���߳�
        ::SetEvent(m_notify_task);
    }
    void notify(size_t attach_tasks_number)
    { // ֪ͨһ���߳�
        int i = auto_min(3, m_thread_started.load());
        while (attach_tasks_number-- && i--)
            notify();
    }

    // ��ȡ��������е����񣬷��ص�ǰ�����δִ����������
    ::std::pair<::std::function<void()>, size_t> get_task()
    {
        ::std::function<void()> task;
        ::std::unique_lock<::std::mutex> lck(m_task_lock); // ������ж�д��
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
    // ����һ�����񣬷�������������Ƿ�������[true:������; false:û����]
    bool run_task(::std::pair<::std::function<void()>, size_t>&& task_val)
    {
        // ���������������δ���������߳�����֪ͨ
        if (task_val.second > 1)
            notify();
        if (task_val.second)
        {
            if (handle_exception)
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
            }
            else
            {
                task_val.first();
            }
            m_task_completed++;
        }
        return task_val.second > 1;
    }

public:
    // ��׼�߳̽�������
    static const size_t success_code = 0x00001000;

    threadpool(){}
    threadpool(int thread_number)
    {
        set_thread_number(thread_number);
    }
    ~threadpool()
    {
        stop_on_completed(); // �˳�ʱ�ȴ��������
        for (auto& handle_obj : m_thread_object)
        {
#if _MSC_VER <= 1800 // Fix std::thread deadlock bug on VS2012,VS2013 (when call join on exit)
            ::WaitForSingleObject((HANDLE)handle_obj.first.native_handle(), INFINITE);
            handle_obj.first.detach();
#else // Other platform
            handle_obj.first.join(); // �ȴ����д򿪵��߳��˳�
#endif // #if _MSC_VER <= 1800
        }
        for (auto& handle_obj : m_thread_destroy)
        {
#if _MSC_VER <= 1800 // Fix std::thread deadlock bug on VS2012,VS2013 (when call join on exit)
            ::WaitForSingleObject((HANDLE)handle_obj.first.native_handle(), INFINITE);
            handle_obj.first.detach();
#else // Other platform
            handle_obj.first.join(); // �ȴ����������ٷ�����߳��˳�
#endif // #if _MSC_VER <= 1800
        }
    }
    // ���ƹ��캯��
    threadpool(const threadpool&) = delete;
    // ���Ƹ�ֵ���
    threadpool& operator=(const threadpool&) = delete;
    // �ƶ����캯��
    threadpool(threadpool&&) = delete;
    // �ƶ���ֵ���
    threadpool& operator=(threadpool&&) = delete;

    // ������ͣpause���߳�
    bool start()
    {
        // ������ж�д��
        ::std::unique_lock<::std::mutex> lck(m_task_lock);
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
            // �˳������н�ֹ�����߳̿����¼�
        case exit_event_t::INITIALIZATION:
        default:
            return false;
        }
    }
    // ��ͣ�߳�ִ�У�ֱ���˳�stop��������start
    bool pause()
    {
        // ������ж�д��
        ::std::unique_lock<::std::mutex> lck(m_task_lock);
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
        default: // �˳������н�ֹ�����߳̿����¼�
            return false;
        }
    }
    // ������������δ��������񽫶���
    void stop()
    {
        switch (m_exit_event.load())
        {
        case exit_event_t::NORMAL:
        case exit_event_t::WAIT_TASK_COMPLETE:
            // ������ж�д��
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
    // ���������ִ����Ϻ��˳��̣߳�����̳߳������˳���ʧ��
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

    // ���һ������
    template<class Fn, class... Args> bool push(Fn&& fn, Args&&... args)
    {
        typedef decltype(decay_type(::std::forward<Fn>(fn))(decay_type(::std::forward<Args>(args))...)) result_type;
        switch (m_exit_event.load())
        {
        case exit_event_t::NORMAL:
        case exit_event_t::PAUSE:
        case exit_event_t::INITIALIZATION: // δ��ʼ�����̳߳���Ȼ�����������
            break;
        default: // �˳������н�ֹ�����߳̿����¼�
            return false;
        }
        // �󶨺���
        auto task_obj = ::std::make_shared<decltype(::std::bind(::std::forward<Fn>(fn), ::std::forward<Args>(args)...))>(
            ::std::bind(::std::forward<Fn>(fn), ::std::forward<Args>(args)...));
        // �������񣨷º�����
        ::std::function<void()> bind_function(::std::bind(function_wapper(), ::std::move(task_obj)));
        // ������ж�д��
        ::std::unique_lock<::std::mutex> lck(m_task_lock);
        m_push_tasks->push_back(::std::move(bind_function));
        lck.unlock();
        m_task_all++;
        notify();
        return true;
    }
    // ���һ�����񲢷��ط���ֵ����pair<future,bool>��ʹ��future::get��ȡ����ֵ����δ��ɻ�ȴ���ɣ�
    template<class Fn, class... Args> auto push_future(Fn&& fn, Args&&... args)
        -> ::std::pair<::std::future<decltype(decay_type(::std::forward<Fn>(fn))(decay_type(::std::forward<Args>(args))...))>, bool>
    {
        typedef decltype(decay_type(::std::forward<Fn>(fn))(decay_type(::std::forward<Args>(args))...)) result_type;
        ::std::future<result_type> future_obj;
        switch (m_exit_event.load())
        {
        case exit_event_t::NORMAL:
        case exit_event_t::PAUSE:
        case exit_event_t::INITIALIZATION: // δ��ʼ�����̳߳���Ȼ�����������
            break;
        default: // �˳������н�ֹ�����߳̿����¼�
            return ::std::make_pair(::std::move(future_obj), false);
        }
        // �󶨺���
        auto task_obj = ::std::make_shared<::std::packaged_task<result_type()>>(
            ::std::bind(::std::forward<Fn>(fn), ::std::forward<Args>(args)...));
        future_obj = task_obj->get_future();
        // �������񣨷º�����
        ::std::function<void()> bind_function(::std::bind(function_wapper(), ::std::move(task_obj)));
        // ������ж�д��
        ::std::unique_lock<::std::mutex> lck(m_task_lock);
        m_push_tasks->push_back(::std::move(bind_function));
        lck.unlock();
        m_task_all++;
        notify();
        return ::std::make_pair(::std::move(future_obj), true);
    }
    // ��Ӷ������
    template<class Fn, class... Args> bool push_multi(size_t count, Fn&& fn, Args&&... args)
    {
        typedef decltype(decay_type(::std::forward<Fn>(fn))(decay_type(::std::forward<Args>(args))...)) result_type;
        assert(count >= 0 && count < USHRT_MAX);
        switch (m_exit_event.load())
        {
        case exit_event_t::NORMAL:
        case exit_event_t::PAUSE:
        case exit_event_t::INITIALIZATION: // δ��ʼ�����̳߳���Ȼ�����������
            break;
        default: // �˳������н�ֹ�����߳̿����¼�
            return false;
        }
        if (count)
        {
            // �󶨺���
            auto task_obj = ::std::make_shared<decltype(::std::bind(::std::forward<Fn>(fn), ::std::forward<Args>(args)...))>(
                ::std::bind(::std::forward<Fn>(fn), ::std::forward<Args>(args)...));
            // �������񣨷º�����
            ::std::function<void()> bind_function(::std::bind(function_wapper(), ::std::move(task_obj)));
            // ������ж�д��
            ::std::unique_lock<::std::mutex> lck(m_task_lock);
            m_push_tasks->insert(m_push_tasks->end(), count, ::std::move(bind_function));
            lck.unlock();
            m_task_all += count;
            notify(count);
        }
        return true;
    }
    // ��Ӷ�����񲢷��ط���ֵ����pair<vector<future>,bool>��ʹ��future::get��ȡ����ֵ����δ��ɻ�ȴ���ɣ�
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
        case exit_event_t::INITIALIZATION: // δ��ʼ�����̳߳���Ȼ�����������
            break;
        default: // �˳������н�ֹ�����߳̿����¼�
            return ::std::make_pair(::std::move(future_obj), false);
        }
        if (count)
        {
            future_obj.reserve(count);
            while (count--)
            {
                // �󶨺���
                auto task_obj = ::std::make_shared<::std::packaged_task<result_type()>>(
                    ::std::bind(::std::forward<Fn>(fn), ::std::forward<Args>(args)...));
                future_obj.push_back(task_obj->get_future());
                // �������񣨷º�����
                ::std::function<void()> bind_function(::std::bind(function_wapper(), ::std::move(task_obj)));
                // ������ж�д��
                ::std::lock_guard<::std::mutex> lck(m_task_lock);
                m_push_tasks->push_back(::std::move(bind_function));
            }
            m_task_all += count;
            notify(count);
        }
        return ::std::make_pair(::std::move(future_obj), true);
    }

    // �����������񣬷�����̳߳ض����߳�������
    bool detach()
    {
        return detach(m_thread_started);
    }
    // ���������������÷�����̳߳ض����߳���Ϊthread_number_new
    bool detach(int thread_number_new)
    {
        ::std::lock_guard<::std::mutex> lck(m_task_lock); // ��ǰ�̳߳�������ж�д��
        if (!m_push_tasks->size())
            return false;
        assert(thread_number_new); // ����߳���Ϊ0���������̳߳���δ��������񽫶���
        auto detach_threadpool = new threadpool(thread_number_new);
        ::std::lock_guard<::std::mutex> lck_new(detach_threadpool->m_task_lock); // ���̳߳�������ж�д��
        ::std::swap(*m_push_tasks, detach_threadpool->m_tasks); // �����������
        // ֪ͨ������̶߳�������
        detach_threadpool->notify(detach_threadpool->m_tasks.size());
        ::std::async([](decltype(detach_threadpool) pClass){
            delete pClass;
            size_t result = success_code + 0xff;
            debug_output("Thread Result: ", result, "(0x", (void*)result, ')');
            return result;
        }, detach_threadpool);
        return true;
    }

    // ��ȡ�߳�����
    int get_thread_number()
    {
        return m_thread_started.load();
    }
    // ��ȡ�����߳���
    int get_free_thread_number()
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
    // ��ȡ�����������
    size_t get_tasks_number()
    {
        ::std::lock_guard<::std::mutex> lck(m_task_lock); // ������ж�д��
        return m_push_tasks->size();
    }
    // ��ȡ�����������
    size_t get_tasks_completed_number()
    {
        return m_task_completed.load();
    }
    // ��ȡ�������������
    size_t get_tasks_total_number()
    {
        return m_task_all.load();
    }

    // ��ȡ�쳣�������
    decltype(m_tasks) get_exception_tasks()
    {
        decltype(m_tasks) exception_tasks = ::std::move(m_exception_tasks);
        return ::std::move(exception_tasks);
    }
    // ��ȡ��ʼ���߳���
    int get_default_thread_number()
    {
        return m_thread_number.load();
    }
    // ��ȡ������Ϣ
    static const type_info& this_type()
    {
        return typeid(threadpool);
    }

    // ��ʼ���̳߳أ����ô����߳������ѳ�ʼ����ʧ��
    bool set_thread_number(int thread_number)
    {
        assert(thread_number >= 0 && thread_number < 255); // "Thread number must greater than or equal 0 and less than 255"
        switch (m_exit_event.load())
        {
        case exit_event_t::INITIALIZATION: // δ��ʼ�����̳߳�
            m_exit_event = exit_event_t::NORMAL;
            m_stop_thread = ::CreateEventW(nullptr, TRUE, FALSE, nullptr);  // �ֶ���λ�����ź�
            m_notify_task = ::CreateEventW(nullptr, FALSE, FALSE, nullptr); // �Զ���λ�����ź�
            break;
        default: // �ѳ�ʼ�����̳߳ؽ�ʧ��
            return false;
        }
        if (thread_number < 0)
            return false;
        m_thread_number = thread_number;
        return set_new_thread_number(thread_number);
    }
    // �����µĴ����߳������˳����̺�δ��ʼ�����̳߳���ʧ��
    bool set_new_thread_number(int thread_number_new)
    {
        assert(thread_number_new >= 0 && thread_number_new < 255); // "Thread number must greater than or equal 0 and less than 255"
        switch (m_exit_event.load())
        {
        case exit_event_t::INITIALIZATION: // δ��ʼ�����̳߳ؽ�ʧ��
            return false;
        case exit_event_t::NORMAL:
        case exit_event_t::PAUSE:
            break;
        default: // �˳������н�ֹ�����߳̿����¼�
            return false;
        }
        if (thread_number_new < 0) // �߳���С��0��ʧ��
            return false;
        if (m_thread_started.load() != thread_number_new)
        {
            // �̴߳����������¼���
            ::std::unique_lock<::std::mutex> lck(m_thread_lock);
            for (register int i = m_thread_started.load(); i < thread_number_new; i++)
            {
                HANDLE thread_exit_event = ::CreateEventW(nullptr, FALSE, FALSE, nullptr); // �Զ���λ�����ź�
                auto iter = m_thread_object.insert(m_thread_object.end(), ::std::make_pair(
                    ::std::thread(&threadpool::thread_entry, this, thread_exit_event), SAFE_HANDLE_OBJECT(thread_exit_event)));
                m_thread_started++;
            }
            for (register int i = m_thread_started.load(); i > thread_number_new; i--)
            {
                auto iter = m_thread_object.begin();
                ::SetEvent(iter->second);
                m_thread_destroy.push_back(::std::move(*iter));
                m_thread_object.erase(iter);
                m_thread_started--;
            }
            lck.unlock();
            assert(m_thread_started.load() == thread_number_new);
        }
        return true;
    }
    // �����̳߳�����Ϊ��ʼ����
    bool reset_thread_number()
    {
        return set_new_thread_number(get_default_thread_number());
    }
};
