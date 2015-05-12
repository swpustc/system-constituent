/**********************************************************
* 线程池控制类（生成宏）
* 支持平台：Windows
* 编译环境：VS2013+
***********************************************************/

#include "threadpool.h"

using namespace std;

#define HANDLE_EXCEPTION true
#include "xxthreadpool.h"
#undef HANDLE_EXCEPTION

#define HANDLE_EXCEPTION false
#include "xxthreadpool.h"
