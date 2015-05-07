# csvstream class

线程池的创建、调度和销毁。


## 公共接口

源文件：[include/csvstream.h](../include/csvstream.h)

```cpp
class csvstream
{
public:
    static struct cell_skip_t cell_skip;

    bool read(T&& filename);
    bool write(T&& filename);

    void set_cell(size_t row, size_t col, T&& val);
    void get_cell(size_t row, size_t col, T&& val);

    void set_row(size_t row, Args&&... args);
    void set_col(size_t col, Args&&... args);

    void set_row_begin(size_t row, size_t begin_col, Args&&... args);
    void set_col_begin(size_t col, size_t begin_row, Args&&... args);

    void get_row(size_t row, Args&&... args);
    void get_col(size_t col, Args&&... args);

    void get_row_begin(size_t row, size_t begin_col, Args&&... args);
    void get_col_begin(size_t col, size_t begin_row, Args&&... args);
};
```


## 成员变量

- ##### `static struct cell_skip_t cell_skip`

    跳过单元格标识。


## 成员函数

- ##### `bool read(T&& filename)`

    filename：文件名。

    从文件中读取CSV文件到`csvstream`对象中。

- ##### `bool write(T&& filename)`

    filename：文件名。

    写入对象中的数据到文件。

- ##### `void set_cell(size_t row, size_t col, T&& val)`

    设置单元格的内容，`val`可以是基础数据类型或者`string`，`wstring`，字符串指针等。

- ##### `void get_cell(size_t row, size_t col, T&& val)`

    获取单元格的内容，`val`可以是基础数据类型或者`string`，`wstring`的左值非常量引用。

- ##### `void set_row(size_t row, Args&&... args)`

    设置一行数据，行号为`row`（从0开始），`args`为从第0列开始的按顺序的连续的单元格的数据。遇到`csvstream::cell_skip`则跳过此单元格。

- ##### `void set_col(size_t col, Args&&... args)`

    设置一列数据，列号为`col`（从0开始），`args`为从第0列开始的按顺序的连续的单元格的数据。遇到`csvstream::cell_skip`则跳过此单元格。

- ##### `void set_row_begin(size_t row, size_t begin_col, Args&&... args)`

    和**set_row**相同，区别是从`begin_col`列开始。

- ##### `void set_col_begin(size_t col, size_t begin_row, Args&&... args)`

    和**set_col**相同，区别是从`begin_row`行开始。

- ##### `void get_row(size_t row, Args&&... args)`

    获取一行数据，行号为`row`（从0开始），`args`为从第0列开始的按顺序的连续的单元格的数据。遇到`csvstream::cell_skip`则跳过此单元格。

- ##### `void get_col(size_t col, Args&&... args)`

    获取一列数据，列号为`col`（从0开始），`args`为从第0列开始的按顺序的连续的单元格的数据。遇到`csvstream::cell_skip`则跳过此单元格。

- ##### `void get_row_begin(size_t row, size_t begin_col, Args&&... args)`

    和**get_row**相同，区别是从`begin_col`列开始。

- ##### `void get_col_begin(size_t col, size_t begin_row, Args&&... args)`

    和**get_col**相同，区别是从`begin_row`行开始。


## 示例代码

在测试代码中，可以查看`csvstream`的[示例代码](../test/csvstream.cpp)。

```cpp
// csvstream example
#include <csvstream.h>                  // csvstream
#include <link_system_constituent.h>    // linker

using namespace std;

int main()
{
    set_log_location("csvstream.log"); // set log file location

    csvstream csv;
    csv.set_cell(0, 0, 1);
    csv.set_cell(0, 1, 2.34567);
    csv.set_cell(0, 2, 4ULL);
    csv.set_cell(1, 0, _T("Tstring"));
    csv.set_cell(1, 1, R"_(raw string, "")_");
    csv.write("csvstream.csv");
    return 0;
}
```


## 要求

项目       |  要求
:--------- |:---------
支持的平台 | Windows
编译器版本 | VS2013+
头文件     | csvstream.h
库文件     | system.lib
DLL        | system.dll


## 参见
