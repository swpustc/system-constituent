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
    csv.set_cell(0, 0, 1);
    csv.set_cell(0, 1, 2.34567);
    csv.set_cell(0, 2, 4ULL);
    csv.set_cell(0, 3, _T("Tstring"));
    csv.set_cell(0, 4, R"_(raw string, "")_");

    csv.set_row(1, 'a', "b", 3, L"wide", "multi");
    csv.set_col(5, wstring(L"wstring中文"), string("string中文"));

    int c_0_0;
    double c_0_1;
    unsigned long long c_0_2;
    wstring c_0_3;
    string c_0_4;
    string c_1[5];

    string c_0_5;
    wstring c_1_5;

    csv.get_cell(0, 0, c_0_0);
    csv.get_cell(0, 1, c_0_1);
    csv.get_cell(0, 2, c_0_2);
    csv.get_cell(0, 3, c_0_3);
    csv.get_cell(0, 4, c_0_4);

    csv.get_row(1, c_1[0], c_1[1], c_1[2], c_1[3], c_1[4]);
    csv.get_col(5, c_0_5, c_1_5);

    debug_output<true>(c_0_0);
    debug_output<true>(c_0_1);
    debug_output<true>(c_0_2);
    debug_output<true>(c_0_3);
    debug_output<true>(c_0_4);

    debug_output<true>(c_1[0]);
    debug_output<true>(c_1[1]);
    debug_output<true>(c_1[2]);
    debug_output<true>(c_1[3]);
    debug_output<true>(c_1[4]);

    debug_output<true>(c_0_5);
    debug_output<true>(c_1_5);

    csv.write();
    return 0;
}
