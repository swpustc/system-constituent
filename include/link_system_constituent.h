/**********************************************************
* ����
* ֧��ƽ̨��Windows; Linux
* ���뻷����VS2010+; g++
* ����ʱ�䣺2015-05-01 ����������
* ����޸ģ�2015-05-01 ����������
***********************************************************/

#pragma once

#ifdef _MSC_VER

#ifdef _DEBUG       /* _DEBUG */
#  ifdef _M_X64     /* _M_X64 */
#    pragma comment(lib, "Debug/x64/system.lib")
#  else             /* _M_X86 */
#    pragma comment(lib, "Debug/system.lib")
#  endif
#else               /* NDEBUG */
#  ifdef _M_X64     /* _M_X64 */
#    pragma comment(lib, "x64/system.lib")
#  else             /* _M_X86 */
#    pragma comment(lib, "system.lib")
#  endif
#endif

#endif /* _MSC_VER */
