# csvstream class

线程池的创建、调度和销毁。


## 公共接口

源文件：[include/csvstream.h](../include/csvstream.h)

```cpp
class csvstream
{
public:
    static const skip_cell_t skip_cell;
    static const sync_set_t sync_set;
    static const sync_get_t sync_get;

    std::unique_lock<spin_mutex> align_bound();

    void read_from_stream(::std::istream& csv);
    bool read(T&& filename);
    void write_to_stream(::std::ostream& csv);
    bool write(T&& filename);

    void set_cell(size_t row, size_t col, T&& val);
    void get_cell(size_t row, size_t col, T&& val) const;
    void sync_cell(Sync&& sync, size_t row, size_t col, T&& val);

    void set_row(size_t row, Args&&... args);
    void get_row(size_t row, Args&&... args) const;
    void sync_row(Sync&& sync, size_t row, Args&&... args);

    void set_row_begin(size_t row, size_t begin_col, Args&&... args);
    void get_row_begin(size_t row, size_t begin_col, Args&&... args) const;
    void sync_row_begin(Sync&& sync, size_t row, size_t begin_col, Args&&... args);

    void set_col(size_t col, Args&&... args);
    void get_col(size_t col, Args&&... args) const;
    void sync_col(Sync&& sync, size_t col, Args&&... args);

    void set_col_begin(size_t col, size_t begin_row, Args&&... args);
    void get_col_begin(size_t col, size_t begin_row, Args&&... args) const;
    void sync_col_begin(Sync&& sync, size_t col, size_t begin_row, Args&&... args);

    void clear();
    void clear_row(size_t row);
    void clear_col(size_t col);

    void erase_row(size_t row);
    void erase_col(size_t col);

    void insert_row(size_t row, Args&&... args);
    void insert_col(size_t col, Args&&... args);

    void insert_row_begin(size_t row, size_t begin_col, Args&&... args);
    void insert_col_begin(size_t col, size_t begin_row, Args&&... args);

    void swap(csvstream& right);
    void swap_row(size_t row1, size_t row2);
    void swap_col(size_t col1, size_t col2);
};
```


## 成员变量

- ##### `static const skip_cell_t skip_cell`

    跳过单元格标识。

- ##### `static const sync_set_t sync_set`

    sync开头的函数调用set功能标识。

- ##### `static const sync_get_t sync_get`

    sync开头的函数调用get功能标识。


## 成员函数

- ##### `std::unique_lock<spin_mutex> align_bound()`

    对齐数据边界，移除空白行和空白列。获取返回值可以延长锁定对象生命周期，以继续后续操作。

- ##### `void read_from_stream(::std::istream& csv)`

    从流`csv`中读取CSV文件到`csvstream`对象中。

- ##### `bool read(T&& filename)`

    filename：文件名。

    从文件中读取CSV文件到`csvstream`对象中。

- ##### `void write_to_stream(::std::ostream& csv)`

    写入CSV文件到流`csv`中。

- ##### `bool write(T&& filename)`

    filename：文件名。

    写入对象中的数据到文件。

- ##### `void set_cell(size_t row, size_t col, T&& val)`

    设置单元格的内容，`val`可以是基础数据类型或者`string`，`wstring`，字符串指针等。

- ##### `void get_cell(size_t row, size_t col, T&& val)`

    获取单元格的内容，`val`可以是基础数据类型或者`string`，`wstring`的左值非常量引用。

- ##### `void sync_cell(Sync&& sync, size_t row, size_t col, T&& val)`

    传入`sync`值为`csvstream::sync_set`时，调用**set_cell**函数；
    传入`sync`值为`csvstream::sync_get`时，调用**get_cell**函数。

- ##### `void set_row(size_t row, Args&&... args)`

    设置一行数据，行号为`row`（从0开始），`args`为从第0列开始的按顺序的连续的单元格的数据。遇到`csvstream::skip_cell`则跳过此单元格。

- ##### `void get_row(size_t row, Args&&... args)`

    获取一行数据，行号为`row`（从0开始），`args`为从第0列开始的按顺序的连续的单元格的数据。遇到`csvstream::skip_cell`则跳过此单元格。

- ##### `void sync_row(Sync&& sync, size_t row, Args&&... args)`

    传入`sync`值为`csvstream::sync_set`时，调用**set_row**函数；
    传入`sync`值为`csvstream::sync_get`时，调用**get_row**函数。

- ##### `void set_row_begin(size_t row, size_t begin_col, Args&&... args)`

    和**set_row**相同，区别是从`begin_col`列开始。

- ##### `void get_row_begin(size_t row, size_t begin_col, Args&&... args)`

    和**get_row**相同，区别是从`begin_col`列开始。

- ##### `void sync_row_begin(Sync&& sync, size_t row, size_t begin_col, Args&&... args)`

    传入`sync`值为`csvstream::sync_set`时，调用**set_row_begin**函数；
    传入`sync`值为`csvstream::sync_get`时，调用**get_row_begin**函数。

- ##### `void set_col(size_t col, Args&&... args)`

    设置一列数据，列号为`col`（从0开始），`args`为从第0列开始的按顺序的连续的单元格的数据。遇到`csvstream::skip_cell`则跳过此单元格。

- ##### `void get_col(size_t col, Args&&... args)`

    获取一列数据，列号为`col`（从0开始），`args`为从第0列开始的按顺序的连续的单元格的数据。遇到`csvstream::skip_cell`则跳过此单元格。

- ##### `void sync_col(Sync&& sync, size_t col, Args&&... args)`

    传入`sync`值为`csvstream::sync_set`时，调用**set_col**函数；
    传入`sync`值为`csvstream::sync_get`时，调用**get_col**函数。

- ##### `void set_col_begin(size_t col, size_t begin_row, Args&&... args)`

    和**set_col**相同，区别是从`begin_row`行开始。

- ##### `void get_col_begin(size_t col, size_t begin_row, Args&&... args)`

    和**get_col**相同，区别是从`begin_row`行开始。

- ##### `void sync_col_begin(Sync&& sync, size_t col, size_t begin_row, Args&&... args)`

    传入`sync`值为`csvstream::sync_set`时，调用**set_col_begin**函数；
    传入`sync`值为`csvstream::sync_get`时，调用**get_col_begin**函数。

- ##### `void clear()`

    清除整个工作表。

- ##### `void clear_row(size_t row)`

    清除表格一行的内容。

- ##### `void clear_col(size_t col)`

    清除表格一列的内容。

- ##### `void erase_row(size_t row)`

    删除表格一行。

- ##### `void erase_col(size_t col)`

    删除表格一列。

- ##### `void insert_row(size_t row, Args&&... args)`

    在第row行前插入新行，args为插入新行的内容，可以为空。遇到`csvstream::skip_cell`则跳过此单元格。

- ##### `void insert_col(size_t col, Args&&... args)`

    在第col列左插入新列，args为插入新行的内容，可以为空。遇到`csvstream::skip_cell`则跳过此单元格。

- ##### `void insert_row_begin(size_t row, size_t begin_col, Args&&... args)`

    和**insert_row**相同，区别是从`begin_col`列开始。

- ##### `void insert_col_begin(size_t col, size_t begin_row, Args&&... args)`

    和**insert_col**相同，区别是从`begin_row`行开始。

- ##### `void swap(csvstream& right)`

    交换两个`csvstream`对象的数据。

- ##### `void swap_row(size_t row1, size_t row2)`

    交换行号为row1和row2的两行。

- ##### `void swap_col(size_t col1, size_t col2)`

    交换列号为col1和col2的两列。


## 备注

行号和列号均从0开始编号，`csvstream::skip_cell`可以用在所有的单元格值操作中，
读取和写入单元格的值为此类型时，会跳过此单元格的读取或写入，不会改变所指单元格的值，也不会改变传入的引用对象的值。

`csvstream`对象具有移动构造函数和移动赋值语句，不能被复制。其多线程安全，使用自旋锁。


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
头文件     | csvstream.h (include system_constituent.h)
库文件     | system133.lib
DLL        | system133.dll


## 参见
