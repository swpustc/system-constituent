/**********************************************************
* CSV逗号分隔文件读写类
* 支持平台：Windows; Linux
* 编译环境：VS2013+; g++ -std=c++11
***********************************************************/

#include "csvstream.h"
#include <cassert>
#include <algorithm>
#include <functional>

using namespace std;

// CSV 捕获正则表达式（原始字符串）
static const regex g_csv_regex(R"___(^(?:"((?:[^"]|"")*)"|([^",]*)),)___");
// CSV 替换字符串
static const string g_csv_replace_from(R"_("")_");
static const string g_csv_replace_to(R"_(")_");
static const string g_csv_replace_first_of(R"_(",)_");

// 跳过变量标识
csvstream::skip_cell_t csvstream::skip_cell;
csvstream::sync_set_t csvstream::sync_set;
csvstream::sync_get_t csvstream::sync_get;

unique_lock<decltype(csvstream::m_lock)> csvstream::align_bound()
{
    // 最优列宽
    size_t col_number = 0;
    unique_lock<decltype(m_lock)> lck(m_lock);
    // 获取最大是列宽
    for (auto& line_data : m_data)
    {
        size_t line_size = line_data.size();
        if (line_size > col_number)
        {   // 最后非空的单元格
            size_t line_not_empty = line_size - 1;
            // 对于超出最大列宽的部分，验证是否为空
            for (; line_not_empty >= col_number; line_not_empty--)
            {
                if (!line_data.at(line_not_empty).empty())
                    break;
            }
            col_number = auto_max(col_number, line_not_empty + 1);
        }
    }
    // 结尾空白行数
    size_t row_number = 0;
    // 是否结尾连续空白行
    bool row_end = true;
    for_each(rbegin(m_data), rend(m_data), [&](vector<string>& line_data)
    {
        size_t line_size = line_data.size();
        // 如果列超出最优列宽，缩减到最优列宽
        if (line_size > col_number)
            line_data.resize(col_number);
        // 如果是结尾连续的空白行
        if (row_end)
        {   // 所有单元格均为空
            if (all_of(begin(line_data), end(line_data), mem_fn(&string::empty)))
            {
                row_number++;
                return; // 需要删除的行不更新列宽
            }
            else
                row_end = false;
        }
        // 如果列不足最优列宽，增加到最优列宽
        if (line_size < col_number)
            line_data.resize(col_number);
    });
    if (row_number)
    {   // 总行数
        size_t row_size = m_data.size();
        assert(row_number <= row_size);
        m_data.resize(row_size - row_number);
    }
    return move(lck);
}

bool csvstream::_read(fstream&& svcstream)
{
    if (!svcstream.is_open())
        return false;
    string line;
    // 捕获结果match_results
    cmatch match_result;
    // 读入的数据
    decltype(m_data) data;
    // 读取一行
    while (getline(svcstream, line))
    {
        // 单行数据
        vector<string> data_line;
        // 验证getline是否输出了换行符
        assert(*line.rbegin() != '\n');
        line.push_back(',');
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
        data.push_back(move(data_line));
    }
    lock_guard<decltype(m_lock)> lck(m_lock);
    // 写入数据
    m_data = move(data);
    return true;
}

bool csvstream::_write(fstream&& svcstream)
{
    if (!svcstream.is_open())
        return false;
    // 输出数据流
    stringstream ss;
    auto&& lck = align_bound();
    // 每一行数据
    for (auto& line_data : m_data)
    {
        string line_string;
        for (auto line : line_data)
        {
            size_t pos;
            if ((pos = line.find_first_of(g_csv_replace_first_of)) != string::npos)
            {
                pos = line.find(g_csv_replace_to, pos);
                while (pos != string::npos)
                {
                    line.replace(pos, g_csv_replace_to.size(), g_csv_replace_from);
                    pos = line.find(g_csv_replace_to, pos + g_csv_replace_from.size());
                }
                line = '\"' + move(line) + '\"';
            }
            line_string += move(line) + ',';
        }
        if (line_string.size())
            *line_string.rbegin() = '\n';
        ss << line_string;
    }
    lck.unlock();
    // 写入文件
    svcstream << ss.str();
    return true;
}

void csvstream::_set_cell(size_t row, size_t col, const string& val)
{
    if (val.empty()) /* val为空，清空已有单元格 */
    {
        if (m_data.size() < row + 1)
            return;
        auto& data_line = m_data.at(row);
        if (data_line.size() < col + 1)
            return;
        auto& cell = data_line.at(col);
        cell = val;
    }
    else /* val非空，设置新单元格 */
    {
        if (m_data.size() < row + 1)
            m_data.resize(row + 1);
        auto& data_line = m_data.at(row);
        if (data_line.size() < col + 1)
            data_line.resize(col + 1);
        auto& cell = data_line.at(col);
        cell = val;
    }
}

void csvstream::_set_cell(size_t row, size_t col, string&& val)
{
    if (val.empty()) /* val为空，清空已有单元格 */
    {
        if (m_data.size() < row + 1)
            return;
        auto& data_line = m_data.at(row);
        if (data_line.size() < col + 1)
            return;
        auto& cell = data_line.at(col);
        cell = move(val);
    }
    else /* val非空，设置新单元格 */
    {
        if (m_data.size() < row + 1)
            m_data.resize(row + 1);
        auto& data_line = m_data.at(row);
        if (data_line.size() < col + 1)
            data_line.resize(col + 1);
        auto& cell = data_line.at(col);
        cell = move(val);
    }
}


void csvstream::swap_row(size_t row1, size_t row2)
{
    ::std::lock_guard<decltype(m_lock)> lck(m_lock);
    size_t row_number = m_data.size();
    if (row_number >= row1 + 1)
    {
        if (row_number >= row2 + 1)
            ::std::swap(m_data.at(row1), m_data.at(row2));
        else /* row_number < row2 + 1 */
        {
            m_data.resize(row2 + 1);
            ::std::swap(m_data.at(row1), *rbegin(m_data));
        }
    }
    else /* row_number < row1 + 1 */
    {
        if (row_number >= row2 + 1)
        {
            m_data.resize(row1 + 1);
            ::std::swap(*rbegin(m_data), m_data.at(row2));
        } /* row_number < row2 + 1 */
    } /* 两行均不存在，无需交换动作 */
}

void csvstream::swap_col(size_t col1, size_t col2)
{
    ::std::lock_guard<decltype(m_lock)> lck(m_lock);
    for (auto& line_data : m_data)
    {
        size_t col_number = line_data.size();
        if (col_number >= col1 + 1)
        {
            if (col_number >= col2 + 1)
                ::std::swap(line_data.at(col1), line_data.at(col2));
            else /* col_number < col2 + 1 */
            {
                line_data.resize(col2 + 1);
                ::std::swap(line_data.at(col1), *rbegin(line_data));
            }
        }
        else /* col_number < col1 + 1 */
        {
            if (col_number >= col2 + 1)
            {
                line_data.resize(col1 + 1);
                ::std::swap(*rbegin(line_data), line_data.at(col2));
            } /* col_number < col2 + 1 */
        } /* 两列均不存在，无需交换动作 */
    }
}
