/**********************************************************
* ��ȫ�ľ�������������װ
* ֧��ƽ̨��Windows
* ���뻷����VS2010+
* ����ʱ�䣺2015-04-05 ����������
* ����޸ģ�2015-05-01 ����������
***********************************************************/

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
            ::CloseHandle(m_handle);
            m_handle = INVALID_HANDLE_VALUE;
        }
    }

public:
    // Ĭ�Ϲ��캯��
    SAFE_HANDLE_OBJECT() : m_handle(INVALID_HANDLE_VALUE)
    {
    }
    // Ĭ����������
    ~SAFE_HANDLE_OBJECT()
    {
        close_handle();
    }

    // ���ƹ��캯��
    SAFE_HANDLE_OBJECT(const SAFE_HANDLE_OBJECT&) = delete;
    // ���Ƹ�ֵ���
    SAFE_HANDLE_OBJECT& operator=(const SAFE_HANDLE_OBJECT&) = delete;

    // �ƶ����캯��
    SAFE_HANDLE_OBJECT(SAFE_HANDLE_OBJECT&& other) : m_handle(other.m_handle)
    {
        other.m_handle = INVALID_HANDLE_VALUE;
    }
    // �ƶ���ֵ���
    SAFE_HANDLE_OBJECT& operator=(SAFE_HANDLE_OBJECT&& other)
    {
        ::std::swap(m_handle, other.m_handle);
        return *this;
    }

    SAFE_HANDLE_OBJECT(HANDLE handle) : m_handle(handle)
    {
    }

    // �رվ��������¾��
    SAFE_HANDLE_OBJECT& operator=(HANDLE handle)
    {
        attach(handle);
        return *this;
    }
    void attach(HANDLE handle)
    {
        close_handle();
        m_handle = handle;
    }

    // ������
    HANDLE detach()
    {
        HANDLE detach_handle = m_handle;
        m_handle = INVALID_HANDLE_VALUE;
        return detach_handle;
    }

    // ��ȡ���
    operator HANDLE() const
    {
        return get_safe_handle();
    }
    HANDLE get_safe_handle() const
    {
        return this ? (m_handle ? m_handle : INVALID_HANDLE_VALUE) : INVALID_HANDLE_VALUE;
    }
};
