﻿/**********************************************************
* CSV逗号分隔文件读写类
* 支持平台：Windows; Linux
* 编译环境：VS2013+; g++ -std=c++11
* 创建时间：2015-05-07 （宋万鹏）
* 最后修改：2015-05-07 （宋万鹏）
***********************************************************/

#include "csvstream.h"
#include <cassert>

using namespace std;

// CSV 捕获正则表达式（原始字符串）
static const regex g_csv_regex(R"___(^(?:"((?:[^"]|"")*)"|([^",]*))(?:,|$))___");
// CSV 替换字符串
static const string g_csv_replace_from(R"_("")_");
static const string g_csv_replace_to(R"_(")_");
static const string g_csv_replace_first_of(R"_(",)_");

bool csvstream::read()
{
    if (!is_open())
        return false;
    string line;
    // 捕获结果match_results
    cmatch match_result;
    // IO读写锁
    lock_guard<mutex> lck(m_lock);
    // 清空数据
    m_data.clear();
    // 设置读指针到开始
    m_io.seekg(0, ios::beg);
    // 读取一行
    while (getline(m_io, line))
    {
        // 单行数据
        vector<string> data_line;
        const char* first = line.c_str();
        while (*first && regex_search(first, match_result, g_csv_regex, regex_constants::format_no_copy))
        {
            // 捕获的表达式只可能是3个
            assert(match_result.size() == 3);
            // 表达式一定已捕获到
            assert(match_result[0].matched);
            if (match_result[1].matched)
            {   // 捕获的字符串
                string match(match_result[1].first, match_result[1].second);
                // 替换所有的
                auto pos = match.find(g_csv_replace_from, 0);
                while (pos != string::npos)
                {
                    match.replace(pos, g_csv_replace_from.size(), g_csv_replace_to);
                    pos = match.find(g_csv_replace_from, pos + g_csv_replace_to.size());
                }
                data_line.push_back(move(match));
            }
            else // if (match_result[2].matched)
            {   // 如果1未捕获，2一定捕获
                assert(match_result[2].matched);
                string match(match_result[2].first, match_result[2].second);
                // 一般来说，没有需要替换的字符
                assert(match.find(g_csv_replace_from, 0) == string::npos);
                data_line.push_back(move(match));
            }
            // 下一个替换字符串
            first = match_result.suffix().first;
        }
        m_data.push_back(move(data_line));
    }
    return true;
}

bool csvstream::write()
{
    if (!is_open())
        return false;
    // IO读写锁
    lock_guard<mutex> lck(m_lock);
    // 设置写指针到开始
    m_io.seekp(0, ios::beg);
    size_t row_number = 0;
    // 每一行数据
    for (auto& line_data : m_data)
    {   // 每一个元数据
        row_number = auto_max(row_number, line_data.size());
    }
    for (auto& line_data : m_data)
    {   // 验证元素个数少于最大列宽
        assert(line_data.size() <= row_number);
        line_data.resize(row_number);
        string line_string;
        for (auto& line : line_data)
            line_string += line + ',';
        if (line_string.size())
            *line_string.end() = '\n';
        m_io << line_string;
    }
    return true;
}
