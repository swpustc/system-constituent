/**********************************************************
* 测试线程池 threadpool<>
* 支持平台：Windows
* 编译环境：VS2013+
* 创建时间：2015-05-02 （宋万鹏）
* 最后修改：2015-05-02 （宋万鹏）
***********************************************************/

// threadpool<> example
#include <threadpool.h>                 // threadpool<>
#include <link_system_constituent.h>    // linker

using namespace std;
using namespace chrono;
using namespace placeholders;

void foo(int length, char c, size_t ms)
{
    while (length-- >= 0)
    {
        debug_output<true>(c);
        this_thread::sleep_for(milliseconds(ms));
    }
}

int main()
{
    set_log_location("threadpool.log"); // 设置日志文件存储路径为当前目录

    threadpool<> thpool1; // 未初始化的线程池
    threadpool<> thpool2(4); // 创建同时初始化线程池

    char c = 'a';
    for (int i = 0; i < 16; i++)
        thpool2.push(foo, 3 + i % 3, c++, (size_t)100 + i); // spawn thread that calls foo(3+i%3, c++, 100+i)

    // 功能性测试
    thpool2.push(foo, 1, '1', 300); // push
    auto fut1 = thpool2.push_future(foo, 1, '2', 300); // push_future
    thpool2.push_multi(2, foo, 1, '3', 300); // push_multi
    auto fut2 = thpool2.push_multi_future(2, foo, 1, '4', 300); // push_multi_future
    timeval begin, end;
    gettimeofday(&begin, nullptr); // 获取启动时间
    if (fut1.second)
        fut1.first.get();
    if (fut2.second)
        for (auto& fut : fut2.first)
            fut.get();
    gettimeofday(&end, nullptr); // 获取结束时间

    auto us = end.tv_sec * 1000000ll + end.tv_usec - begin.tv_sec * 1000000ll + begin.tv_usec;
    debug_output(_T("fut完成时间："), us / 1000000ll, _T('s'), us % 1000000ll, _T("us"));

    thpool2.push([](char c, size_t ms){foo(1, c, ms); }, '5', 300); // 测试lambda
    thpool2.push(bind(foo, 1, '6', 300)); // 测试bind
    thpool2.push(bind(foo, _1, _2, 300), 1, '7'); // 测试bind+placeholders
    auto&& bind_obj = bind(foo, 1, _1, 300);
    thpool2.push(ref(bind_obj), '8'); // 测试function_wrapper

    c = 'A';
    for (int i = 0; i < 32; i++)
        thpool1.push(foo, 3 + i % 3, c++, (size_t)100 + i); // spawn thread that calls foo(3+i%3, c++, 100+i)
    thpool1.set_thread_number(2); // 设置线程数以初始化线程

    thpool1.pause();

    auto&& thread_number = thpool1.get_thread_number();
    auto&& free_thread_number = thpool1.get_free_thread_number();
    auto&& tasks_number = thpool1.get_tasks_number();
    auto&& tasks_completed_number = thpool1.get_tasks_completed_number();
    auto&& tasks_total_number = thpool1.get_tasks_total_number();
    auto&& default_thread_number = thpool1.get_default_thread_number();
    auto&& threadpool1_type = thpool1.this_type();
    thpool1.set_new_thread_number(thread_number + thread_number);
    auto&& new_thread_number = thpool1.get_thread_number();

    auto fut_res = thpool1.push_multi_future(4, foo, 1, '9', 100); // push_multi_future

    thpool1.start();

    thpool1.detach(4);
    // 同步线程池分离的任务
    if (fut_res.second)
        for (auto& fut : fut_res.first)
            fut.get();

    return 0;
}
