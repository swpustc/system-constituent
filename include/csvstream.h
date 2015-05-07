/**********************************************************
* CSV逗号分隔文件读写类
* 支持平台：Windows; Linux
* 编译环境：VS2013+; g++ -std=c++11
* 创建时间：2015-05-07 （宋万鹏）
* 最后修改：2015-05-07 （宋万鹏）
***********************************************************/

#pragma once

#include "common.h"
#include <mutex>
#include <regex>
#include <deque>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>

// CSV逗号分隔文件
class csvstream
{
private:
    // 锁定文件读写
    ::std::mutex m_lock;
    // CSV行列数据
    ::std::deque<::std::vector<::std::string>> m_data;

    // 写入单元格 const char*
    SYSCONAPI void _set_cell(size_t row, size_t col, const char* val);
    // 写入单元格 const string&
    SYSCONAPI void _set_cell(size_t row, size_t col, const ::std::string& val);
    // 写入单元格 string&&
    SYSCONAPI void _set_cell(size_t row, size_t col, ::std::string&& val);
    template<class T, class A> // 写入单元格 string
    void _set_cell(size_t row, size_t col, const ::std::basic_string<char, T, A>& val){ _set_cell(row, col, val.c_str()); }
    // 写入单元格 const wchar_t*
    void _set_cell(size_t row, size_t col, const wchar_t* val){ _set_cell(row, col, convert_default_unicode.to_bytes(val)); }
    template<class T, class A> // 写入单元格 const wstring&
    void _set_cell(size_t row, size_t col, const ::std::basic_string<wchar_t, T, A>& val){ _set_cell(row, col, convert_default_unicode.to_bytes(val)); }
    template<class T, class A> // 写入单元格 wstring&&
    void _set_cell(size_t row, size_t col, ::std::basic_string<wchar_t, T, A>&& val){ _set_cell(row, col, convert_default_unicode.to_bytes(::std::move(val))); }
    template<class T> // 写入单元格 auto
    void _set_cell(size_t row, size_t col, T&& val)
    {
        ::std::stringstream ss;
        ss << ::std::forward<T>(val);
        _set_cell(row, col, ss.str());
    }

    template<class T, class A> // 读取单元格 string&
    void _get_cell(size_t row, size_t col, ::std::basic_string<char, T, A>& val)
    {
        val.clear();
        // IO读写锁
        lock_guard<mutex> lck(m_lock);
        if (m_data.size() < row)
            return;
        auto& data_line = m_data.at(row);
        if (data_line.size() < col)
            return;
        val = data_line.at(col);
    }
    template<class T, class A> // 读取单元格 wstring&
    void _get_cell(size_t row, size_t col, ::std::basic_string<wchar_t, T, A>& val)
    {
        ::std::string val_raw;
        _get_cell(row, col, val_raw);
        val = convert_default_unicode.from_bytes(::std::move(val_raw));
    }
    template<class T> // 读取单元格 auto
    void _get_cell(size_t row, size_t col, T& val)
    {
        ::std::string val_raw;
        _get_cell(row, col, val_raw);
        ::std::stringstream ss(::std::move(val_raw));
        ss >> val;
    }

    void _set_row(size_t row, size_t col){}
    template<class Arg, class... Args> // 写入一行
    void _set_row(size_t row, size_t col, Arg&& arg, Args&&... args)
    {
        _set_cell(row, col, ::std::forward<Arg>(arg));
        _set_row(row, col + 1, ::std::forward<Args>(args)...);
    }

    void _set_col(size_t row, size_t col){}
    template<class Arg, class... Args> // 写入一列
    void _set_col(size_t row, size_t col, Arg&& arg, Args&&... args)
    {
        _set_cell(row, col, ::std::forward<Arg>(arg));
        _set_col(row + 1, col, ::std::forward<Args>(args)...);
    }

    void _get_row(size_t row, size_t col){}
    template<class Arg, class... Args> // 读取一行
    void _get_row(size_t row, size_t col, Arg& arg, Args&... args)
    {
        _get_cell(row, col, arg);
        _get_row(row, col + 1, args...);
    }

    void _get_col(size_t row, size_t col){}
    template<class Arg, class... Args> // 读取一列
    void _get_col(size_t row, size_t col, Arg& arg, Args&... args)
    {
        _get_cell(row, col, arg);
        _get_col(row + 1, col, args...);
    }

    ::std::fstream _open(const char* filename, ::std::ios::openmode mode){ return ::std::fstream(filename, mode); }
    ::std::fstream _open(const ::std::string& filename, ::std::ios::openmode mode){ return ::std::fstream(filename, mode); }
#ifdef _WIN32
    ::std::fstream _open(const wchar_t* filename, ::std::ios::openmode mode){ return ::std::fstream(filename, mode); }
    ::std::fstream _open(const ::std::wstring& filename, ::std::ios::openmode mode){ return ::std::fstream(filename, mode); }
#else  /* UNIX */
    ::std::fstream _open(const wchar_t* filename, ::std::ios::openmode mode){ return ::std::fstream(convert_utf8_unicode.to_bytes(filename), mode); }
    ::std::fstream _open(const ::std::wstring& filename, ::std::ios::openmode mode){ return ::std::fstream(convert_utf8_unicode.to_bytes(filename), mode); }
#endif  /* _WIN32 */

    SYSCONAPI bool _read(::std::fstream&& svcstream);
    SYSCONAPI bool _write(::std::fstream&& svcstream);

public:
    csvstream(){}

    template<class T> // 读取CSV文件
    bool read(T&& filename){ return _read(_open(::std::forward<T>(filename), ::std::ios::in)); }
    template<class T> // 写入CSV文件
    bool write(T&& filename){ return _write(_open(::std::forward<T>(filename), ::std::ios::out | ::std::ios::trunc)); }

    template<class T> // 写入单元格
    void set_cell(size_t row, size_t col, T&& val){ _set_cell(row, col, ::std::forward<T>(val)); }
    template<class T> // 读取单元格
    void get_cell(size_t row, size_t col, T& val){ _get_cell(row, col, val); }

    template<class... Args> // 写入一行
    void set_row(size_t row, Args&&... args){ _set_row(row, 0, ::std::forward<Args>(args)...); }
    template<class... Args> // 写入一列
    void set_col(size_t col, Args&&... args){ _set_col(0, col, ::std::forward<Args>(args)...); }

    template<class... Args> // 读取一行
    void get_row(size_t row, Args&... args){ _get_row(row, 0, args...); }
    template<class... Args> // 读取一列
    void get_col(size_t col, Args&... args){ _get_col(0, col, args...); }
};
