/**********************************************************
 * 安全的句柄、对象操作封装
 * 支持平台：Windows
 * 编译环境：VS2010+
 * 创建时间：2015-04-05 （宋万鹏）
 * 最后修改：2015-04-05 （宋万鹏）
 **********************************************************/

#ifndef __SAFE_OBJECT_H__
#define __SAFE_OBJECT_H__
#pragma once

#include <utility>
#include <Windows.h>


class SAFE_HANDLE_OBJECT
{
private:
    HANDLE m_handle;

    void close_handle()
    {
        if (m_handle && m_handle != INVALID_HANDLE_VALUE)
        {
            CloseHandle(m_handle);
            m_handle = INVALID_HANDLE_VALUE;
        }
    }

public:
    // 默认构造函数
    SAFE_HANDLE_OBJECT()
        : m_handle(INVALID_HANDLE_VALUE)
    {
    }
    // 默认析构函数
    ~SAFE_HANDLE_OBJECT()
    {
        close_handle();
    }

    // 复制构造函数
    SAFE_HANDLE_OBJECT(const SAFE_HANDLE_OBJECT&) = delete;
    // 复制赋值语句
    SAFE_HANDLE_OBJECT& operator=(const SAFE_HANDLE_OBJECT&) = delete;

    // 移动构造函数
    SAFE_HANDLE_OBJECT(SAFE_HANDLE_OBJECT&& _Other)
        : m_handle(_Other.m_handle)
    {
        _Other.m_handle = INVALID_HANDLE_VALUE;
    }
    // 移动赋值语句
    SAFE_HANDLE_OBJECT& operator=(SAFE_HANDLE_OBJECT&& _Other)
    {
        ::std::swap(m_handle, _Other.m_handle);
        return *this;
    }

    SAFE_HANDLE_OBJECT(HANDLE handle)
        : m_handle(handle)
    {
    }

    // 关闭句柄并添加新句柄
    SAFE_HANDLE_OBJECT& operator=(HANDLE handle)
    {
        Attach(handle);
        return *this;
    }
    void Attach(HANDLE handle)
    {
        close_handle();
        m_handle = handle;
    }

    // 分离句柄
    HANDLE Detach()
    {
        HANDLE detach_handle = m_handle;
        m_handle = INVALID_HANDLE_VALUE;
        return detach_handle;
    }
    operator HANDLE()
    {
        return m_handle;
    }
};


#endif // #ifndef __SAFE_OBJECT_H__
