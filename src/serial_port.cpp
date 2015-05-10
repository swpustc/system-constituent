/**********************************************************
* 串口对象
* 支持平台：Windows
* 编译环境：VS2013+
* 创建时间：2015-05-10 （宋万鹏）
* 最后修改：2015-05-10 （宋万鹏）
***********************************************************/

#include "common.h"
#include "serial_port.h"
#include <tchar.h>
#include <Windows.h>

using namespace std;


static const wstring comm_name_prefix = L"\\\\.\\";

// 已验证过的串口名
bool serial_port::__open(const wchar_t* portname)
{
    m_comm = CreateFileW(portname, GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
    if (!is_open())
        return false;
    if (!SetCommMask(m_comm, EV_RXCHAR | EV_TXEMPTY))
        return false;
    // 设置输入输出缓冲区大小
    if (!SetupComm(m_comm, 1024, 1024))
        return false;
    //  清除状态位
    if (!PurgeComm(m_comm, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR))
        return false;
    COMMTIMEOUTS comm_timeouts = { 0 };
    if (!SetCommTimeouts(m_comm, &comm_timeouts))
        return false;
    return true;
}


bool serial_port::_open(const wchar_t* portname)
{
    if (wcsncmp(portname, comm_name_prefix.c_str(), comm_name_prefix.size()) == 0)
        return __open(portname);
    else
        return __open(comm_name_prefix + portname);
}

bool serial_port::_open(const ::std::wstring& portname)
{
    if (portname.compare(0, 4, comm_name_prefix) == 0)
        return __open(portname.c_str());
    else
        return __open(comm_name_prefix + portname);
}

bool serial_port::_open(::std::wstring&& portname)
{
    if (portname.compare(0, 4, comm_name_prefix) == 0)
        return __open(portname.c_str());
    else
        return __open(comm_name_prefix + move(portname));
}

bool serial_port::_open(int port_number)
{
    if (port_number <= 0 || port_number > 32)
        return false;
    wchar_t name[10];
    wsprintfW(name, L"\\\\.\\COM%d", port_number);
    return __open(name);
}


// 获取默认选项类型
DCB serial_port::_get_option_template() const
{
    DCB dcb = { 0 };
    if (is_open())
        GetCommState(m_comm, &dcb);                 /* 获取DCB串口配置数据结构             */
    else
    {
        dcb.DCBlength = sizeof(dcb);
        dcb.BaudRate        = CBR_9600;             /* Baudrate at which running            */
        dcb.fBinary         = 1;                    /* Binary Mode (skip EOF check)         */
        dcb.fParity         = 1;                    /* Enable parity checking               */
        dcb.fOutxCtsFlow    = 0;                    /* CTS handshaking on output            */
        dcb.fOutxDsrFlow    = 0;                    /* DSR handshaking on output            */
        dcb.fDtrControl     = DTR_CONTROL_DISABLE;  /* DTR Flow control                     */
        dcb.fDsrSensitivity = 0;                    /* DSR Sensitivity                      */
        dcb.fTXContinueOnXoff;                      /* Continue TX when Xoff sent           */
        dcb.fOutX           = 0;                    /* Enable output X-ON/X-OFF             */
        dcb.fInX            = 0;                    /* Enable input X-ON/X-OFF              */
        dcb.fErrorChar      = 0;                    /* Enable Err Replacement               */
        dcb.fNull           = 0;                    /* Enable Null stripping                */
        dcb.fRtsControl     = RTS_CONTROL_DISABLE;  /* Rts Flow control                     */
        dcb.fAbortOnError   = 0;                    /* Abort all reads and writes on Error  */
        dcb.fDummy2;                                /* Reserved                             */
        dcb.wReserved       = 0;                    /* Not currently used                   */
        dcb.XonLim          = 2;                    /* Transmit X-ON threshold              */
        dcb.XoffLim         = 4;                    /* Transmit X-OFF threshold             */
        dcb.ByteSize        = 8;                    /* Number of bits/byte, 4-8             */
        dcb.Parity          = NOPARITY;             /* 0-4=None,Odd,Even,Mark,Space         */
        dcb.StopBits        = ONESTOPBIT;           /* 0,1,2 = 1, 1.5, 2                    */
        dcb.XonChar         = 0x13;                 /* Tx and Rx X-ON character             */
        dcb.XoffChar        = 0x19;                 /* Tx and Rx X-OFF character            */
        dcb.ErrorChar       = 0;                    /* Error replacement char               */
        dcb.EofChar;                                /* End of Input character               */
        dcb.EvtChar         = 0;                    /* Received Event character             */
        dcb.wReserved1      = 0;                    /* Fill for now.                        */
    }
    return move(dcb);
}


// 波特率：枚举类
enum class serial_port::_baud_rate_t : DWORD
{
    baud_rate_110 = CBR_110,
    baud_rate_300 = CBR_300,
    baud_rate_600 = CBR_600,
    baud_rate_1200 = CBR_1200,
    baud_rate_2400 = CBR_2400,
    baud_rate_4800 = CBR_4800,
    baud_rate_9600 = CBR_9600,
    baud_rate_14400 = CBR_14400,
    baud_rate_19200 = CBR_19200,
    baud_rate_38400 = CBR_38400,
    baud_rate_56000 = CBR_56000,
    baud_rate_57600 = CBR_57600,
    baud_rate_115200 = CBR_115200,
    baud_rate_128000 = CBR_128000,
    baud_rate_256000 = CBR_256000,
};

// 导出的波特率选项
const serial_port::_baud_rate serial_port::baud_rate_110 = { _baud_rate_t::baud_rate_110 };
const serial_port::_baud_rate serial_port::baud_rate_300 = { _baud_rate_t::baud_rate_300 };
const serial_port::_baud_rate serial_port::baud_rate_600 = { _baud_rate_t::baud_rate_600 };
const serial_port::_baud_rate serial_port::baud_rate_1200 = { _baud_rate_t::baud_rate_1200 };
const serial_port::_baud_rate serial_port::baud_rate_2400 = { _baud_rate_t::baud_rate_2400 };
const serial_port::_baud_rate serial_port::baud_rate_4800 = { _baud_rate_t::baud_rate_4800 };
const serial_port::_baud_rate serial_port::baud_rate_9600 = { _baud_rate_t::baud_rate_9600 };
const serial_port::_baud_rate serial_port::baud_rate_14400 = { _baud_rate_t::baud_rate_14400 };
const serial_port::_baud_rate serial_port::baud_rate_19200 = { _baud_rate_t::baud_rate_19200 };
const serial_port::_baud_rate serial_port::baud_rate_38400 = { _baud_rate_t::baud_rate_38400 };
const serial_port::_baud_rate serial_port::baud_rate_56000 = { _baud_rate_t::baud_rate_56000 };
const serial_port::_baud_rate serial_port::baud_rate_57600 = { _baud_rate_t::baud_rate_57600 };
const serial_port::_baud_rate serial_port::baud_rate_115200 = { _baud_rate_t::baud_rate_115200 };
const serial_port::_baud_rate serial_port::baud_rate_128000 = { _baud_rate_t::baud_rate_128000 };
const serial_port::_baud_rate serial_port::baud_rate_256000 = { _baud_rate_t::baud_rate_256000 };
