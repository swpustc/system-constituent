/**********************************************************
* 串口对象定义
* 支持平台：Windows
* 编译环境：VS2013+
* 创建时间：2015-05-10 （宋万鹏）
* 最后修改：2015-05-10 （宋万鹏）
***********************************************************/

#pragma once

#include "common.h"
#include "safe_object.h"


class serial_port
{
private:
    // 串口句柄
    SAFE_HANDLE_OBJECT m_comm;

    // 已验证过的串口名
    SYSCONAPI bool __open(const wchar_t* portname);
    bool __open(const ::std::wstring& portname){ return __open(portname.c_str()); }
    bool _open(const char* portname){ return _open(convert_default_unicode.from_bytes(portname)); }
    bool _open(const ::std::string& portname){ return _open(convert_default_unicode.from_bytes(portname)); }
    bool _open(::std::string&& portname){ return _open(convert_default_unicode.from_bytes(::std::move(portname))); }
    SYSCONAPI bool _open(const wchar_t* portname);
    SYSCONAPI bool _open(const ::std::wstring& portname);
    SYSCONAPI bool _open(::std::wstring&& portname);
    SYSCONAPI bool _open(int port_number);
    bool _open(unsigned int port_number){ return _open((int)(port_number)); }

    // 获取默认选项类型
    SYSCONAPI DCB _get_option_template() const;


    enum class _baud_rate_t : DWORD;

    // 选项类型
    template<class Elem> struct _option{ Elem elem; _option(const Elem& val) : elem(val){} };

    typedef _option<_baud_rate_t> _baud_rate;




public:
    serial_port() = default;
    serial_port(const serial_port&) = delete;
    serial_port& operator=(const serial_port&) = delete;

    // 打开串口：portname可以为串口名(string|pointer)或串口序号(int 1,2,3...)
    template<class T> bool open(T&& portname){ return _open(::std::forward<T>(portname)); }
    // 串口是否已打开
    bool is_open() const
    {
        return m_comm.is_open();
    }
    // 关闭串口
    bool close()
    {
        if (is_open())
        {
            m_comm.close();
            return true;
        }
        else
            return false;
    }

    bool set_option(){}

    SYSCONAPI static const _baud_rate baud_rate_110;
    SYSCONAPI static const _baud_rate baud_rate_300;
    SYSCONAPI static const _baud_rate baud_rate_600;
    SYSCONAPI static const _baud_rate baud_rate_1200;
    SYSCONAPI static const _baud_rate baud_rate_2400;
    SYSCONAPI static const _baud_rate baud_rate_4800;
    SYSCONAPI static const _baud_rate baud_rate_9600;
    SYSCONAPI static const _baud_rate baud_rate_14400;
    SYSCONAPI static const _baud_rate baud_rate_19200;
    SYSCONAPI static const _baud_rate baud_rate_38400;
    SYSCONAPI static const _baud_rate baud_rate_56000;
    SYSCONAPI static const _baud_rate baud_rate_57600;
    SYSCONAPI static const _baud_rate baud_rate_115200;
    SYSCONAPI static const _baud_rate baud_rate_128000;
    SYSCONAPI static const _baud_rate baud_rate_256000;
};
