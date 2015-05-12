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

// 分离任务
template<> bool threadpool<HANDLE_EXCEPTION>::detach(int thread_number_new)
{
    lock_guard<mutex> lck(m_task_lock); // 当前线程池任务队列读写锁
    if (!m_push_tasks->size())
        return false;
    auto detach_threadpool = new threadpool(thread_number_new);
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

// 分离任务，并得到分离任务执行情况的future
template<> future<size_t> threadpool<HANDLE_EXCEPTION>::detach_future(int thread_number_new)
{
    future<size_t> future_obj;
    lock_guard<mutex> lck(m_task_lock); // 当前线程池任务队列读写锁
    auto detach_threadpool = new threadpool(thread_number_new);
    lock_guard<mutex> lck_new(detach_threadpool->m_task_lock); // 新线程池任务队列读写锁
    swap(*m_push_tasks, detach_threadpool->m_tasks); // 交换任务队列
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
        // 线程创建、销毁事件锁
        unique_lock<mutex> lck(m_thread_lock);
        for (register int i = m_thread_started.load(); i < thread_number_new; i++)
        {
            HANDLE thread_exit_event = ::CreateEventW(nullptr, FALSE, FALSE, nullptr); // 自动复位，无信号
            auto iter = m_thread_object.insert(m_thread_object.end(), make_pair(
                thread(thread_entry, this, thread_exit_event), SAFE_HANDLE_OBJECT(thread_exit_event)));
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
