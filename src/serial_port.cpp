/**********************************************************
* 串口对象
* 支持平台：Windows
* 编译环境：VS2013+
* 创建时间：2015-05-10 （宋万鹏）
* 最后修改：2015-05-10 （宋万鹏）
***********************************************************/

#include "common.h"
#include "serial_port.h"
#include <tchar.h>
#include <Windows.h>

using namespace std;


static const wstring comm_name_prefix = L"\\\\.\\";

// 已验证过的串口名
bool serial_port::__open(const wchar_t* portname)
{
    m_comm = CreateFileW(portname, GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
    if (!is_open())
        return false;
    if (!SetCommMask(m_comm, EV_RXCHAR | EV_TXEMPTY))
        return false;
    // 设置输入输出缓冲区大小
    if (!SetupComm(m_comm, 1024, 1024))
        return false;
    //  清除状态位
    if (!PurgeComm(m_comm, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR))
        return false;
    COMMTIMEOUTS comm_timeouts = { 0 };
    if (!SetCommTimeouts(m_comm, &comm_timeouts))
        return false;
    return true;
}


bool serial_port::_open(const wchar_t* portname)
{
    if (wcsncmp(portname, comm_name_prefix.c_str(), comm_name_prefix.size()) == 0)
        return __open(portname);
    else
        return __open(comm_name_prefix + portname);
}

bool serial_port::_open(const ::std::wstring& portname)
{
    if (portname.compare(0, 4, comm_name_prefix) == 0)
        return __open(portname.c_str());
    else
        return __open(comm_name_prefix + portname);
}

bool serial_port::_open(::std::wstring&& portname)
{
    if (portname.compare(0, 4, comm_name_prefix) == 0)
        return __open(portname.c_str());
    else
        return __open(comm_name_prefix + move(portname));
}

bool serial_port::_open(int port_number)
{
    if (port_number <= 0 || port_number > 32)
        return false;
    wchar_t name[10];
    wsprintfW(name, L"\\\\.\\COM%d", port_number);
    return __open(name);
}
