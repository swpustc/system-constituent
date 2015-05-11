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
#include <array>
#include <vector>
#include <string>
#include <cstdint>
#include <Windows.h>


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

    enum class _baud_rate_t : DWORD;
    enum class _byte_size_t : BYTE;
    enum class _stop_bits_t : BYTE;
    enum class _parity_t : BYTE;
    enum class _Dtr_t : DWORD;
    enum class _Rts_t : DWORD;

    // 选项类型
    template<class Elem> struct _option{ Elem elem; _option(const Elem& val) : elem(val){} };
    typedef _option<_baud_rate_t> _baud_rate;
    typedef _option<_byte_size_t> _byte_size;
    typedef _option<_stop_bits_t> _stop_bits;
    typedef _option<_parity_t> _parity;
    typedef _option<_Dtr_t> _Dtr;
    typedef _option<_Rts_t> _Rts;

    // 获取当前选项
    SYSCONAPI DCB _get_option() const;

    // 设置选项
    SYSCONAPI bool _set_option(const DCB& dcb);
    // default templete
    template<class Arg, class... Args> bool _set_option(DCB& dcb, const _option<Arg>& arg, const _option<Args>&... args)
    {
        static_assert(false, "Arg must be one of _baud_rate, _byte_size, _stop_bits, _parity, _Dtr, _Rts.");
        return false;
    }
    // 波特率
    template<class... Args> bool _set_option(DCB& dcb, const _option<_baud_rate_t>& arg, const _option<Args>&... args)
    {
        dcb.BaudRate = arg.elem;
        return _set_option(dcb, args...);
    }
    // 数据位
    template<class... Args> bool _set_option(DCB& dcb, const _option<_byte_size>& arg, const _option<Args>&... args)
    {
        dcb.ByteSize = arg.elem;
        return _set_option(dcb, args...);
    }
    // 停止位
    template<class... Args> bool _set_option(DCB& dcb, const _option<_stop_bits>& arg, const _option<Args>&... args)
    {
        dcb.StopBits = arg.elem;
        return _set_option(dcb, args...);
    }
    // 校验位
    template<class... Args> bool _set_option(DCB& dcb, const _option<_parity>& arg, const _option<Args>&... args)
    {
        dcb.fParity = arg.elem ? 0 : 1;
        dcb.Parity = arg.elem;
        return _set_option(dcb, args...);
    }
    // DTR
    template<class... Args> bool _set_option(DCB& dcb, const _option<_Dtr>& arg, const _option<Args>&... args)
    {
        dcb.fDtrControl = arg.elem;
        return _set_option(dcb, args...);
    }
    // RTS
    template<class... Args> bool _set_option(DCB& dcb, const _option<_Rts>& arg, const _option<Args>&... args)
    {
        dcb.fRtsControl = arg.elem;
        return _set_option(dcb, args...);
    }

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

    // 获取当前选项
    DCB get_option() const{ return _get_option(); }
    // 设置串口选项，参数必须是_option<Arg>类型，如果有相同类型的参数，以后一个为准
    template<class... Args> bool set_option(const _option<Args>&... args)
    {
        auto&& dcb = _get_option();
        return _set_option(dcb, args...);
    }
    // 从DCB数据结构设置串口选项
    bool set_option(const DCB& dcb)
    {
        return _set_option(dcb);
    }

    // 从串口读数据，size为缓冲区长度，返回已读取的长度
    SYSCONAPI bool read(void* data, size_t& size);
    template<class T, class A> bool read(::std::basic_string<char, T, A>& data)
    {
        size_t size = (size_t)INT16_MAX;
        data.resize(size);
        bool result = read(const_cast<char*>(data.c_str()), size);
        data.resize(size);
        return result;
    }
    template<class A> bool read(::std::vector<unsigned char, A>& data)
    {
        size_t size = (size_t)INT16_MAX;
        data.resize(size);
        bool result = read(const_cast<unsigned char*>(data.data()), size);
        data.resize(size);
        return result;
    }
    template<class A> bool read(::std::vector<char, A>& data)
    {
        size_t size = (size_t)INT16_MAX;
        data.resize(size);
        bool result = read(const_cast<char*>(data.data()), size);
        data.resize(size);
        return result;
    }

    // 写数据到串口，返回值为已写入的长度
    SYSCONAPI size_t write(const void* data, size_t size);
    template<class T, class A> size_t write(const ::std::basic_string<char, T, A>& data){ return write(data.c_str(), data.length()); }
    template<class A> size_t write(const ::std::vector<unsigned char, A>& data){ return write(data.data(), data.size()); }
    template<class A> size_t write(const ::std::vector<char, A>& data){ return write(data.data(), data.size()); }
    template<size_t size> size_t write(const ::std::array<unsigned char, size>& data){ return write(data.data(), data.size()); }
    template<size_t size> size_t write(const ::std::array<char, size>& data){ return write(data.data(), data.size()); }

    // 波特率选项
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
    // 数据位
    SYSCONAPI static const _byte_size byte_size_4;
    SYSCONAPI static const _byte_size byte_size_5;
    SYSCONAPI static const _byte_size byte_size_6;
    SYSCONAPI static const _byte_size byte_size_7;
    SYSCONAPI static const _byte_size byte_size_8;
    // 停止位
    SYSCONAPI static const _stop_bits stop_bits_1;
    SYSCONAPI static const _stop_bits stop_bits_1_5;
    SYSCONAPI static const _stop_bits stop_bits_2;
    // 校验位
    SYSCONAPI static const _parity parity_none;
    SYSCONAPI static const _parity parity_odd;
    SYSCONAPI static const _parity parity_even;
    SYSCONAPI static const _parity parity_mark;
    SYSCONAPI static const _parity parity_space;
    // DTR
    SYSCONAPI static const _Dtr Dtr_disable;
    SYSCONAPI static const _Dtr Dtr_enable;
    SYSCONAPI static const _Dtr Dtr_handshake;
    // RTS
    SYSCONAPI static const _Rts Rts_disable;
    SYSCONAPI static const _Rts Rts_enable;
    SYSCONAPI static const _Rts Rts_handshake;
    SYSCONAPI static const _Rts Rts_toggle;
};
