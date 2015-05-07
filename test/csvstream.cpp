/**********************************************************
* 测试CSV文档 csvstream
* 支持平台：Windows; Linux
* 编译环境：VS2013+; g++ -std=c++11
* 创建时间：2015-05-07 （宋万鹏）
* 最后修改：2015-05-07 （宋万鹏）
***********************************************************/

// csvstream example
#include <csvstream.h>                  // csvstream
#include <link_system_constituent.h>    // linker

using namespace std;

int main()
{
    set_log_location("csvstream.log"); // 设置日志文件存储路径为当前目录

    csvstream csv; // CSV文档对象
    csv.open("csvstream.csv");
    csv.read();

    return 0;
}
