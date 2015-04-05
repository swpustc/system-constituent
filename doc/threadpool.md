
# CThreadPool class

线程池的创建、调度和销毁。


## 类申明

```
template<size_t ThreadNum = 2> class CThreadPool
{
private:
    size_t m_threadNum;
    ::std::vector<SAFE_HANDLE_OBJECT> m_hThread;
    ::std::deque<::std::function<size_t()>> m_mission;
    ::std::mutex m_mutex;
    SAFE_HANDLE_OBJECT m_stop;
    SAFE_HANDLE_OBJECT m_continue;
    SAFE_HANDLE_OBJECT m_notify_one;
    SAFE_HANDLE_OBJECT m_notify_all;
    enum class exit_event_t m_exit_event;

    static size_t WINAPI ThreadProc(CThreadPool* pThis);
    size_t Run();

    void notify();
    void notify(size_t attachCount);
    template<size_t attachCount> void notify();
    ::std::pair<::std::function<size_t()>, size_t> getMission();

public:
    CThreadPool();
    ~CThreadPool();
    CThreadPool(const CThreadPool&) = delete;
    CThreadPool& operator=(const CThreadPool&) = delete;
    template<size_t _ThreadNum> CThreadPool(CThreadPool<_ThreadNum>&& _Other) = delete;
    template<size_t _ThreadNum> CThreadPool& operator=(CThreadPool<_ThreadNum>&& _Other) = delete;

    template<class Fn, class... Args> bool Attach(Fn&& fn, Args&&... args);
    template<class Fn, class... Args> bool AttachMulti(size_t Count, Fn&& fn, Args&&... args);
    bool Start();
    bool Pause();
    void Stop();
    bool StopOnComplete();

    size_t GetThreadNum();
    bool AddThreadNum(size_t thread_num_add);
};
```


## 成员函数

#### CThreadPool::CThreadPool()

构造函数。

#### CThreadPool::~CThreadPool()

析构函数，退出时会等待所有任务执行完毕。

#### CThreadPool::CThreadPool(const CThreadPool&) = delete

不允许通过复制构造对象。

#### CThreadPool::CThreadPool& operator=(const CThreadPool&) = delete

不允许复制另一个CThreadPool对象。

#### CThreadPool::CThreadPool(CThreadPool&& _Other) = delete

不允许通过移动构造对象。

#### CThreadPool::CThreadPool& operator=(CThreadPool&& _Other) = delete

不允许移动另一个CThreadPool对象。

#### template<class Fn, class... Args> bool CThreadPool::Attach(Fn&& fn, Args&&... args)

添加单个任务，fn为函数，args为参数列表。函数可以为函数对象、函数指针、C++98/03/11兼容的仿函数对象、lambda表达式。
args通过完美转发转发参数，参数可以是任何类型，数量不限。

传入函数的参数列表的类型和数量必须与传入的参数类型和数量一致，否则会产生编译时错误。
传入函数的返回值类型必须是void。

如果线程池已进入退出流程，返回false，否则返回true。

#### template<class Fn, class... Args> bool CThreadPool::AttachMulti(size_t Count, Fn&& fn, Args&&... args)

添加重复的任务，Count为重复的次数。如果Count为0，亦返回true。

如果线程池已进入退出流程，返回false，否则返回true。

#### bool CThreadPool::Start()

如果线程池已暂停，则启动线程池。

如果线程池已进入退出流程，返回false，否则返回true。

#### bool CThreadPool::Pause()

暂停线程池中的所有线程。

如果线程池已进入退出流程，返回false，否则返回true。

#### void CThreadPool::Stop()

立即停止线程池，未完成的任务将丢弃。

#### bool CThreadPool::StopOnComplete()

停止线程池，不再允许添加新的任务；在所有任务完成后，退出工作线程。
可以调用**CThreadPool::Stop()**以立即停止线程池。

如果已经调用过**CThreadPool::Stop()**，返回false，否则返回true。

#### size_t CThreadPool::GetThreadNum()

获取当前线程中工作线程的数量。

#### bool CThreadPool::AddThreadNum(size_t thread_num_add)

增加线程池中工作线程的数量。

如果线程池已进入退出流程，或者添加的线程数`thread_num_add==0`，返回false，否则返回true。


## 备注

此线程池未使用虚函数，异常安全。默认的线程数为2，实际运用时的线程数应少于CPU核心数。

使用C++11模板类编写，无需链接库文件和单独编译。


## 示例代码

在测试代码中，可以查看**CThreadPool**的[示例代码](../test/threadpool.cpp)。


## 要求

项目       |  要求
:--------- |:---------
支持的平台 | Windows
编译器版本 | VS2013+
头文件     | threadpool.h
库文件     | 无
DLL        | 无


## 参见
