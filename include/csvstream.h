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
    // 文件IO
    ::std::fstream m_io;
    // CSV行列数据
    ::std::deque<::std::vector<::std::string>> m_data;

    // 写入单元格 const char*
    SYSCONAPI bool _set_cell(size_t row, size_t col, const char* val);
    // 写入单元格 const string&
    SYSCONAPI bool _set_cell(size_t row, size_t col, const ::std::string& val);
    // 写入单元格 string&&
    SYSCONAPI bool _set_cell(size_t row, size_t col, ::std::string&& val);
    template<class T, class A> // 写入单元格 string
    bool _set_cell(size_t row, size_t col, const ::std::basic_string<char, T, A>& val){ return _set_cell(row, col, val.c_str()); }
    // 写入单元格 const wchar_t*
    bool _set_cell(size_t row, size_t col, const wchar_t* val){ return _set_cell(row, col, convert_default_unicode.to_bytes(val)); }
    template<class T, class A> // 写入单元格 const wstring&
    bool _set_cell(size_t row, size_t col, const ::std::basic_string<wchar_t, T, A>& val){ return _set_cell(row, col, convert_default_unicode.to_bytes(val)); }
    template<class T, class A> // 写入单元格 wstring&&
    bool _set_cell(size_t row, size_t col, ::std::basic_string<wchar_t, T, A>&& val){ return _set_cell(row, col, convert_default_unicode.to_bytes(::std::move(val))); }
    template<class T> // 写入单元格 auto
    bool _set_cell(size_t row, size_t col, T&& val)
    {
        ::std::stringstream ss;
        ss << ::std::forward<T>(val);
        return _set_cell(row, col, ss.str());
    }

    template<class T, class A> // 读取单元格 string&
    bool _get_cell(size_t row, size_t col, ::std::basic_string<char, T, A>& val)
    {
        val.clear();
        // IO读写锁
        lock_guard<mutex> lck(m_lock);
        if (m_data.size() < row)
            return true;
        auto& data_line = m_data.at(row);
        if (data_line.size() < col)
            return true;
        val = data_line.at(col);
        return true;
    }
    template<class T, class A> // 读取单元格 wstring&
    bool _get_cell(size_t row, size_t col, ::std::basic_string<wchar_t, T, A>& val)
    {
        ::std::string val_raw;
        if (_get_cell(row, col, val_raw))
        {
            val = convert_default_unicode.from_bytes(val_raw);
            return true;
        }
        return false;
    }
    template<class T> // 读取单元格 auto
    bool _get_cell(size_t row, size_t col, T& val)
    {
        ::std::string val_raw;
        if (_get_cell(row, col, val_raw))
        {
            ::std::stringstream ss(val_raw);
            ss >> val;
            return true;
        }
        return false;
    }

    bool _set_row(size_t row, size_t col){ return true; }
    template<class Arg, class... Args> // 写入一行
    bool _set_row(size_t row, size_t col, Arg&& arg, Args&&... args)
    {
        _set_cell(row, col, ::std::forward<Arg>(arg));
        return _set_row(row, col + 1, ::std::forward<Args>(args)...);
    }

    bool _set_col(size_t row, size_t col){ return true; }
    template<class Arg, class... Args> // 写入一列
    bool _set_col(size_t row, size_t col, Arg&& arg, Args&&... args)
    {
        _set_cell(row, col, ::std::forward<Arg>(arg));
        return _set_col(row + 1, col, ::std::forward<Args>(args)...);
    }

    bool _get_row(size_t row, size_t col){ return true; }
    template<class Arg, class... Args> // 读取一行
    bool _get_row(size_t row, size_t col, Arg& arg, Args&... args)
    {
        _get_cell(row, col, arg);
        return _get_row(row, col + 1, args...);
    }

    bool _get_col(size_t row, size_t col){ return true; }
    template<class Arg, class... Args> // 读取一列
    bool _get_col(size_t row, size_t col, Arg& arg, Args&... args)
    {
        _get_cell(row, col, arg);
        return _get_col(row + 1, col, args...);
    }

public:
    csvstream(){}
    csvstream(const char* filename){ open(filename); }
    csvstream(const ::std::string& filename){ open(filename); }
#ifdef _WIN32
    csvstream(const wchar_t* filename){ open(filename); }
    csvstream(const ::std::wstring& filename){ open(filename); }
#endif  /* _WIN32 */

    void open(const char* filename){ m_io.open(filename, ::std::ios::app | ::std::ios::in | ::std::ios::out); }
    void open(const ::std::string& filename){ m_io.open(filename, ::std::ios::app | ::std::ios::in | ::std::ios::out); }
#ifdef _WIN32
    void open(const wchar_t* filename){ m_io.open(filename, ::std::ios::app | ::std::ios::in | ::std::ios::out); }
    void open(const ::std::wstring& filename){ m_io.open(filename, ::std::ios::app | ::std::ios::in | ::std::ios::out); }
#endif  /* _WIN32 */

    // 文件是否打开
    bool is_open(){ return m_io.is_open(); }
    // 读取CSV文件
    SYSCONAPI bool read();
    // 写入CSV文件
    SYSCONAPI bool write();

    template<class T> // 写入单元格
    bool set_cell(size_t row, size_t col, T&& val){ return is_open() ? _set_cell(row, col, ::std::forward<T>(val)) : false; }
    template<class T> // 读取单元格
    bool get_cell(size_t row, size_t col, T& val){ return is_open() ? _get_cell(row, col, val) : false; }

    template<class... Args> // 写入一行
    bool set_row(size_t row, Args&&... args){ return is_open() ? _set_row(row, 0, ::std::forward<Args>(args)...) : false; }
    template<class... Args> // 写入一列
    bool set_col(size_t col, Args&&... args){ return is_open() ? _set_col(0, col, ::std::forward<Args>(args)...) : false; }

    template<class... Args> // 读取一行
    bool get_row(size_t row, Args&... args){ return is_open() ? _get_row(row, 0, args...) : false; }
    template<class... Args> // 读取一列
    bool get_col(size_t col, Args&... args){ return is_open() ? _get_col(0, col, args...) : false; }
};
