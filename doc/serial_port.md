# serial_port class

串口打开、设置、读取、写入。


## 公共接口

源文件：[include/serial_port.h](../include/serial_port.h)

```cpp
class serial_port
{
public:
    serial_port() = default;
    serial_port(const serial_port&) = delete;
    serial_port& operator=(const serial_port&) = delete;

    bool open(T&& portname);
    bool is_open() const;
    bool close();

    DCB get_option() const;
    bool set_option(const _option<Args>&... args);
    bool set_option(const DCB& dcb);

    bool read(void* data, size_t& size);
    bool read(std::basic_string<char, T, A>& data);
    bool read(std::vector<unsigned char, A>& data);
    bool read(std::vector<char, A>& data);

    size_t write(const void* data, size_t size);
    size_t write(const std::basic_string<char, T, A>& data, size_t size = -1);
    size_t write(const std::vector<unsigned char, A>& data, size_t size = -1);
    size_t write(const std::vector<char, A>& data, size_t size = -1);
    size_t write(const std::array<unsigned char, size>& data, size_t size = -1);
    size_t write(const std::array<char, size>& data, size_t size = -1);

    void clear();
};
```


## 成员函数

- ##### `serial_port()`

    默认构造函数。

- ##### `open(T&& portname)`

    打开串口，portname可以是串口名：`"COM1"`，也可以是串口序号：`1,2,3...`。

- ##### `bool is_open()`

    串口是否已经打开。

- ##### `bool close()`

    关闭串口。如果串口未打开或已关闭则失败。

- ##### `DCB get_option()`

    返回串口配置选项。如果串口未打开则返回一个默认配置。

- ##### `bool set_option(const _option<Args>&... args)`

    设置串口选项。如果设置两个相同的选项类型，则以后面一个选项值为准。选项值参见备注。

    各个选项值得顺序可以任意。

- ##### `bool set_option(const DCB& dcb)`

    从串口选项结构体`DCB`配置串口。

- ##### `bool read(void* data, size_t& size)`

    读取串口数据。`data`为缓冲区指针，`size`为缓冲区大小。
    如果读取串口数据成功，`size`为已读取的长度。

- ##### `bool read(std::basic_string<char, T, A>& data)`

    读取串口数据到`basic_string<char>`容器。

- ##### `bool read(std::vector<unsigned char, A>& data)`

    读取串口数据到`vector<unsigned char>`容器。

- ##### `bool read(std::vector<char, A>& data)`

    读取串口数据到`vector<char>`容器。

- ##### `size_t write(const void* data, size_t size)`

    写入数据到串口，返回值为写入的长度。

- ##### `size_t write(const std::basic_string<char, T, A>& data, size_t size = -1)`

    写入`basic_string<char>`容器中的数据到串口，返回值为写入的长度。写入的长度为`size`与`data.size()`的较小的值。

- ##### `size_t write(const std::vector<unsigned char, A>& data, size_t size = -1)`

    写入`vector<unsigned char>`容器中的数据到串口，返回值为写入的长度。写入的长度为`size`与`data.size()`的较小的值。

- ##### `size_t write(const std::vector<char, A>& data, size_t size = -1)`

    写入`vector<char>`容器中的数据到串口，返回值为写入的长度。写入的长度为`size`与`data.size()`的较小的值。

- ##### `size_t write(const std::array<unsigned char, size>& data, size_t size = -1)`

    写入`array<unsigned char>`容器中的数据到串口，返回值为写入的长度。写入的长度为`size`与`data.size()`的较小的值。

- ##### `size_t write(const std::array<char, size>& data, size_t size = -1)`

    写入`array<char>`容器中的数据到串口，返回值为写入的长度。写入的长度为`size`与`data.size()`的较小的值。

- ##### `void clear()`

    清空串口错误标志。


## 备注

串口对象可移动，不可复制。

设置选项函数`set_option`的参数为以下值之一：

```cpp
/* 波特率 */
serial_port::baud_rate_110;
serial_port::baud_rate_300;
serial_port::baud_rate_600;
serial_port::baud_rate_1200;
serial_port::baud_rate_2400;
serial_port::baud_rate_4800;
serial_port::baud_rate_9600;
serial_port::baud_rate_14400;
serial_port::baud_rate_19200;
serial_port::baud_rate_38400;
serial_port::baud_rate_56000;
serial_port::baud_rate_57600;
serial_port::baud_rate_115200;
serial_port::baud_rate_128000;
serial_port::baud_rate_256000;
/* 数据位 */
serial_port::byte_size_4;
serial_port::byte_size_5;
serial_port::byte_size_6;
serial_port::byte_size_7;
serial_port::byte_size_8;
/* 停止位 */
serial_port::stop_bits_1;
serial_port::stop_bits_1_5;
serial_port::stop_bits_2;
/* 校验位 */
serial_port::parity_none;
serial_port::parity_odd;
serial_port::parity_even;
serial_port::parity_mark;
serial_port::parity_space;
/* DTR */
serial_port::Dtr_disable;
serial_port::Dtr_enable;
serial_port::Dtr_handshake;
/* RTS */
serial_port::Rts_disable;
serial_port::Rts_enable;
serial_port::Rts_handshake;
serial_port::Rts_toggle;
```


## 示例代码

```cpp
// serial_port example
#include <serial_port.h>                // serial_port
#include <link_system_constituent.h>    // linker

using namespace std;

int main()
{
    set_log_location("serial_port.log"); // set log file location

    serial_port com;
    if (com.open(1)) // open COM1
    {   // the same as: com.open("COM1");
        com.set_option(
            serial_port::baud_rate_9600,
            serial_port::byte_size_8,
            serial_port::stop_bits_1,
            serial_port::parity_none,
            serial_port::Dtr_disable,
            serial_port::Rts_disable);

        char c = 'A';

        com.write(&c ,1); // write char 'A'

        size_t size = sizeof(c);
        com.read(&c, size); // read char to c

        com.close();
    }

    return 0;
}
```


## 要求

项目       |  要求
:--------- |:---------
支持的平台 | Windows
编译器版本 | VS2013+
头文件     | serial_port.h (include system_constituent.h)
库文件     | system132.lib
DLL        | system132.dll


## 参见
