# CThreadPool class

线程池的创建、调度和销毁。


## 公共接口

源文件：[include/threadpool.h](../include/threadpool.h "查看 threadpool.h")

```cpp
template<size_t ThreadNum = 2> class CThreadPool
{
public:
    CThreadPool();
    CThreadPool(size_t threadNum);
    ~CThreadPool();
    template<size_t _ThreadNum> CThreadPool(CThreadPool<_ThreadNum>&& _Other);
    template<size_t _ThreadNum> CThreadPool& operator=(CThreadPool<_ThreadNum>&& _Other);

    bool Start();
    bool Pause();
    void Stop();
    bool StopOnComplete();

    template<class Fn, class... Args> bool Attach(Fn&& fn, Args&&... args);
    template<class Fn, class... Args> bool AttachMulti(size_t Count, Fn&& fn, Args&&... args);

    bool Detach();
    bool Detach(size_t threadNumNew);

    size_t GetThreadNum();
    bool AddThreadNum(size_t thread_num_add);
};
```


## 成员函数

- ##### `CThreadPool::CThreadPool()`
    默认构造函数。

- ##### `CThreadPool::CThreadPool(size_t threadNum)`
    指定线程数的构造函数。

- ##### `CThreadPool::~CThreadPool()`
    默认析构函数，退出时会等待所有任务执行完毕。

- ##### `template<size_t _ThreadNum> CThreadPool::CThreadPool(CThreadPool<_ThreadNum>&& _Other)`
    移动构造函数。

    **注意：**虽然实现了安全的移动构造函数，但是将对象应用在STL容器中，会遇到严重的性能问题。
    应避免大量使用移动构造函数的情况。

- ##### `template<size_t _ThreadNum> CThreadPool& CThreadPool::operator=(CThreadPool<_ThreadNum>&& _Other)`
    移动赋值语句，用于对象的交换和堆、栈对象的分离机制。

- ##### `bool CThreadPool::Start()`
    如果线程池已暂停，则启动线程池。

    如果线程池已进入退出流程，返回false，否则返回true。

- ##### `bool CThreadPool::Pause()`
    暂停线程池中的所有线程。

    如果线程池已进入退出流程，返回false，否则返回true。

- ##### `void CThreadPool::Stop()`
    立即停止线程池，未完成的任务将丢弃。

- ##### `bool CThreadPool::StopOnComplete()`
    停止线程池，不再允许添加新的任务；在所有任务完成后，退出工作线程。
    可以调用**CThreadPool::Stop()**以立即停止线程池。

    如果已经调用过**CThreadPool::Stop()**，返回false，否则返回true。

- ##### `template<class Fn, class... Args> bool CThreadPool::Attach(Fn&& fn, Args&&... args)`
    添加单个任务，fn为函数，args为参数列表。函数可以为函数对象、函数指针、C++98/03/11兼容的仿函数对象、lambda表达式。
    args通过完美转发转发参数，参数可以是任何类型，数量不限。

    传入函数的参数列表的类型和数量必须与传入的参数类型和数量一致，否则会产生编译时错误。

    如果线程池已进入退出流程，返回false，否则返回true。

- ##### `template<class Fn, class... Args> bool CThreadPool::AttachMulti(size_t Count, Fn&& fn, Args&&... args)`
    添加重复的任务，Count为重复的次数。如果Count为0，亦返回true。

    如果线程池已进入退出流程，返回false，否则返回true。

- ##### `bool CThreadPool::Detach()`
    分离所有任务，分离的线程池对象线程数和当前线程池正在运行的线程数相同。

    如果任务队列为空，返回`false`，否则返回`true`。如果当前线程池线程数量为0，未完成的任务不会被执行而是立即销毁。

- ##### `bool CThreadPool::Detach(size_t threadNumNew)`
    分离线程池对象，设置分离的线程池对象线程数为`threadNumNew`。

    如果任务队列为空，返回`false`，否则返回`true`。如果`ThreadNumNew==0`，未完成的任务不会被执行而是立即销毁。

- ##### `size_t CThreadPool::GetThreadNum()`
    获取当前线程中工作线程的数量。

- ##### `bool CThreadPool::AddThreadNum(size_t thread_num_add)`
    增加线程池中工作线程的数量。

    如果线程池已进入退出流程，或者添加的线程数`thread_num_add==0`，返回false，否则返回true。


## 备注

此线程池未使用虚函数，异常安全。默认的线程数为2，实际运用时的线程数应少于CPU核心数。

线程退出代码基址为`success_code=0x00001000`，正常退出时，返回值大于等于`success_code`；
非正常退出时，返回值小于`success_code`。

使用C++11模板类编写，无需链接库文件和单独编译。
`class CThreadPool`不允许通过复制构造对象，不允许复制另一个CThreadPool对象。


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
