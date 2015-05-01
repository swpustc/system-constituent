/**********************************************************
* 版本号头文件
* 支持平台：Windows; Linux
* 编译环境：VS2013+; g++ -std=c++11
* 创建时间：2015-04-05 （宋万鹏）
* 最后修改：2015-05-01 （宋万鹏）
***********************************************************/

#define FILE_VERSION_MAJOR  1
#define FILE_VERSION_MINOR  0
#define FILE_VERSION_POINT  0
#define FILE_VERSION_POINT2 1

#define INTERAL_NAME        "system"
#define EMAIL_ADDRESS       "swpustc@mail.ustc.edu.cn"
#define LEGAL_COPYRIGHT     "Copyright (C) 2015-2015 Chixiao Tech."

#ifndef _CRT_STRINGIZE
#define __CRT_STRINGIZE(_Value) #_Value
#define _CRT_STRINGIZE(_Value) __CRT_STRINGIZE(_Value)
#endif
#define FILE_VERSION        _CRT_STRINGIZE(FILE_VERSION_MAJOR) "." _CRT_STRINGIZE(FILE_VERSION_MINOR) "." _CRT_STRINGIZE(FILE_VERSION_POINT) "." _CRT_STRINGIZE(FILE_VERSION_POINT2)
