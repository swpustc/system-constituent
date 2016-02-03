/**********************************************************
* 版本号头文件
* 支持平台：Windows; Linux
* 编译环境：VS2010+; g++
***********************************************************/

#include "system_constituent_version.h"
#include "GIT_HEAD_MASTER"

#define FILE_VERSION_MAJOR  SYSTEM_CONSTITUENT_VERSION_MAJOR
#define FILE_VERSION_MINOR  SYSTEM_CONSTITUENT_VERSION_MINOR
#define FILE_VERSION_POINT  SYSTEM_CONSTITUENT_VERSION_POINT
#define FILE_VERSION_POINT2 SYSTEM_CONSTITUENT_VERSION_POINT2


#define INTERAL_NAME        _T(_CRT_STRINGIZE(PROJECT_NAME))
#define FILE_DESCRIPTION    _T("System Constituent")
#define PRODUCT_NAME        FILE_DESCRIPTION _T(" ") _T(_CRT_STRINGIZE(GIT_HEAD_MASTER))
#define SUPPORT_AUTHOR      _T("宋万鹏")
#define EMAIL_ADDRESS       _T("swpustc@mail.ustc.edu.cn")
#define COMPANY_NAME        _T("杭州赤霄科技有限公司")
#define LEGAL_COPYRIGHT     _T("Copyright (C) 2015-2016 ") COMPANY_NAME

#define FILE_VERSION        _T(_CRT_STRINGIZE(FILE_VERSION_MAJOR)) _T(".")    \
                            _T(_CRT_STRINGIZE(FILE_VERSION_MINOR)) _T(".")    \
                            _T(_CRT_STRINGIZE(FILE_VERSION_POINT)) _T(".")    \
                            _T(_CRT_STRINGIZE(FILE_VERSION_POINT2))
