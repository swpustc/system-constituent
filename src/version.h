/**********************************************************
* �汾��ͷ�ļ�
* ֧��ƽ̨��Windows; Linux
* ���뻷����VS2010+; g++
* ����ʱ�䣺2015-04-25 ����������
* ����޸ģ�2015-05-08 ����������
***********************************************************/

#define FILE_VERSION_MAJOR  1
#define FILE_VERSION_MINOR  2
#define FILE_VERSION_POINT  1
#define FILE_VERSION_POINT2 0

#define INTERAL_NAME        _T("system")
#define PRODUCT_NAME        _T("ϵͳ���")
#define SUPPORT_AUTHOR      _T("������")
#define EMAIL_ADDRESS       _T("swpustc@mail.ustc.edu.cn")
#define COMPANY_NAME        _T("���ݳ����Ƽ����޹�˾")
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
