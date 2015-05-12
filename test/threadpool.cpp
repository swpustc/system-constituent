/**********************************************************
* 测试线程池 threadpool<>
* 支持平台：Windows
* 编译环境：VS2013+
***********************************************************/

// threadpool<> example
#include <threadpool.h>                 // threadpool<>
#include <link_system_constituent.h>    // linker

using namespace std;
using namespace chrono;
using namespace placeholders;

void foo(int length, char c, size_t ms)
{
    while (length-- > 0)
    {
        debug_output<true>(c);
        this_thread::sleep_for(milliseconds(ms));
    }
}

int main()
{
    set_log_location("threadpool.log"); // 设置日志文件存储路径为当前目录

    threadpool<true> thpool1; // 未初始化的线程池，处理异常
    threadpool<false> thpool2(4); // 创建同时初始化线程池，不处理异常

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
    debug_output<true>(_T("fut完成时间："), us / 1000000ll, _T('s'), us % 1000000ll, _T("us"));

    thpool2.push([](char c, size_t ms){foo(1, c, ms); }, '5', 300); // 测试lambda
    thpool2.push(bind(foo, 1, '6', 300)); // 测试bind
    thpool2.push(bind(foo, _1, _2, 300), 1, '7'); // 测试bind+placeholders
    auto&& bind_obj = bind(foo, 1, _1, 300);
    thpool2.push(ref(bind_obj), '8'); // 测试function_wrapper

    c = 'A';
    for (int i = 0; i < 32; i++)
        thpool1.push(foo, 3 + i % 3, c++, (size_t)100 + i); // spawn thread that calls foo(3+i%3, c++, 100+i)
    thpool1.set_thread_number(2); // 设置线程数以初始化线程

    auto&& thread_number = thpool1.get_thread_number();
    auto&& free_thread_number = thpool1.get_free_thread_number();
    auto&& tasks_number = thpool1.get_tasks_number();
    auto&& tasks_completed_number = thpool1.get_tasks_completed_number();
    auto&& tasks_total_number = thpool1.get_tasks_total_number();
    auto&& default_thread_number = thpool1.get_default_thread_number();
    auto&& threadpool_type = thpool1.this_type();
    thpool1.set_new_thread_number(thread_number + 1);
    auto&& new_thread_number = thpool1.get_thread_number();

    // 暂停线程池
    thpool1.pause();

    debug_output<true>(_T("thread_number: "), thread_number, _T("\nfree_thread_number: "), free_thread_number,
        _T("\ntasks_number: "), tasks_number, _T("\ntasks_completed_number: "), tasks_completed_number,
        _T("\ntasks_total_number: "), tasks_total_number, _T("\ndefault_thread_number: "), default_thread_number,
        _T("\nthreadpool_type: "), threadpool_type.name(), _T("\nnew_thread_number: "), new_thread_number);

    auto fut_res = thpool1.detach_future(4);
    thpool1.start();

    // 同步线程池分离的任务
    debug_output<true>(_T("detach thread result: "), fut_res.get());

    return 0;
}
