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
