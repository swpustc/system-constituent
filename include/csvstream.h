/**********************************************************
* CSV逗号分隔文件读写类
* 支持平台：Windows; Linux
* 编译环境：VS2013+; g++ -std=c++11
* 创建时间：2015-05-07 （宋万鹏）
* 最后修改：2015-05-08 （宋万鹏）
***********************************************************/

#pragma once

#include "common.h"
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
    spin_mutex m_lock;
    // CSV行列数据
    ::std::deque<::std::vector<::std::string>> m_data;

    // 跳过单元格标识类型
    struct skip_cell_t{};

    // 写入单元格 const char*
    void _set_cell(size_t row, size_t col, const char* val){ _set_cell(row, col, ::std::string(val)); }
    void _set_cell(size_t row, size_t col, char* val){ _set_cell(row, col, ::std::string(val)); }
    void _set_cell(size_t row, size_t col, volatile char* val){ _set_cell(row, col, (const char*)(val)); }
    void _set_cell(size_t row, size_t col, const volatile char* val){ _set_cell(row, col, (const char*)(val)); }
    // 写入单元格 const string&
    SYSCONAPI void _set_cell(size_t row, size_t col, const ::std::string& val);
    // 写入单元格 string&
    void _set_cell(size_t row, size_t col, ::std::string& val){ _set_cell(row, col, (const ::std::string&)(val)); }
    // 写入单元格 string&&
    SYSCONAPI void _set_cell(size_t row, size_t col, ::std::string&& val);
    template<class T, class A> // 写入单元格 const string&
    void _set_cell(size_t row, size_t col, const ::std::basic_string<char, T, A>& val){ _set_cell(row, col, ::std::string(val)); }
    template<class T, class A> // 写入单元格 string&
    void _set_cell(size_t row, size_t col, ::std::basic_string<char, T, A>& val){ _set_cell(row, col, ::std::string(val)); }
    template<class T, class A> // 写入单元格 string&&
    void _set_cell(size_t row, size_t col, ::std::basic_string<char, T, A>&& val){ _set_cell(row, col, ::std::string(val)); }
    // 写入单元格 const wchar_t*
    void _set_cell(size_t row, size_t col, const wchar_t* val){ _set_cell(row, col, convert_default_unicode.to_bytes(val)); }
    void _set_cell(size_t row, size_t col, wchar_t* val){ _set_cell(row, col, convert_default_unicode.to_bytes(val)); }
    void _set_cell(size_t row, size_t col, volatile wchar_t* val){ _set_cell(row, col, (const wchar_t*)(val)); }
    void _set_cell(size_t row, size_t col, const volatile wchar_t* val){ _set_cell(row, col, (const wchar_t*)(val)); }
    template<class T, class A> // 写入单元格 const wstring&
    void _set_cell(size_t row, size_t col, const ::std::basic_string<wchar_t, T, A>& val){ _set_cell(row, col, convert_default_unicode.to_bytes(val)); }
    template<class T, class A> // 写入单元格 wstring&
    void _set_cell(size_t row, size_t col, ::std::basic_string<wchar_t, T, A>& val){ _set_cell(row, col, convert_default_unicode.to_bytes(val)); }
    template<class T, class A> // 写入单元格 wstring&&
    void _set_cell(size_t row, size_t col, ::std::basic_string<wchar_t, T, A>&& val){ _set_cell(row, col, convert_default_unicode.to_bytes(::std::move(val))); }
    template<class T> // 写入单元格 auto
    void _set_cell(size_t row, size_t col, T&& val)
    {
        ::std::stringstream ss;
        ss << ::std::forward<T>(val);
        _set_cell(row, col, ss.str());
    }
    void _set_cell(size_t row, size_t col, skip_cell_t&){}
    void _set_cell(size_t row, size_t col, const skip_cell_t&){}
    void _set_cell(size_t row, size_t col, skip_cell_t&&){}

    // 读取单元格 string&
    template<class T, class A> void _get_cell(size_t row, size_t col, ::std::basic_string<char, T, A>& val)
    {
        val.clear();
        if (m_data.size() < row)
            return;
        auto& data_line = m_data.at(row);
        if (data_line.size() < col)
            return;
        val = data_line.at(col);
    }
    // 读取单元格 wstring&
    template<class T, class A> void _get_cell(size_t row, size_t col, ::std::basic_string<wchar_t, T, A>& val)
    {
        ::std::string val_raw;
        _get_cell(row, col, val_raw);
        val = convert_default_unicode.from_bytes(::std::move(val_raw));
    }
    // 读取单元格 auto
    template<class T> void _get_cell(size_t row, size_t col, T& val)
    {
        ::std::string val_raw;
        _get_cell(row, col, val_raw);
        ::std::stringstream ss(::std::move(val_raw));
        ss >> val;
    }
    void _get_cell(size_t row, size_t col, skip_cell_t&){}
    void _get_cell(size_t row, size_t col, const skip_cell_t&){}
    void _get_cell(size_t row, size_t col, skip_cell_t&&){}
    template<class T> void _get_cell(size_t row, size_t col, T&&)
    {
        static_assert(false, "T must be a l-value reference.");
    }
    template<class T> void _get_cell(size_t row, size_t col, const T&)
    {
        static_assert(false, "T must not be a const type");
    }

    void _set_row(size_t row, size_t col){}
    // 写入一行
    template<class Arg, class... Args> void _set_row(size_t row, size_t col, Arg&& arg, Args&&... args)
    {
        _set_cell(row, col, ::std::forward<Arg>(arg));
        _set_row(row, col + 1, ::std::forward<Args>(args)...);
    }

    void _set_col(size_t row, size_t col){}
    // 写入一列
    template<class Arg, class... Args> void _set_col(size_t row, size_t col, Arg&& arg, Args&&... args)
    {
        _set_cell(row, col, ::std::forward<Arg>(arg));
        _set_col(row + 1, col, ::std::forward<Args>(args)...);
    }

    void _get_row(size_t row, size_t col){}
    // 读取一行
    template<class Arg, class... Args> void _get_row(size_t row, size_t col, Arg& arg, Args&... args)
    {
        _get_cell(row, col, arg);
        _get_row(row, col + 1, args...);
    }

    void _get_col(size_t row, size_t col){}
    // 读取一列
    template<class Arg, class... Args> void _get_col(size_t row, size_t col, Arg& arg, Args&... args)
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
    csvstream() = default;
    csvstream(const csvstream&) = delete;
    csvstream& operator=(const csvstream&) = delete;

    SYSCONAPI ::std::unique_lock<decltype(m_lock)> align_bound();

    // 读取CSV文件
    template<class T> bool read(T&& filename){ return _read(_open(::std::forward<T>(filename), ::std::ios::in)); }
    // 写入CSV文件
    template<class T> bool write(T&& filename){ return _write(_open(::std::forward<T>(filename), ::std::ios::out | ::std::ios::trunc)); }

    // 写入单元格
    template<class T> void set_cell(size_t row, size_t col, T&& val)
    {
        ::std::lock_guard<spin_mutex> lck(m_lock);
        _set_cell(row, col, ::std::forward<T>(val));
    }
    // 读取单元格
    template<class T> void get_cell(size_t row, size_t col, T&& val)
    {
        ::std::lock_guard<spin_mutex> lck(m_lock);
        _get_cell(row, col, ::std::forward<T>(val));
    }

    // 写入一行
    template<class... Args> void set_row(size_t row, Args&&... args)
    {
        ::std::lock_guard<spin_mutex> lck(m_lock);
        _set_row(row, 0, ::std::forward<Args>(args)...);
    }
    // 写入一列
    template<class... Args> void set_col(size_t col, Args&&... args)
    {
        ::std::lock_guard<spin_mutex> lck(m_lock);
        _set_col(0, col, ::std::forward<Args>(args)...);
    }

    // 从begin_col列开始写入一行
    template<class... Args> void set_row_begin(size_t row, size_t begin_col, Args&&... args)
    {
        ::std::lock_guard<spin_mutex> lck(m_lock);
        _set_row(row, begin_col, ::std::forward<Args>(args)...);
    }
    // 从begin_row行开始写入一列
    template<class... Args> void set_col_begin(size_t col, size_t begin_row, Args&&... args)
    {
        ::std::lock_guard<spin_mutex> lck(m_lock);
        _set_col(begin_row, col, ::std::forward<Args>(args)...);
    }

    // 读取一行
    template<class... Args> void get_row(size_t row, Args&&... args)
    {
        ::std::lock_guard<spin_mutex> lck(m_lock);
        _get_row(row, 0, ::std::forward<Args>(args)...);
    }
    // 读取一列
    template<class... Args> void get_col(size_t col, Args&&... args)
    {
        ::std::lock_guard<spin_mutex> lck(m_lock);
        _get_col(0, col, ::std::forward<Args>(args)...);
    }

    // 从begin_col列开始读取一行
    template<class... Args> void get_row_begin(size_t row, size_t begin_col, Args&&... args)
    {
        ::std::lock_guard<spin_mutex> lck(m_lock);
        _get_row(row, begin_col, ::std::forward<Args>(args)...);
    }
    // 从begin_row行开始读取一列
    template<class... Args> void get_col_begin(size_t col, size_t begin_row, Args&&... args)
    {
        ::std::lock_guard<spin_mutex> lck(m_lock);
        _get_col(begin_row, col, ::std::forward<Args>(args)...);
    }

    // 清除整个数据表
    void clear()
    {
        ::std::lock_guard<spin_mutex> lck(m_lock);
        m_data.clear();
    }
    // 清除一行
    void clear_row(size_t row)
    {
        ::std::lock_guard<spin_mutex> lck(m_lock);
        if (m_data.size() >= row + 1)
            m_data.at(row).clear();
    }
    // 清除一列
    void clear_col(size_t col)
    {
        ::std::lock_guard<spin_mutex> lck(m_lock);
        for (auto& line_data : m_data)
            if (line_data.size() >= col + 1)
                line_data.at(col).clear();
    }

    // 删除一行，下边的行上移
    void erase_row(size_t row)
    {
        ::std::lock_guard<spin_mutex> lck(m_lock);
        if (m_data.size() >= row + 1)
            m_data.erase(m_data.cbegin() + row);
    }
    // 删除一列，右边的列左移
    void erase_col(size_t col)
    {
        ::std::lock_guard<spin_mutex> lck(m_lock);
        for (auto& line_data : m_data)
            if (line_data.size() >= col + 1)
                line_data.erase(line_data.cbegin() + col);
    }

    // 在row行上边插入一行
    template<class... Args> void insert_row(size_t row, Args&&... args)
    {
        ::std::lock_guard<spin_mutex> lck(m_lock);
        // 如果插入的行已存在
        if (m_data.size() >= row + 1)
            m_data.insert(m_data.cbegin() + row, ::std::vector<::std::string>());
        _set_row(row, 0, ::std::forward<Args>(args)...);
    }
    // 在col列左边插入一列
    template<class... Args> void insert_col(size_t col, Args&&... args)
    {
        ::std::lock_guard<spin_mutex> lck(m_lock);
        for (auto& line_data : m_data)
            if (line_data.size() >= col + 1) // 如果插入的列已存在
                line_data.insert(line_data.cbegin() + col, ::std::string());
        _set_col(col, 0, ::std::forward<Args>(args)...);
    }

    // 在row行上边插入一行，写入数据从begin_col列开始
    template<class... Args> void insert_row_begin(size_t row, size_t begin_col, Args&&... args)
    {
        ::std::lock_guard<spin_mutex> lck(m_lock);
        // 如果插入的行已存在
        if (m_data.size() >= row + 1)
            m_data.insert(m_data.cbegin() + row, ::std::vector<::std::string>(begin_col));
        _set_row(row, begin_col, ::std::forward<Args>(args)...);
    }
    // 在col列左边插入一列，写入数据从begin_row行开始
    template<class... Args> void insert_col_begin(size_t col, size_t begin_row, Args&&... args)
    {
        ::std::lock_guard<spin_mutex> lck(m_lock);
        for (auto& line_data : m_data)
            if (line_data.size() >= col + 1) // 如果插入的列已存在
                line_data.insert(line_data.cbegin() + col, ::std::string());
        _set_col(col, begin_row, ::std::forward<Args>(args)...);
    }

    SYSCONAPI static skip_cell_t skip_cell;
};
