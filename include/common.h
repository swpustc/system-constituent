/**********************************************************
* STL�㷨��չ
* ֧��ƽ̨��Windows; Linux
* ���뻷����VS2013+; g++ -std=c++11
* ����ʱ�䣺2015-04-05 ����������
* ����޸ģ�2015-05-02 ����������
***********************************************************/

#pragma once

#include <ctime>
#include <mutex>
#include <chrono>
#include <cstdio>
#include <memory>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <type_traits>

#if defined(_WIN32) || defined(WIN32)
#include <tchar.h>
#include <crtdefs.h>
#include <Windows.h>
#else // Linux
#include <unistd.h>
#include <sys/time.h>
#endif // #if defined(_WIN32) || defined(WIN32)


#define EXTERN_C  extern "C"

#ifndef _CRT_STRINGIZE
#define __CRT_STRINGIZE(_Value) #_Value
#define _CRT_STRINGIZE(_Value) __CRT_STRINGIZE(_Value)
#endif  /* _CRT_STRINGIZE */

#ifndef _CRT_WIDE
#define __CRT_WIDE(_String) L ## _String
#define _CRT_WIDE(_String) __CRT_WIDE(_String)
#endif  /* _CRT_WIDE */

#ifndef _CRT_APPEND
#define __CRT_APPEND(_Value1, _Value2) _Value1 ## _Value2
#define _CRT_APPEND(_Value1, _Value2) __CRT_APPEND(_Value1, _Value2)
#endif  /* _CRT_APPEND */

#ifndef _T
#ifdef _UNICODE
#define __TXT(x)    L ## x
#else  /* _UNICODE */
#define __TXT(x)    x
#endif  /* _UNICODE */
#define _T(x)       __TXT(x)
#endif  /* _T */

#ifdef _WIN32
#  ifdef SYSCON_EXPORT
#    define SYSCONAPI           extern __declspec(dllexport)
#  else
#    define SYSCONAPI           extern __declspec(dllimport)
#  endif
#endif
#ifndef SYSCONAPI
#  define SYSCONAPI             extern
#endif


#if defined(_WIN32) || defined(WIN32)
struct timezone
{
    int tz_minuteswest;
    int tz_dsttime;
};

inline int gettimeofday(struct timeval* tv, struct timezone* tz)
{
    if (tv)
    {
        union filetime_t
        {
            FILETIME ft;
            DWORD64  ut;
        } filetime;
        DWORD64 time_now;
        static const DWORD64 EPOCH_BIAS = 116444736000000000; /*1970/01/01*/
        ::GetSystemTimeAsFileTime(&filetime.ft);
        time_now = filetime.ut - EPOCH_BIAS;
        tv->tv_sec = (long)(time_now / 10000000ULL);
        tv->tv_usec = (long)((time_now % 10000000ULL) / 10ULL);
    }
    if (tz)
    {
        TIME_ZONE_INFORMATION TimeZoneInformation;
        ::GetTimeZoneInformation(&TimeZoneInformation);
        tz->tz_dsttime = (int)TimeZoneInformation.DaylightBias;
        tz->tz_minuteswest = (int)TimeZoneInformation.Bias;
    }
    return 0;
}

#else // UNIX
#define OutputDebugStringW(...)     void()
#define OutputDebugStringA(...)     void()
#define OutputDebugString(...)      void()

#endif // #if defined(_WIN32) || defined(WIN32)

template<class T> inline auto auto_max(T&& t) -> decltype(::std::forward<T>(t))
{
    return ::std::forward<T>(t);
}

template<class T1, class T2, class... Args> inline
auto auto_max(T1&& t1, T2&& t2, Args&&... args)
    -> decltype(::std::forward<T1>(t1) > ::std::forward<T2>(t2) ? ::std::forward<T1>(t1) : ::std::forward<T2>(t2))
{
    return auto_max(::std::forward<T1>(t1) > ::std::forward<T2>(t2) ?
        ::std::forward<T1>(t1) : ::std::forward<T2>(t2), ::std::forward<Args>(args)...);
}


template<class T> inline auto auto_min(T&& t) -> decltype(::std::forward<T>(t))
{
    return ::std::forward<T>(t);
}

template<class T1, class T2, class... Args> inline
auto auto_min(T1&& t1, T2&& t2, Args&&... args)
-> decltype(::std::forward<T1>(t1) < ::std::forward<T2>(t2) ? ::std::forward<T1>(t1) : ::std::forward<T2>(t2))
{
    return auto_min(::std::forward<T1>(t1) < ::std::forward<T2>(t2) ?
        ::std::forward<T1>(t1) : ::std::forward<T2>(t2), ::std::forward<Args>(args)...);
}


template<class T> inline
typename ::std::decay<T>::type decay_type(T&& arg)
{
    return ::std::forward<T>(arg);
}


struct function_wapper
{
    template<class Fn, class... Args> inline void operator()(Fn&& fn, Args&&... args)
    {
        decay_type(::std::forward<Fn>(fn))(decay_type(::std::forward<Args>(args))...);
    }
    template<class Fn, class... Args> inline void operator()(::std::shared_ptr<Fn>& fn, Args&&... args)
    {
        (*fn.get())(decay_type(::std::forward<Args>(args))...);
    }
    template<class Fn, class... Args> inline void operator()(::std::unique_ptr<Fn>& fn, Args&&... args)
    {
        (*fn.get())(decay_type(::std::forward<Args>(args))...);
    }
};


class log_stream_manager_t
{
private:
    ::std::streambuf* m_clog_rdbuf;
    ::std::wstreambuf* m_wclog_rdbuf;
public:
    ::std::mutex m_log_lock;
    ::std::unique_ptr<::std::ofstream> m_log_ofstream;
    ::std::unique_ptr<::std::wofstream> m_log_wofstream;
    log_stream_manager_t();
    ~log_stream_manager_t();
};
SYSCONAPI log_stream_manager_t g_log_stream_manager;

#define g_log_lock      (g_log_stream_manager.m_log_lock)
#define g_log_ofstream  (*g_log_stream_manager.m_log_ofstream)
#define g_log_wofstream (*g_log_stream_manager.m_log_wofstream)


#ifdef _UNICODE
#define _tclog          ::std::wclog
#else  /* _UNICODE */
#define _tclog          ::std::clog
#endif  /* _UNICODE */


template<class T, class Arg> inline void debug_put(::std::basic_ostream<char, T>& os, Arg&& arg)
{
    ::std::stringstream ss;
    ss << ::std::forward<Arg>(arg);
    os << ss.str();
    os.flush();
#if defined(_DEBUG) || defined(DEBUG)
    ::OutputDebugStringA(ss.str().c_str());
#endif
}

template<class T, class Arg> inline void debug_put(::std::basic_ostream<wchar_t, T>& os, Arg&& arg)
{
    ::std::wstringstream ss;
    ss << ::std::forward<Arg>(arg);
    os << ss.str();
    os.flush();
#if defined(_DEBUG) || defined(DEBUG)
    ::OutputDebugStringW(ss.str().c_str());
#endif
}

inline void _debug_output()
{
    auto now = ::std::chrono::system_clock::to_time_t(::std::chrono::system_clock::now());
    debug_put(_tclog, _T(" [TID:"));
    debug_put(_tclog,
#if defined(_WIN32) || defined(WIN32)
        ::GetCurrentThreadId()
#else // Linux
        ::gettid()
#endif // #if defined(_WIN32) || defined(WIN32)
        );
    debug_put(_tclog, _T("] "));
#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4996 )
    debug_put(_tclog, ::_tctime(&now));
#pragma warning( pop )
#else // _MSC_VER
    debug_put(::std::clog, ::std::ctime(&now));
#endif // #ifdef _MSC_VER
}

template<class T, class... Args> inline void _debug_output(T&& arg, Args&&... args)
{
    debug_put(_tclog, arg);
    _debug_output(::std::forward<Args>(args)...);
}

template<class... Args> inline void _debug_output(const char* debug_string, Args&&... args)
{
    debug_put(::std::clog, debug_string);
    _debug_output(::std::forward<Args>(args)...);
}

template<class... Args> inline void _debug_output(volatile char* debug_string, Args&&... args)
{
    debug_put(::std::clog, debug_string);
    _debug_output(::std::forward<Args>(args)...);
}

template<class... Args> inline void _debug_output(const volatile char* debug_string, Args&&... args)
{
    debug_put(::std::clog, debug_string);
    _debug_output(::std::forward<Args>(args)...);
}

template<class T, class A, class... Args> inline void _debug_output(const ::std::basic_string<char, T, A>& debug_string, Args&&... args)
{
    debug_put(::std::clog, debug_string);
    _debug_output(::std::forward<Args>(args)...);
}

template<class... Args> inline void _debug_output(const wchar_t* debug_string, Args&&... args)
{
    debug_put(::std::wclog, debug_string);
    _debug_output(::std::forward<Args>(args)...);
}

template<class... Args> inline void _debug_output(volatile wchar_t* debug_string, Args&&... args)
{
    debug_put(::std::wclog, debug_string);
    _debug_output(::std::forward<Args>(args)...);
}

template<class... Args> inline void _debug_output(const volatile wchar_t* debug_string, Args&&... args)
{
    debug_put(::std::wclog, debug_string);
    _debug_output(::std::forward<Args>(args)...);
}

template<class T, class A, class... Args> inline void _debug_output(const ::std::basic_string<wchar_t, T, A>& debug_string, Args&&... args)
{
    debug_put(::std::wclog, debug_string);
    _debug_output(::std::forward<Args>(args)...);
}

#if defined(_DEBUG) || defined(DEBUG)
// Debug�£�����outputΪ��ֵ���������Release�£�outputΪtrueʱ���
template<bool output = false, class... Args> inline void debug_output(Args&&... args)
{
    ::std::lock_guard<::std::mutex> lck(g_log_lock);
    _debug_output(decay_type(::std::forward<Args>(args))...);
}

#else // NDEBUG
// Debug�£�����outputΪ��ֵ���������Release�£�outputΪtrueʱ����������ʹ��debug_output(...)
template<bool output = false, class... Args> inline void debug_output(Args&&... args)
{
    if (output)
    {
        ::std::lock_guard<::std::mutex> lck(g_log_lock);
        _debug_output(decay_type(::std::forward<Args>(args))...);
    }
}

// Debug�£�����outputΪ��ֵ���������Release�£�Ĭ�ϲ���������ʹ��debug_output<true>(...)
#define debug_output(...)   void()

#endif //#if defined(_DEBUG) || defined(DEBUG)


// ����log�����ļ�λ��
template<class Elem, class T = ::std::char_traits<Elem>, class A = ::std::allocator<Elem>> inline
void set_log_location(::std::basic_string<Elem, T, A> file_name)
{
    set_log_location(file_name.c_str());
}

// ����log�����ļ�λ��
template<class Elem> inline void set_log_location(const Elem* file_name)
{
    g_log_ofstream.open(file_name, ::std::ios::app);
    g_log_wofstream.open(file_name, ::std::ios::app);
    auto rdbuf = g_log_ofstream.rdbuf();
    if (rdbuf)
        ::std::clog.rdbuf(rdbuf);
    auto wrdbuf = g_log_wofstream.rdbuf();
    if (wrdbuf)
        ::std::wclog.rdbuf(wrdbuf);
    debug_output<true>(_T("Process Start: [PID:"),
#if defined(_WIN32) || defined(WIN32)
        ::GetCurrentProcessId()
#else // Linux
        ::getpid()
#endif // #if defined(_WIN32) || defined(WIN32)
        , _T(']'));
}
