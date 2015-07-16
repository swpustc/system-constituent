/**********************************************************
* 线程池控制类
* 支持平台：Windows
* 编译环境：VS2013+
***********************************************************/

template<> threadpool<HANDLE_EXCEPTION>::~threadpool()
{
    stop_on_completed(); // 退出时等待任务清空
    for (auto& handle_obj : m_thread_object)
    {
#if _MSC_VER <= 1800 // Fix std::thread deadlock bug on VS2012,VS2013 (when call join on exit)
        WaitForSingleObject((HANDLE)get<0>(handle_obj).native_handle(), INFINITE);
        get<0>(handle_obj).detach();
#else // Other platform
        get<0>(handle_obj).join(); // 等待所有打开的线程退出
#endif // #if _MSC_VER <= 1800
    }
    for (auto& handle_obj : m_thread_destroy)
    {
#if _MSC_VER <= 1800 // Fix std::thread deadlock bug on VS2012,VS2013 (when call join on exit)
        WaitForSingleObject((HANDLE)get<0>(handle_obj).native_handle(), INFINITE);
        get<0>(handle_obj).detach();
#else // Other platform
        get<0>(handle_obj).join(); // 等待所有已销毁分离的线程退出
#endif // #if _MSC_VER <= 1800
    }
}

// 线程入口函数
template<> size_t threadpool<HANDLE_EXCEPTION>::thread_entry(threadpool* object, HANDLE pause_event, HANDLE resume_event)
{
    debug_output(_T("Thread Start: ["), this_type().name(), _T("](0x"), object, _T(')'));
    size_t result = object->pre_run(pause_event, resume_event);
    debug_output(_T("Thread Result: ["), (void*)result, _T("] ["), this_type().name(), _T("](0x"), object, _T(')'));
#if _MSC_VER <= 1800 // Fix std::thread deadlock bug on VS2012,VS2013 (when call join on exit)
    ExitThread((DWORD)result);
#endif // #if _MSC_VER <= 1800
    return result;
}

// 任务运行主体函数
template<> inline size_t threadpool<HANDLE_EXCEPTION>::run(HANDLE pause_event, HANDLE resume_event)
{
    // 线程通知事件
    HANDLE handle_notify[] = { pause_event, m_stop_thread, m_notify_task };
    HANDLE handle_resume[] = { resume_event, m_stop_thread };
    while (true)
    {
        // 监听线程通知事件
        switch (WaitForMultipleObjects(sizeof(handle_notify) / sizeof(HANDLE), handle_notify, FALSE, INFINITE))
        {
        case WAIT_OBJECT_0:         // 挂起当前线程
            switch (WaitForMultipleObjects(sizeof(handle_resume) / sizeof(HANDLE), handle_resume, FALSE, INFINITE))
            {
            case WAIT_OBJECT_0:     // 恢复当前线程
                break;
            case WAIT_OBJECT_0 + 1: // 正常退出事件
                return success_code + 0;
            case WAIT_FAILED:       // 错误
                return success_code - 1;
            default:                // 其他
                return success_code - 2;
            }
            break;
        case WAIT_OBJECT_0 + 1:     // 正常退出事件
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
        case WAIT_OBJECT_0 + 2:     // 当前线程激活
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
        case WAIT_FAILED:           // 错误
            return success_code - 3;
        default:
            return success_code - 4;
        }
    }
}


// 分离任务
template<> void threadpool<HANDLE_EXCEPTION>::detach(int thread_number_new)
{
    auto detach_threadpool = new threadpool(thread_number_new);
    unique_lock<decltype(m_task_lock)> lck(m_task_lock); // 当前线程池任务队列读写锁
    unique_lock<decltype(m_task_lock)> lck_new(detach_threadpool->m_task_lock); // 新线程池任务队列读写锁
    swap(*m_push_tasks, detach_threadpool->m_tasks); // 交换任务队列
    lck_new.unlock();
    lck.unlock();
    // 通知分离的线程对象运行
    detach_threadpool->notify(detach_threadpool->m_tasks.size());
    async([](decltype(detach_threadpool) pClass){
        delete pClass;
        static const size_t result = success_code + 0xff;
        debug_output(_T("Thread Result: ["), (void*)result, _T("] ["), pClass->this_type().name(), _T("](0x"), pClass, _T(')'));
        return result;
    }, detach_threadpool);
}

// 分离任务，并得到分离任务执行情况的future
template<> future<size_t> threadpool<HANDLE_EXCEPTION>::detach_future(int thread_number_new)
{
    future<size_t> future_obj;
    auto detach_threadpool = new threadpool(thread_number_new);
    unique_lock<decltype(m_task_lock)> lck(m_task_lock); // 当前线程池任务队列读写锁
    unique_lock<decltype(m_task_lock)> lck_new(detach_threadpool->m_task_lock); // 新线程池任务队列读写锁
    swap(*m_push_tasks, detach_threadpool->m_tasks); // 交换任务队列
    lck_new.unlock();
    lck.unlock();
    // 通知分离的线程对象运行
    detach_threadpool->notify(detach_threadpool->m_tasks.size());
    // 绑定函数
    auto task_obj = make_shared<packaged_task<size_t()>>(bind([](decltype(detach_threadpool) pClass){
        delete pClass;
        static const size_t result = success_code + 0xff;
        debug_output(_T("Thread Result: ["), (void*)result, _T("] ["), pClass->this_type().name(), _T("](0x"), pClass, _T(')'));
        return result;
    }, detach_threadpool));
    future_obj = task_obj->get_future();
    // 生成任务（仿函数）
    async(function<void()>(bind(function_wapper(), move(task_obj))));
    return move(future_obj);;
}

// 设置线程数
template<> bool threadpool<HANDLE_EXCEPTION>::set_thread_number(int thread_number)
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
    return set_new_thread_number(thread_number);
}

template<> bool threadpool<HANDLE_EXCEPTION>::set_new_thread_number(int thread_number_new)
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
        unique_lock<decltype(m_thread_lock)> lck(m_thread_lock);
        for (register int i = m_thread_started.load(); i < thread_number_new; i++)
        {
            if (m_thread_destroy.size())
            {
                auto iter = m_thread_destroy.begin();
                ResetEvent(get<1>(*iter));  // 取消线程暂停事件
                SetEvent(get<2>(*iter));    // 如果线程已暂停则恢复
                m_thread_object.push_back(move(*iter));
                m_thread_destroy.erase(iter);
            }
            else
            {
                already_create_new_thread = true;
                HANDLE thread_exit_event = CreateEventW(nullptr, FALSE, FALSE, nullptr); // 自动复位，无信号
                HANDLE thread_resume_event = CreateEventW(nullptr, FALSE, FALSE, nullptr); // 自动复位，无信号
                m_thread_object.push_back(make_tuple(
                    thread(thread_entry, this, thread_exit_event, thread_resume_event),
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
            ResetEvent(get<2>(*iter));  // 取消线程恢复事件
            SetEvent(get<1>(*iter));    // 如果线程在运行则暂停
            m_thread_destroy.push_back(move(*iter));
            m_thread_object.erase(iter);
            m_thread_started--;
        }
        assert(m_thread_started.load() == thread_number_new);
        lck.unlock();
    }
    return true;
}

// 设置线程优先级
template<> void threadpool<HANDLE_EXCEPTION>::set_thread_priority(thread_priority priority/*=thread_priority::none*/)
{
    // 线程创建、销毁事件锁
    unique_lock<decltype(m_thread_lock)> lck(m_thread_lock);
#ifdef _WIN32
    int _priority;
    switch (m_priority = priority)
    {
    case thread_priority::time_critical:
        _priority = THREAD_PRIORITY_TIME_CRITICAL;
        break;
    case thread_priority::highest:
        _priority = THREAD_PRIORITY_HIGHEST;
        break;
    case thread_priority::above_normal:
        _priority = THREAD_PRIORITY_ABOVE_NORMAL;
        break;
    case thread_priority::normal:
    case thread_priority::none:
        _priority = THREAD_PRIORITY_NORMAL;
        break;
    case thread_priority::below_normal:
        _priority = THREAD_PRIORITY_BELOW_NORMAL;
        break;
    case thread_priority::lowest:
        _priority = THREAD_PRIORITY_LOWEST;
        break;
    case thread_priority::idle:
        _priority = THREAD_PRIORITY_IDLE;
        break;
    default:
        return;
    }
    for (auto& th : m_thread_object)
        SetThreadPriority(get<0>(th).native_handle(), _priority);
    for (auto& th : m_thread_destroy)
        SetThreadPriority(get<0>(th).native_handle(), _priority);
#else  /* UNIX */
    struct sched_param _priority;
    switch (m_priority = priority)
    {
    case thread_priority::time_critical:
        _priority = sched_get_priority_max(SCHED_RR);
        break;
    case thread_priority::highest:
        _priority = sched_get_priority_min(SCHED_RR) + (sched_get_priority_max(SCHED_RR) - sched_get_priority_min(SCHED_RR)) * 5 / 6;
        break;
    case thread_priority::above_normal:
        _priority = sched_get_priority_min(SCHED_RR) + (sched_get_priority_max(SCHED_RR) - sched_get_priority_min(SCHED_RR)) * 2 / 3;
        break;
    case thread_priority::normal:
        _priority = (sched_get_priority_max(SCHED_RR) + sched_get_priority_min(SCHED_RR)) / 2;
        break;
    case thread_priority::below_normal:
        _priority = sched_get_priority_min(SCHED_RR) + (sched_get_priority_max(SCHED_RR) - sched_get_priority_min(SCHED_RR)) / 3;
        break;
    case thread_priority::lowest:
        _priority = sched_get_priority_min(SCHED_RR) + (sched_get_priority_max(SCHED_RR) - sched_get_priority_min(SCHED_RR)) / 6 ;
        break;
    case thread_priority::idle:
        _priority = sched_get_priority_min(SCHED_RR);
        break;
    case thread_priority::none:
        for (auto& th : m_thread_object)
            pthread_attr_setinheritsched(get<0>(th).native_handle(), PTHREAD_INHERIT_SCHED);
        for (auto& th : m_thread_destroy)
            pthread_attr_setinheritsched(get<0>(th).native_handle(), PTHREAD_INHERIT_SCHED);
    default:
        return;
    }
    for (auto& th : m_thread_object)
    {
        pthread_attr_setschedpolicy(get<0>(th).native_handle(), SCHED_RR);
        pthread_attr_setschedparam(get<0>(th).native_handle(), &_priority);
        pthread_attr_setinheritsched(get<0>(th).native_handle(), PTHREAD_EXPLICIT_SCHED);
    }
    for (auto& th : m_thread_destroy)
    {
        pthread_attr_setschedpolicy(get<0>(th).native_handle(), SCHED_RR);
        pthread_attr_setschedparam(get<0>(th).native_handle(), &_priority);
        pthread_attr_setinheritsched(get<0>(th).native_handle(), PTHREAD_EXPLICIT_SCHED);
    }
#endif  /* _WIN32 */
}
