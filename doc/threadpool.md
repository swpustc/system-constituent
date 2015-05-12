# threadpool class

线程池的创建、调度和销毁。


## 公共接口

源文件：[include/threadpool.h](../include/threadpool.h)

```cpp
template<bool handle_exception = true> class threadpool
{
public:
    static const size_t success_code = 0x00001000;

    threadpool();
    threadpool(int thread_number);
    ~threadpool();
    threadpool(const threadpool&) = delete;
    threadpool& operator=(const threadpool&) = delete;
    threadpool(threadpool&&) = delete;
    threadpool& operator=(threadpool&&) = delete;

    bool start();
    bool pause();
    void stop();
    bool stop_on_completed();

    bool push(Fn&& fn, Args&&... args);
    auto push_future(Fn&& fn, Args&&... args)->std::pair<std::future<fn(args...)>, bool>;
    bool push_multi(size_t Count, Fn&& fn, Args&&... args);
    auto push_multi_future(size_t count, Fn&& fn, Args&&... args)->std::pair<std::vector<std::future<fn(args...)>>, bool>;

    bool detach();
    bool detach(int thread_number_new);
    std::future<size_t> detach_future();
    std::future<size_t> detach_future(int thread_number_new);

    int get_thread_number() const;
    int get_free_thread_number() const;
    size_t get_tasks_number() const;
    size_t get_tasks_completed_number() const;
    size_t get_tasks_total_number() const;

    std::deque<std::function<void()>> get_exception_tasks();
    int get_default_thread_number() const;
    static const type_info& this_type();

    bool set_thread_number(int thread_number);
    bool set_new_thread_number(int thread_number_new);
    bool reset_thread_number();
};
```


## 模板参数

- ##### `bool handle_exception`

    是否处理异常标志。如果处理异常，则会抛出任务，并跳过此任务继续运行。


## 成员变量

- ##### `static const size_t success_code = 0x00001000`

    标准退出代码。


## 成员函数

- ##### `threadpool()`

    默认构造函数，线程池未初始化，直到调用**set_thread_number**初始化线程池。

    未初始化的线程池可以添加、分离任务。

- ##### `threadpool(size_t thread_number)`

    指定线程数的构造函数。

- ##### `~threadpool()`

    默认析构函数，退出时会等待所有任务执行完毕。

- ##### `threadpool(const threadpool&) = delete`

    复制构造函数、复制赋值语句、移动构造函数、移动赋值语句均已删除。

- ##### `bool start()`

    如果线程池已暂停，则启动线程池。

    如果线程池已进入退出流程，返回`false`，否则返回`true`。

- ##### `bool pause()`

    暂停线程池中的所有线程。

    如果线程池已进入退出流程，返回`false`，否则返回`true`。

- ##### `void stop()`

    立即停止线程池，未完成的任务将丢弃。

- ##### `bool stop_on_completed()`

    停止线程池，不再允许添加新的任务；在所有任务完成后，退出工作线程。
    可以调用**stop**以立即停止线程池。

    如果已经调用过**stop**，返回`false`，否则返回`true`。

- ##### `bool push(Fn&& fn, Args&&... args)`

    添加单个任务，fn为函数，args为参数列表。函数可以为函数对象、函数指针、C++98/03/11兼容的仿函数对象、lambda表达式、bind的函数。
    args通过完美转发转发参数，参数可以是任何类型，数量不限。

    传入函数的参数列表的类型和数量必须与传入的参数类型和数量一致，否则会产生编译时错误。

    如果线程池已进入退出流程，返回`false`，否则返回`true`。

- ##### `auto push_future(Fn&& fn, Args&&... args)->std::pair<std::future<fn(args...)>, bool>`

    返回类型为`pair<future, bool>`，可以通过**futurn::get**获取任务函数的返回值。
    其余和**push**函数相同。

- ##### `bool push_multi(size_t Count, Fn&& fn, Args&&... args)`

    添加重复的任务，Count为重复的次数。如果Count为0，亦返回true。

    如果线程池已进入退出流程，返回false，否则返回true。

- ##### `auto push_multi_future(size_t count, Fn&& fn, Args&&... args)->std::pair<std::vector<std::future<fn(args...)>>, bool>`

    返回类型为`pair<vector<future>, bool>`，可以通过**futurn::get**获取任务函数的返回值。
    其余和**push_multi**函数相同。

- ##### `bool detach()`

    分离所有任务，分离的线程池对象线程数和当前线程池正在运行的线程数相同。

    如果任务队列为空，返回`false`，否则返回`true`。如果当前线程池线程数量为0，未完成的任务不会被执行而是立即销毁。

- ##### `bool detach(int thread_number_new)`

    分离所有任务，设置分离的线程池对象线程数为`thread_number_new`。

    如果任务队列为空，返回`false`，否则返回`true`。如果`thread_number_new==0`，未完成的任务不会被执行而是立即销毁。

- ##### `std::future<size_t> detach_future()`

    分离所有任务，分离的线程池对象线程数和当前线程池正在运行的线程数相同。

    返回值为分离的线程池返回情况。如果当前线程池线程数量为0，未完成的任务不会被执行而是立即销毁。

- ##### `std::future<size_t> detach_future(int thread_number_new)`

    分离所有任务，设置分离的线程池对象线程数为`thread_number_new`。

    返回值为分离的线程池返回情况。如果`thread_number_new==0`，未完成的任务不会被执行而是立即销毁。

- ##### `int get_thread_number()`

    获取当前线程池中线程的数量。

- ##### `int get_free_thread_number()`

    获取当前线程池中空闲线程的数量。

- ##### `size_t get_tasks_number()`

    获取当前任务队列中未处理的任务数。

- ##### `size_t get_tasks_completed_number()`

    获取已完成任务数。

- ##### `size_t get_tasks_total_number()`

    获取任务总数。

- ##### `::std::deque<::std::function<void()>> get_exception_tasks()`

    获取抛出异常的任务信息。每次调用此函数会清空异常队列。

- ##### `int get_default_thread_number()`

    获取初始化线程数。

- ##### `static const type_info& this_type()`

    获取类型信息。

- ##### `bool set_thread_number(int thread_number)`

    设置线程池中线程的数量。

    未初始化的线程池，第一次调用**set_thread_number**设置线程数的同时会初始化线程池，此任务非线程安全。

    已初始化的线程池调用此函数将返回`false`。

- ##### `bool set_new_thread_number(int thread_number_new)`

    重新设置线程池中线程的数量。

    如果线程池已进入退出流程，或者新的线程数`thread_num_set<0`，返回`false`，否则返回`true`。

    未初始化的线程池调用此函数将返回`false`。

- ##### `bool reset_thread_number()`

    重置线程池工作线程数量为线程池初始值。


## 备注

线程池未使用虚函数，异常安全。默认的线程数为2，实际运用时的线程数应少于CPU核心数。

线程退出代码基址为`success_code=0x00001000`，正常退出时，返回值大于等于`success_code`；
非正常退出时，返回值小于`success_code`。
如果线程抛出异常，`thread_entry [private]`将捕获异常并将此任务添加到异常任务队列。
如果线程池未准备好，线程返回值为`success_code-0xff`。

分离`detach`的线程池控制函数返回值为`success_code+0xff`。

使用C++11模板类编写，需链接`system.lib`。
`class threadpool`不允许通过复制构造对象，不允许复制另一个`threadpool`对象。


## 示例代码

在测试代码中，可以查看`threadpool`的[示例代码](../test/threadpool.cpp)。

```cpp
// threadpool<> example
#include <threadpool.h>                 // threadpool<>
#include <link_system_constituent.h>    // linker

using namespace std;

void foo(int length, char c)
{
    while (length-- >= 0)
    {
        cout << c;
        this_thread::sleep_for(chrono::milliseconds(100));
    }
}

int main()
{
    set_log_location("threadpool.log"); // set log file location

    threadpool<> thpool;
    thpool.set_thread_number(6); // set threadpool thread number to 6
    // the same as: threadpool<> thpool(6);

    char c = 'A';
    for (int i = 0; i < 16; i++)
        thpool.push(foo, 3 + i % 3, c++); // spawn thread that calls foo(3+i%3, c++)

    // synchronize threadpool: call threadpool<bool>::~threadpool

    return 0;
}
```

输出：
```
ABCFEDBCAFDEBACFEDBCAFEDBGCFEHCGIFJHIGKLHJIGKLJHIKMLHJIMKLNOIKMLONPMLNOPONPNOPO
```


## 要求

项目       |  要求
:--------- |:---------
支持的平台 | Windows
编译器版本 | VS2013+
头文件     | threadpool.h (include system_constituent.h)
库文件     | system130.lib
DLL        | system130.dll


## 参见
