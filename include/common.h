/**********************************************************
 * STL公共算法部分
 * 支持平台：Windows; Linux
 * 编译环境：VS2013+; g++ -std=c++11
 * 创建时间：2015-04-05 （宋万鹏）
 * 最后修改：2015-04-12 （宋万鹏）
 **********************************************************/

#ifndef __COMMON_H__
#define __COMMON_H__
#pragma once

#include <cstdint>
#include <type_traits>

#define EXTERN_C  extern "C"


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
    template<class Fn, class... Args> inline
    void operator()(Fn&& fn, Args&&... args)
    {
        fn(::std::forward<Args>(args)...);
    }
};


#endif // #ifndef __COMMON_H__
