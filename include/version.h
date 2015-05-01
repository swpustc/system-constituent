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
