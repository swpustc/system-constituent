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

public:
    csvstream(){}
    csvstream(const char* filename){ open(filename); }
    csvstream(const ::std::string& filename){ open(filename); }
#ifdef _WIN32
    csvstream(const wchar_t* filename){ open(filename); }
    csvstream(const ::std::wstring& filename){ open(filename); }
#endif  /* _WIN32 */

    void open(const char* filename){ m_io.open(filename, ::std::ios::app | ::std::ios::in | ::std::ios::out | ::std::ios::ate); }
    void open(const ::std::string& filename){ m_io.open(filename, ::std::ios::app | ::std::ios::in | ::std::ios::out | ::std::ios::ate); }
#ifdef _WIN32
    void open(const wchar_t* filename){ m_io.open(filename, ::std::ios::app | ::std::ios::in | ::std::ios::out | ::std::ios::ate); }
    void open(const ::std::wstring& filename){ m_io.open(filename, ::std::ios::app | ::std::ios::in | ::std::ios::out | ::std::ios::ate); }
#endif  /* _WIN32 */

    // 文件是否打开
    bool is_open(){ return m_io.is_open(); }
    // 读取CSV文件
    SYSCONAPI bool read();
    // 写入CSV文件
    SYSCONAPI bool write();
};
