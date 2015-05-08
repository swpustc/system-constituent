/**********************************************************
* 版本号头文件
* 支持平台：Windows; Linux
* 编译环境：VS2010+; g++
* 创建时间：2015-04-25 （宋万鹏）
* 最后修改：2015-05-08 （宋万鹏）
***********************************************************/

#define FILE_VERSION_MAJOR  1
#define FILE_VERSION_MINOR  2
#define FILE_VERSION_POINT  1
#define FILE_VERSION_POINT2 0

#define INTERAL_NAME        _T("system")
#define PRODUCT_NAME        _T("系统组件")
#define SUPPORT_AUTHOR      _T("宋万鹏")
#define EMAIL_ADDRESS       _T("swpustc@mail.ustc.edu.cn")
#define COMPANY_NAME        _T("杭州赤霄科技有限公司")
#define LEGAL_COPYRIGHT     _T("Copyright (C) 2015-2015 ") COMPANY_NAME

#ifndef _CRT_STRINGIZE
#define __CRT_STRINGIZE(_Value) #_Value
#define _CRT_STRINGIZE(_Value) __CRT_STRINGIZE(_Value)
#endif

#ifndef _T
#ifdef _UNICODE
#define __TXT(x)    L ## x
#else  /* _UNICODE */
#define __TXT(x)    x
#endif  /* _UNICODE */
#define _T(x)       __TXT(x)
#endif  /* _T */

#define FILE_VERSION        _T(_CRT_STRINGIZE(FILE_VERSION_MAJOR)) _T(".") _T(_CRT_STRINGIZE(FILE_VERSION_MINOR)) _T(".") _T(_CRT_STRINGIZE(FILE_VERSION_POINT)) _T(".") _T(_CRT_STRINGIZE(FILE_VERSION_POINT2))
