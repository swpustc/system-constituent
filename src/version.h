/**********************************************************
* 版本号头文件
* 支持平台：Windows; Linux
* 编译环境：VS2010+; g++
***********************************************************/

#include "system_constituent_version.h"

#define INTERAL_NAME        _T("system") _T(_CRT_STRINGIZE(SYSTEM_CONSTITUENT_VERSION))
#define PRODUCT_NAME        _T("system constituent")
#define SUPPORT_AUTHOR      _T("宋万鹏")
#define EMAIL_ADDRESS       _T("swpustc@mail.ustc.edu.cn")
#define COMPANY_NAME        _T("杭州赤霄科技有限公司")
#define LEGAL_COPYRIGHT     _T("Copyright (C) 2015-2015 ") COMPANY_NAME

#define FILE_VERSION        _T(_CRT_STRINGIZE(FILE_VERSION_MAJOR)) _T(".") _T(_CRT_STRINGIZE(FILE_VERSION_MINOR)) _T(".") _T(_CRT_STRINGIZE(FILE_VERSION_POINT)) _T(".") _T(_CRT_STRINGIZE(FILE_VERSION_POINT2))
