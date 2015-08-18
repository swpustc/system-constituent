/**********************************************************
* 串口对象
* 支持平台：Windows
* 编译环境：VS2013+
***********************************************************/

#include "common.h"
#include "serial_port.h"
#include <tchar.h>

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
    swprintf_s(name, L"\\\\.\\COM%d", port_number);
    return __open(name);
}


// 获取默认选项类型
DCB serial_port::_get_option() const
{
    DCB dcb = { 0 };
    dcb.DCBlength = sizeof(dcb);
    if (is_open())
        GetCommState(m_comm, &dcb);                 /* 获取DCB串口配置数据结构              */
    else
    {
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

// 设置选项
bool serial_port::_set_option(const DCB& dcb)
{
    if (!is_open())
        return false;
    if (!SetCommState(m_comm, (LPDCB)&dcb))
        return false;
    return true;
}


// 从串口读数据
bool serial_port::read(void* data, size_t& size)
{
    if (!data)
        return false;
    if (!is_open())
        return false;
    size = auto_min(size, (size_t)INT16_MAX);
    while (true)
    {
        DWORD dwEvtMask = 0;
        if (!GetCommMask(m_comm, &dwEvtMask))   // 获取端口状态位
            return false;
        if (dwEvtMask & EV_RXCHAR)              // Any Character received
        {                                       // 接收字符
            DWORD dwErr;
            COMSTAT ComStat;                    // 获取缓冲长度
            if (!ClearCommError(m_comm, &dwErr, &ComStat))
                return false;
            size = auto_min(size, ComStat.cbInQue);
            if (size == 0)
                return true;
            DWORD read_bytes;
            if (!ReadFile(m_comm, data, (DWORD)size, &read_bytes, nullptr))
            {
                DWORD dwErr = GetLastError();
                switch (dwErr)
                {
                case ERROR_SUCCESS:
                case ERROR_IO_PENDING:
                    break;
                default:
                    PurgeComm(m_comm, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
                    return false;
                }
            }
            assert(read_bytes <= (DWORD)size);
            size = auto_min(read_bytes, size);  // 已读取到的字符
            if (size == 0)
                return false;
            return true;
        }
        else if (dwEvtMask & EV_RXFLAG)         // Received certain character
        {                                       // 接收特殊字符
            continue;
        }
        else if (dwEvtMask & EV_TXEMPTY)        // Transmitt Queue Empty
        {                                       // 数据已发送
            continue;
        }
        else if (dwEvtMask & EV_CTS)            // CTS changed state
        {                                       // CTS信号变化
            continue;
        }
        else if (dwEvtMask & EV_DSR)            // DSR changed state
        {                                       // 端口状态
            continue;
        }
        // else if(dwEvtMask & EV_RLSD)         // RLSD changed state
        else if (dwEvtMask & EV_BREAK)          // BREAK received
        {                                       // 端口中断
            continue;
        }
        else if (dwEvtMask & EV_ERR)            // Line status error occurred
        {                                       // 端口错误
            if (!PurgeComm(m_comm, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR))
                return false;
            else
                continue;
        }
        // else if(dwEvtMask & EV_RING)         // Ring signal detected
        // else if(dwEvtMask & EV_PERR)         // Printer error occured
        // else if(dwEvtMask & EV_RX80FULL)     // Receive buffer is 80 percent full
        // else if(dwEvtMask & EV_EVENT1)       // Provider specific event 1
        // else if(dwEvtMask & EV_EVENT2)       // Provider specific event 2
        else                                    // default
            return false;
    }
}

// 写数据到串口
size_t serial_port::write(const void* data, size_t size)
{
    if (!data)
        return 0;
    if (!is_open())
        return 0;
    if (size > (size_t)INT_MAX)
        return 0;
    if (size == 0)
        return 0;
    DWORD write_bytes;
    if (!WriteFile(m_comm, data, (DWORD)size, &write_bytes, nullptr))
    {
        DWORD dwErr = GetLastError();
        switch (dwErr)
        {
        case ERROR_SUCCESS:
        case ERROR_IO_PENDING:
            break;
        default:
            PurgeComm(m_comm, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
            return 0;
        }
    }
    assert(write_bytes <= (DWORD)size); // 已写入到的字符
    return write_bytes;
}

// 清空串口错误标志
void serial_port::clear()
{
    if (!is_open())
        return;
    PurgeComm(m_comm, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
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

// 数据位：枚举类
enum class serial_port::_byte_size_t : BYTE
{
    byte_size_4 = 4,
    byte_size_5 = 5,
    byte_size_6 = 6,
    byte_size_7 = 7,
    byte_size_8 = 8,
};

// 停止位：枚举类
enum class serial_port::_stop_bits_t : BYTE
{
    stop_bits_1 = ONESTOPBIT,
    stop_bits_1_5 = ONE5STOPBITS,
    stop_bits_2 = TWOSTOPBITS,
};

// 校验位：枚举类
enum class serial_port::_parity_t : BYTE
{
    parity_none = NOPARITY,
    parity_odd = ODDPARITY,
    parity_even = EVENPARITY,
    parity_mark = MARKPARITY,
    parity_space = SPACEPARITY,
};

// Dtr
enum class serial_port::_Dtr_t : DWORD
{
    Dtr_disable = DTR_CONTROL_DISABLE,
    Dtr_enable = DTR_CONTROL_ENABLE,
    Dtr_handshake = DTR_CONTROL_HANDSHAKE,
};

// Rts
enum class serial_port::_Rts_t : DWORD
{
    Rts_disbale = RTS_CONTROL_DISABLE,
    Rts_enable = RTS_CONTROL_ENABLE,
    Rts_handshake = RTS_CONTROL_HANDSHAKE,
    Rts_toggle = RTS_CONTROL_TOGGLE,
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

// 数据位
const serial_port::_byte_size serial_port::byte_size_4 = { _byte_size_t::byte_size_4 };
const serial_port::_byte_size serial_port::byte_size_5 = { _byte_size_t::byte_size_5 };
const serial_port::_byte_size serial_port::byte_size_6 = { _byte_size_t::byte_size_6 };
const serial_port::_byte_size serial_port::byte_size_7 = { _byte_size_t::byte_size_7 };
const serial_port::_byte_size serial_port::byte_size_8 = { _byte_size_t::byte_size_8 };

// 停止位
const serial_port::_stop_bits serial_port::stop_bits_1 = { _stop_bits_t::stop_bits_1 };
const serial_port::_stop_bits serial_port::stop_bits_1_5 = { _stop_bits_t::stop_bits_1_5 };
const serial_port::_stop_bits serial_port::stop_bits_2 = { _stop_bits_t::stop_bits_2 };

// 校验位
const serial_port::_parity serial_port::parity_none = { _parity_t::parity_none };
const serial_port::_parity serial_port::parity_odd = { _parity_t::parity_odd };
const serial_port::_parity serial_port::parity_even = { _parity_t::parity_even };
const serial_port::_parity serial_port::parity_mark = { _parity_t::parity_mark };
const serial_port::_parity serial_port::parity_space = { _parity_t::parity_space };

// DTR
const serial_port::_Dtr serial_port::Dtr_disable = { _Dtr_t::Dtr_disable };
const serial_port::_Dtr serial_port::Dtr_enable = { _Dtr_t::Dtr_enable };
const serial_port::_Dtr serial_port::Dtr_handshake = { _Dtr_t::Dtr_handshake };

// RTS
const serial_port::_Rts serial_port::Rts_disable = { _Rts_t::Rts_disbale };
const serial_port::_Rts serial_port::Rts_enable = { _Rts_t::Rts_enable };
const serial_port::_Rts serial_port::Rts_handshake = { _Rts_t::Rts_handshake };
const serial_port::_Rts serial_port::Rts_toggle = { _Rts_t::Rts_toggle };
