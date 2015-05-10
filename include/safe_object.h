/**********************************************************
* 安全的句柄、对象操作封装
* 支持平台：Windows
* 编译环境：VS2010+
* 创建时间：2015-04-05 （宋万鹏）
* 最后修改：2015-05-02 （宋万鹏）
***********************************************************/

#pragma once

#include <utility>
#include <Windows.h>


class SAFE_HANDLE_OBJECT
{
private:
    HANDLE m_handle;

public:
    // 默认构造函数
    SAFE_HANDLE_OBJECT() : m_handle(INVALID_HANDLE_VALUE)
    {
    }
    // 默认析构函数
    ~SAFE_HANDLE_OBJECT()
    {
        close();
    }

    // 复制构造函数
    SAFE_HANDLE_OBJECT(const SAFE_HANDLE_OBJECT&) = delete;
    // 复制赋值语句
    SAFE_HANDLE_OBJECT& operator=(const SAFE_HANDLE_OBJECT&) = delete;

    // 移动构造函数
    SAFE_HANDLE_OBJECT(SAFE_HANDLE_OBJECT&& other) : m_handle(other.m_handle)
    {
        other.m_handle = INVALID_HANDLE_VALUE;
    }
    // 移动赋值语句
    SAFE_HANDLE_OBJECT& operator=(SAFE_HANDLE_OBJECT&& other)
    {
        ::std::swap(m_handle, other.m_handle);
        return *this;
    }

    SAFE_HANDLE_OBJECT(HANDLE handle) : m_handle(handle)
    {
    }

    // 关闭句柄
    void close()
    {
        if (!!m_handle && m_handle != INVALID_HANDLE_VALUE)
        {
            ::CloseHandle(m_handle);
            m_handle = INVALID_HANDLE_VALUE;
        }
    }
    // 是否存储有句柄
    bool is_open()
    {
        return !!m_handle && m_handle != INVALID_HANDLE_VALUE;
    }

    // 关闭句柄并添加新句柄
    SAFE_HANDLE_OBJECT& operator=(HANDLE handle)
    {
        attach(handle);
        return *this;
    }
    void attach(HANDLE handle)
    {
        close();
        m_handle = handle;
    }

    // 分离句柄
    HANDLE detach()
    {
        HANDLE detach_handle = m_handle;
        m_handle = INVALID_HANDLE_VALUE;
        return detach_handle;
    }

    // 获取句柄
    operator HANDLE() const
    {
        return get_safe_handle();
    }
    HANDLE get_safe_handle() const
    {
        return this ? (m_handle ? m_handle : INVALID_HANDLE_VALUE) : INVALID_HANDLE_VALUE;
    }
};
