/**********************************************************
 * STL公共算法部分
 * 支持平台：Windows, Linux
 * 宋万鹏 2015-04-05
 **********************************************************/

#ifndef __COMMON_H__
#define __COMMON_H__
#pragma once

#include <type_traits>

#define EXTERN_C  extern "C"


template<class T> inline
typename ::std::decay<T>::type decay_type(T&& arg)
{
    return ::std::forward<T>(arg);
}


#endif // #ifndef __COMMON_H__
