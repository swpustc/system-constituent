/**********************************************************
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
static const regex g_csv_regex(R"___(^(?:"((?:[^"]|"")*)"|([^",]*)),)___");
// CSV 替换字符串
static const string g_csv_replace_from(R"_("")_");
static const string g_csv_replace_to(R"_(")_");
static const string g_csv_replace_first_of(R"_(",)_");

// 跳过变量标识
csvstream::cell_skip_t csvstream::cell_skip;

bool csvstream::_read(fstream&& svcstream)
{
    if (!svcstream.is_open())
        return false;
    string line;
    // 捕获结果match_results
    cmatch match_result;
    // IO读写锁
    lock_guard<spin_mutex> lck(m_lock);
    // 清空数据
    m_data.clear();
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
        m_data.push_back(move(data_line));
    }
    return true;
}

bool csvstream::_write(fstream&& svcstream)
{
    if (!svcstream.is_open())
        return false;
    size_t row_number = 0;
    // IO读写锁
    lock_guard<spin_mutex> lck(m_lock);
    // 获取最大是列宽
    for (auto& line_data : m_data)
    {
        size_t line_size = line_data.size();
        if (line_size > row_number)
        {   // 最后非空的单元格
            size_t line_not_empty = line_size - 1;
            // 对于超出最大列宽的部分，验证是否为空
            for (; line_not_empty >= row_number; line_not_empty--)
            {
                if (!line_data.at(line_not_empty).empty())
                    break;
            }
            row_number = auto_max(row_number, line_not_empty + 1);
        }
    }
    // 每一行数据
    for (auto& line_data : m_data)
    {
        line_data.resize(row_number);
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
        svcstream << line_string;
    }
    return true;
}

void csvstream::_set_cell(size_t row, size_t col, const string& val)
{
    // IO读写锁
    lock_guard<spin_mutex> lck(m_lock);
    if (val.empty())
    {    /* val为空，清空已有单元格 */
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

void csvstream::_set_cell(size_t row, size_t col, string&& val)
{
    // IO读写锁
    lock_guard<spin_mutex> lck(m_lock);
    if (val.empty())
    {    /* val为空，清空已有单元格 */
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
