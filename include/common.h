/**********************************************************
* STL算法扩展
* 支持平台：Windows; Linux
* 编译环境：VS2013+; g++ -std=c++11
* 创建时间：2015-04-05 （宋万鹏）
* 最后修改：2015-05-02 （宋万鹏）
***********************************************************/

#pragma once

#include <ctime>
#include <mutex>
#include <chrono>
#include <cstdio>
#include <memory>
#include <codecvt>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <cassert>
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


SYSCONAPI ::std::mutex g_log_lock;
SYSCONAPI ::std::ofstream g_log_ofstream;

#ifdef _MSC_VER
template<uint32_t codepage = CP_ACP, class Elem = wchar_t, class Walloc = ::std::allocator<Elem>, class Balloc = ::std::allocator<char>>
class convert_cp_unicode_t
{
    typedef ::std::basic_string<char, ::std::char_traits<char>, Balloc> byte_string;
    typedef ::std::basic_string<Elem, ::std::char_traits<Elem>, Walloc> wide_string;
    typedef ::mbstate_t state_type;
    void init()
    {
        static_assert(sizeof(wchar_t) == sizeof(Elem), "sizeof Elem must be 2");
        static state_type State0 = '?';
        State = State0;
        nconv = 0;
    }
public:
    convert_cp_unicode_t()
        : has_berr(false), has_werr(false), has_state(false)
    {
        init();
    }
    convert_cp_unicode_t(state_type State_arg)
        : has_berr(false), has_werr(false), has_state(true)
    {
        init();
        State = State_arg;
    }
    convert_cp_unicode_t(const byte_string& berr_arg)
        : has_berr(true), has_werr(false), has_state(false), berr(berr_arg)
    {
        init();
    }
    convert_cp_unicode_t(const byte_string& berr_arg, const wide_string& werr_arg)
        : has_berr(true), has_werr(false), has_state(false), berr(berr_arg), werr(werr_arg)
    {
        init();
    }
    ~convert_cp_unicode_t(){}
    size_t converted() const
    {
        return nconv;
    }
    state_type state() const
    {
        return State;
    }
    wide_string from_bytes(char _Byte)
    {
        return ::std::move(from_bytes(&_Byte, &_Byte + 1));
    }
    wide_string from_bytes(const char *ptr)
    {
        return ::std::move(from_bytes(ptr, ptr + ::strlen(ptr)));
    }
    wide_string from_bytes(const byte_string& bstr)
    {
        const char *ptr = bstr.c_str();
        return ::std::move(from_bytes(ptr, ptr + bstr.size()));
    }
    wide_string from_bytes(const char *first, const char *last)
    {
        size_t length, result;
        length = (size_t)::MultiByteToWideChar(codepage, 0, first, (int)(last - first), nullptr, 0);
        wide_string wstr;
        wstr.resize(length);
        result = (size_t)::MultiByteToWideChar(codepage, 0, first, (int)(last - first), (LPWSTR)const_cast<Elem*>(wstr.c_str()), (int)(length + 1));
        const Elem *wptr = wstr.c_str(), *next = wptr;
        for (; *next++;);
        wstr.resize(nconv = next - wptr - 1);
        assert(length >= nconv);
        return ::std::move(wstr);
    }
    byte_string to_bytes(Elem _Char)
    {
        return ::std::move(to_bytes(&_Char, &_Char + 1));
    }
    byte_string to_bytes(const Elem *wptr)
    {
        const Elem *next = wptr;
        for (; *next++;);
        return ::std::move(to_bytes(wptr, next));
    }
    byte_string to_bytes(const wide_string& wstr)
    {
        const Elem *wptr = wstr.c_str();
        return ::std::move(to_bytes(wptr, wptr + wstr.size()));
    }
    byte_string to_bytes(const Elem *first, const Elem *last)
    {
        size_t length, result;
        if (codepage == CP_UTF7 || codepage == CP_UTF8)
            length = (size_t)::WideCharToMultiByte(codepage, 0, (LPCWCH)first, (int)(last - first) * sizeof(Elem), nullptr, 0, nullptr, nullptr);
        else
            length = (size_t)::WideCharToMultiByte(codepage, 0, (LPCWCH)first, (int)(last - first) * sizeof(Elem), nullptr, 0, (LPCCH)&State, nullptr);
        byte_string str;
        str.resize(length);
        if (codepage == CP_UTF7 || codepage == CP_UTF8)
            result = (size_t)::WideCharToMultiByte(codepage, 0, (LPCWCH)first, (int)(last - first) * sizeof(Elem), const_cast<char*>(str.c_str()), (int)(length + 1), nullptr, nullptr);
        else
            result = (size_t)::WideCharToMultiByte(codepage, 0, (LPCWCH)first, (int)(last - first) * sizeof(Elem), const_cast<char*>(str.c_str()), (int)(length + 1), (LPCCH)&State, nullptr);
        str.resize(nconv = ::strlen(str.c_str()));
        assert(length >= nconv);
        return ::std::move(str);
    }
    convert_cp_unicode_t(const convert_cp_unicode_t&) = delete;
    convert_cp_unicode_t& operator=(const convert_cp_unicode_t&) = delete;
private:
    byte_string berr;
    wide_string werr;
    state_type State;  // the remembered State
    bool has_state;
    bool has_berr;
    bool has_werr;
    size_t nconv;
};
SYSCONAPI convert_cp_unicode_t<CP_UTF8, wchar_t> convert_utf8_unicode;
SYSCONAPI convert_cp_unicode_t<CP_ACP, wchar_t> convert_ansi_unicode;
#else  /* _MSC_VER */
SYSCONAPI ::std::wstring_convert<::std::codecvt_utf8<wchar_t>, wchar_t> convert_utf8_unicode;
#define convert_ansi_unicode convert_utf8_unicode
#endif  /* _MSC_VER */

#ifdef _UNICODE
typedef ::std::wstringstream    _tstringstream;
#else  /* _UNICODE */
typedef ::std::stringstream     _tstringstream;
#endif  /* _UNICODE */


template<class T, class Arg> inline void debug_put(::std::basic_stringstream<char, T>&& ss, Arg&& arg)
{
    ss << ::std::forward<Arg>(arg);
#ifdef _MSC_VER
    auto&& uni_str = convert_ansi_unicode.from_bytes(ss.str());
    auto&& cvt_str = convert_utf8_unicode.to_bytes(uni_str);
#else  /* _MSC_VER */
    auto&& cvt_str = ss.str();
#endif  /* _MSC_VER */
    g_log_ofstream << cvt_str;
    g_log_ofstream.flush();
#if defined(_DEBUG) || defined(DEBUG)
#ifdef _MSC_VER
    ::OutputDebugStringW(uni_str.c_str());
#else  /* _MSC_VER */
    ::OutputDebugStringA(cvt_str.c_str());
#endif  /* _MSC_VER */
#endif
}

template<class T, class Arg> inline void debug_put(::std::basic_stringstream<wchar_t, T>&& ss, Arg&& arg)
{
    ss << ::std::forward<Arg>(arg);
    auto&& uni_str = ss.str();
    auto&& cvt_str = convert_utf8_unicode.to_bytes(uni_str);
    g_log_ofstream << cvt_str;
    g_log_ofstream.flush();
#if defined(_DEBUG) || defined(DEBUG)
    ::OutputDebugStringW(uni_str.c_str());
#endif
}

inline void _debug_output()
{
    auto now = ::std::chrono::system_clock::to_time_t(::std::chrono::system_clock::now());
    debug_put(_tstringstream(), _T(" [TID:"));
    debug_put(_tstringstream(),
#if defined(_WIN32) || defined(WIN32)
        ::GetCurrentThreadId()
#else // Linux
        ::gettid()
#endif // #if defined(_WIN32) || defined(WIN32)
        );
    debug_put(_tstringstream(), _T("] "));
#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4996 )
    debug_put(_tstringstream(), ::_tctime(&now));
#pragma warning( pop )
#else // _MSC_VER
    debug_put(::std::stringstream(), ::std::ctime(&now));
#endif // #ifdef _MSC_VER
}

template<class T, class... Args> inline void _debug_output(T&& arg, Args&&... args)
{
    debug_put(_tstringstream(), arg);
    _debug_output(::std::forward<Args>(args)...);
}

template<class... Args> inline void _debug_output(const char* debug_string, Args&&... args)
{
    debug_put(::std::stringstream(), debug_string);
    _debug_output(::std::forward<Args>(args)...);
}

template<class... Args> inline void _debug_output(volatile char* debug_string, Args&&... args)
{
    debug_put(::std::stringstream(), debug_string);
    _debug_output(::std::forward<Args>(args)...);
}

template<class... Args> inline void _debug_output(const volatile char* debug_string, Args&&... args)
{
    debug_put(::std::stringstream(), debug_string);
    _debug_output(::std::forward<Args>(args)...);
}

template<class T, class A, class... Args> inline void _debug_output(const ::std::basic_string<char, T, A>& debug_string, Args&&... args)
{
    debug_put(::std::stringstream(), debug_string);
    _debug_output(::std::forward<Args>(args)...);
}

template<class... Args> inline void _debug_output(const wchar_t* debug_string, Args&&... args)
{
    debug_put(::std::wstringstream(), debug_string);
    _debug_output(::std::forward<Args>(args)...);
}

template<class... Args> inline void _debug_output(volatile wchar_t* debug_string, Args&&... args)
{
    debug_put(::std::wstringstream(), debug_string);
    _debug_output(::std::forward<Args>(args)...);
}

template<class... Args> inline void _debug_output(const volatile wchar_t* debug_string, Args&&... args)
{
    debug_put(::std::wstringstream(), debug_string);
    _debug_output(::std::forward<Args>(args)...);
}

template<class T, class A, class... Args> inline void _debug_output(const ::std::basic_string<wchar_t, T, A>& debug_string, Args&&... args)
{
    debug_put(::std::wstringstream(), debug_string);
    _debug_output(::std::forward<Args>(args)...);
}

#if defined(_DEBUG) || defined(DEBUG)
// Debug下，无论output为何值总是输出。Release下，output为true时输出
template<bool output = false, class... Args> inline void debug_output(Args&&... args)
{
    ::std::lock_guard<::std::mutex> lck(g_log_lock);
    _debug_output(decay_type(::std::forward<Args>(args))...);
}

#else // NDEBUG
// Debug下，无论output为何值总是输出。Release下，output为true时输出。不输出使用debug_output(...)
template<bool output = false, class... Args> inline void debug_output(Args&&... args)
{
    if (output)
    {
        ::std::lock_guard<::std::mutex> lck(g_log_lock);
        _debug_output(decay_type(::std::forward<Args>(args))...);
    }
}

// Debug下，无论output为何值总是输出。Release下，默认不输出。输出使用debug_output<true>(...)
#define debug_output(...)   void()

#endif //#if defined(_DEBUG) || defined(DEBUG)


// 设置log流的文件位置
template<class Elem, class T = ::std::char_traits<Elem>, class A = ::std::allocator<Elem>> inline
void set_log_location(::std::basic_string<Elem, T, A> file_name)
{
    set_log_location(file_name.c_str());
}

// 设置log流的文件位置
template<class Elem> inline void set_log_location(const Elem* file_name)
{
    g_log_ofstream.open(file_name, ::std::ios::app | ::std::ios::ate);
    debug_output<true>(_T("Process Start: [PID:"),
#if defined(_WIN32) || defined(WIN32)
        ::GetCurrentProcessId()
#else // Linux
        ::getpid()
#endif // #if defined(_WIN32) || defined(WIN32)
        , _T(']'));
}
