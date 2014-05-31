// All rights reserved ADENEO EMBEDDED 2010
/*****************************************************************************
   
    Copyright (c) 2004-2008 SMSC. All rights reserved.

    Use of this source code is subject to the terms of the SMSC Software
    License Agreement (SLA) under which you licensed this software product.  
    If you did not accept the terms of the SLA, you are not authorized to use
    this source code. 

    This code and information is provided as is without warranty of any kind,
    either expressed or implied, including but not limited to the implied
    warranties of merchantability and/or fitness for a particular purpose.
     
    File name   : os.h
    Description : OS releted Header file

    History     :
        03-16-05 WH         First Release
        08-12-05 MDG        ver 1.01 
            - add LED1 inversion, add PHY work around
        11-07-05 WH         ver 1.02
            - Fixed middle buffer handling bug
              (Driver didn't handle middle buffers correctly if it is less than 
               4bytes size)
            - workaround for Multicast bug
            - Workaround for MAC RXEN bug
        11-17-05 WH         ver 1.03
            - 1.02 didn't have 1.01 patches
            - 1.03 is 1.02 + 1.01
        12-06-05 WH         ver 1.04
            - Fixed RX doesn't work on Silicon A1 (REV_ID = 0x011x0002)
            - Support SMSC9118x/117x/116x/115x family
        02-27-05 WH         ver 1.05
            - Fixing External Phy bug that doesn't work with 117x/115x
        03-23-05 WH         ver 1.06
            - Put the variable to avoid PHY_WORKAROUND for External PHY
            - Change product name to 9118x->9218, 9115x->9215
        07-26-06 WH, MDG, NL        ver 1.07
            - Add RXE and TXE interrupt handlers
            - Workaround Code for direct GPIO connection from 9118 family 
              Interrupt (Level Interrupt -> Edge Interrupt)
            - Change GPT interrupt interval to 200mSec from 50mSec
            - clean up un-used SH3 code
        08-25-06  WH, MDG, NL       ver 1.08
            - Fixed RXE and TXE interrupt handlers bug
            - support for direct and nondirect Interrupt
        02-15-07   NL               ver 1.09
            - First version of WinCE 6.0 driver
            - Removed Support for LAN9112
            - Added AutoMdix as modifiable parameter in the Registry
            - Fixed DMA Xmit Bug
        04-17-07   NL               ver 1.10
            - Added Support LAN9211 Chip
            - Changed Register Name ENDIAN to WORD_SWAP According to the Menual
            - Merged CE6.0 & 5.0 Drivers Together
        10-24-07   NL               ver 1.11
            - Added Support LAN9218A/LAN9217A/LAN9216A/LAN9215A Chips
        01-08-08   AH               ver 1.12
            - Added Support for LAN9210 Chip
        -----------------------------------------------------------------------
        09-26-08   WH
            - Move to version 2.00 intentionally
            - From version 2.00, 
               driver drops support chip ID of 0x011x0000 and 0x011x0001
        -----------------------------------------------------------------------
        09-26-08   WH               ver 2.00
            - replace TAB to SPACE
            - Reorder initialization routines to avoid 
               possible unexpect behavior
            - Fixed the issue Flow Control ignores "FlowControl" key 
               if not Auto Negotiation
            - Added "MinLinkChangeWakeUp", "MinMagicPacketWakeUp" and
               "MinPatternWakeUp" registry key for PM
            - Fixed PM issues
                - DHCP doesn't work after wakeup
                - Disable GPTimer while in sleep mode
                - Add routine to wakeup chip during Reset
            - Fix discarding Rx Frame when it is less than 16bytes
            - Enable RXE & RWT interrupt
            - Chip goes to D2(Energy Detect Power Down) when there is no link
               when POWERDOWN_AT_NO_LINK is defined (see smsc9118.h)
            - Clean up Registers which are absolete
            - See relnotes.txt for detail
        11-17-08   WH               ver 2.01
            - Lan_SetMiiRegW() and Lan_GetMiiRegW are changed to return BOOL.
              Caller checks return value to detect error
            - Tx Error Interrupt is enabled
            - Check return from NdisAllocateMemory() is 32bit aligned or not
              Technically it won't happen though
              See comments in source (smsc9118.c)
            - Fixed bug which does't go to AUTOIP when failed to get DHCP addr
		-----------------------------------------------------------------------
		07-24-09   RL
			- Added support for OMAP platform
*****************************************************************************/
#ifndef _OS_H_
#define _OS_H_

#ifdef  _lint
#define __cdecl 
#endif

#define NDIS_MINIPORT_DRIVER
#define NDIS50_MINIPORT 1

// some lint error suppressions for WinCE headers
// -e14     // Symbol previously defined
// -e43     // Vacuous type for variable
// -e46     // field type should be int
// -e123    // Macro defined with arguments someplace else
// -e427    // "//" comment terminates in "\"
// -e537    // Repeated include file
// -e620    // Suspicious constant (L or one?)
// -e652    // #define of symbol declared previously
// -e659    // Nothing follows '}' on line within struct/union/enum declaration
// -e683    // function #define'd, semantics may be lost

// -e726    // Extraneous comma ignored
// -e760    // Redundant macro xxx defined identically at line yyy
// -e761    // Redundant typedef previously declared
// -e762    // Redundantly declared symbol
// -e763    // Redundant declaration for symbol
// -e767    // macro was defined differently in another module 
// -e773    // Expression-like macro 'va_start' not parenthesized
// -e793    // ANSI limit of 8 'conditional inclusion levels' exceeded -- processing is unaffected
// -e806    // Small bit field is signed rather than unsigned
// -e828    // Semantics of function 'setjmp' copied to function '_setjmp'

// -e935    // int within struct
// -e937    // old-style function declaration for function
// -e950    // Non-ANSI reserved word or construct
// -e955    // Parameter name missing from prototype for function
// -e956    // Non const, non volatile static or external variable
// -e957    // Function defined without a prototype in scope
// -e958    // Padding is required to align member on 'n' byte boundary
// -e959    // Nominal struct size is not an even multiple of the maximum member alignment
// -e960    // a whole bunch of invidiaul MISRA Required Rules
// -e961    // a whole bunch of individual MISRA Advisory Rules
// -e962    // Macro defined identically at another location 
// -e973    // Unary operator in macro not parenthesized
// -e1916   // Ellipsis encountered
// -e912    // Implicit binary conversion from int to unsigned int

/*lint -save*/
/*lint -e14 -e43 -e46 -e123 -e427 -e537 -e620 -e652 -e659 -e683*/
/*lint -e726 -e760 -e761 -e762 -e763 -e767 -e773 -e793 -e806 -e828 -e912*/
/*lint -e935 -e937 -e950 -e955 -e956 -e957 -e958 -e959 -e960 -e961 -e962*/
/*lint -e973 -e1916*/
#include <ndis.h>
#include <winreg.h>
#include <nkintr.h>
#include <Pkfuncs.h>
#include <pm.h>
/*lint -restore*/

typedef UCHAR const *CPUCHAR;

__inline void SetRegDW(const DWORD dwBase, const DWORD dwOffset, const DWORD dwVal);
__inline DWORD GetRegDW(const DWORD dwBase, const DWORD dwOffset);
__inline void WriteFifo(const DWORD dwBase, const DWORD dwOffset, const DWORD *pdwBuf, DWORD dwDwordCount);
__inline void ReadFifo(const DWORD dwBase, const DWORD dwOffset, DWORD *pdwBuf, DWORD dwDwordCount);

__inline void SetRegDW(
    const DWORD dwBase,
    const DWORD dwOffset,
    const DWORD dwVal)
{
    (*(volatile DWORD *)(dwBase + dwOffset)) = dwVal;
}

__inline DWORD GetRegDW(
    const DWORD dwBase,
    const DWORD dwOffset)
{
    return (DWORD)(*(volatile DWORD *)(dwBase + dwOffset));
}

__inline void WriteFifo(
    const DWORD dwBase,
    const DWORD dwOffset,
    const DWORD *pdwBuf,
    DWORD dwDwordCount)
{
    volatile DWORD * pdwReg;
    pdwReg = (volatile DWORD *)(dwBase + dwOffset);
    
    while (dwDwordCount)
    {
        *pdwReg = *pdwBuf++;
        dwDwordCount--;
    }
}

__inline void ReadFifo(
    const DWORD dwBase,
    const DWORD dwOffset,
    DWORD *pdwBuf,
    DWORD dwDwordCount)
{
    const volatile DWORD * const pdwReg = 
        (const volatile DWORD * const)(dwBase + dwOffset);
    
    while (dwDwordCount)
    {
        *pdwBuf++ = *pdwReg;
        dwDwordCount--;
    }
}

#define SMSC_ZERO_MEMORY(bufferPtr,size)    NdisZeroMemory (bufferPtr, size)
#define SMSC_MICRO_DELAY(uCount)            NdisStallExecution(uCount)
//#define SMSC_MICRO_DELAY(uCount)          NdisMSleep(uCount)

//
//  PACKET QUEUE & MACROS
//

//
// Queue structure(FIFO) for packet.
//
typedef struct Queue 
{
    PNDIS_PACKET FirstPacket;
    PNDIS_PACKET LastPacket;
    DWORD Count;
} Queue_t, *pQueue_t;


//
// Initializing a Queue data structure before 
// any of the insert/remove/count operations can be performed.
//
#define QUEUE_INIT(List) {\
    (List)->FirstPacket = (PNDIS_PACKET) NULL;\
    (List)->LastPacket = (PNDIS_PACKET) NULL;\
    (List)->Count = 0UL;\
}

#define QUEUE_IS_EMPTY(List) (((List)->FirstPacket == 0) ? 1 : 0 )
#define QUEUE_COUNT(List) ((List)->Count)

//
// On the TX side, defines a link pointer structure to queue up NDIS
// packets. On the RX side, defines a link pointer to the HALPacket
// associated with the NDIS packet.
//        
typedef struct _SMSC9118_RESERVED 
{
    PNDIS_PACKET Next;
    DWORD Tag;
} SMSC9118_RESERVED, *PSMSC9118_RESERVED;

#define SET_PACKET_RESERVED_RXSTS(_Packet, _Data)           \
    ((*(PDWORD)(_Packet)->MiniportReserved) = (DWORD)(_Data))

#define GET_PACKET_RESERVED_RXSTS(_Packet)                  \
    (*(PDWORD)(_Packet)->MiniportReserved)

#define SET_PACKET_RESERVED_PA(_Packet, _Data)          \
    ((*(PDWORD)&((_Packet)->MiniportReserved[4])) = (DWORD)(_Data))

#define GET_PACKET_RESERVED_PA(_Packet)                 \
    (*(PDWORD)&((_Packet)->MiniportReserved[4]))
//
// Defines a SMSC9118_RESERVED structure in the first 4 bytes of space reserved
// for a miniport driver inside an NDIS packet
//
#define PSMSC9118_RESERVED_FROM_PACKET(_Packet) \
    ((PSMSC9118_RESERVED)((_Packet)->MiniportReserved))

#define EnqueuePacket(_Head, _Tail, _Packet, _TAG)              \
{                                                               \
    if (!_Head)                                                 \
    {                                                           \
        _Head = _Packet;                                        \
    }                                                           \
    else                                                        \
    {                                                           \
        PSMSC9118_RESERVED_FROM_PACKET(_Tail)->Next = _Packet;   \
    }                                                           \
    PSMSC9118_RESERVED_FROM_PACKET(_Packet)->Next = NULL;        \
    if (_TAG)                                                   \
    {                                                           \
        PSMSC9118_RESERVED_FROM_PACKET(_Packet)->Tag = _TAG;     \
    }                                                           \
    _Tail = _Packet;                                            \
}

#define DequeuePacket(_Head, _Tail)                 \
{                                                   \
    const SMSC9118_RESERVED * const Reserved =       \
        PSMSC9118_RESERVED_FROM_PACKET(_Head);       \
    if (!Reserved->Next)                            \
    {                                               \
        _Tail = NULL;                               \
    }                                               \
    _Head = Reserved->Next;                         \
}


//
//  DEBUG MACROS
//
#define DBG_INIT            0x00000001UL
#define DBG_TX              0x00000002UL
#define DBG_RX              0x00000004UL
#define DBG_PHY             0x00000008UL
#define DBG_SHUT_DOWN       0x00000010UL
#define DBG_ISR             0x00000020UL
#define DBG_DMA             0x00000040UL
#define DBG_MULTICAST       0x00000080UL
#define DBG_FLOW            0x00000100UL
#define DBG_POWER           0x00000200UL
#define DBG_EEPROM          0x00000400UL
#define DBG_SET_OID         0x00001000UL
#define DBG_QUERY_OID       0x00002000UL
#define DBG_ENABLE_BREAK    0x80000000UL
#define DBG_ERROR           0x20000000UL
#define DBG_WARNING         0x10000000UL

#define SMSC9118_NDIS_MAJOR_VERSION 5
#define SMSC9118_NDIS_MINOR_VERSION 0


#ifdef SMSC_DBG
extern DWORD g_DebugMode;

#define SMSC_SET_DEBUG_MODE(debugMode) {g_DebugMode=debugMode;}

#define SMSC_BREAK

#define SMSC_WARNING(printf_exp)                        \
        {                                               \
            RETAILMSG(1, (TEXT("SMSC_WARNING: ")));     \
            RETAILMSG(1, (TEXT(" Func:%s, Line:%d "), __FILE__, __LINE__)); \
            RETAILMSG(1, (printf_exp));                 \
        }

#define SMSC_WARNING0(message)                          \
        {                                               \
            RETAILMSG(1, (TEXT("SMSC_WARNING: ")));     \
            RETAILMSG(1, (TEXT(message)));              \
        }

#define SMSC_WARNING1(message, arg)                     \
        {                                               \
            RETAILMSG(1, (TEXT("SMSC_WARNING: ")));     \
            RETAILMSG(1, (TEXT(message), arg));         \
        }

#define SMSC_WARNING2(message, arg1, arg2)              \
        {                                               \
            RETAILMSG(1, (TEXT("SMSC_WARNING: ")));     \
            RETAILMSG(1, (TEXT(message), arg1, arg2));  \
        }

#define SMSC_WARNING3(message, arg1, arg2, arg3)                \
        {                                                       \
            RETAILMSG(1, (TEXT("SMSC_WARNING: ")));             \
            RETAILMSG(1, (TEXT(message), arg1, arg2, arg3));    \
        }

#define SMSC_TRACE0(job, message)                           \
        if ((job)&g_DebugMode) {                    \
            RETAILMSG(1, (TEXT(message)));                  \
        }

#define SMSC_TRACE1(job, message, arg)                      \
        if ((job)&g_DebugMode) {                    \
            RETAILMSG(1, (TEXT(message), arg));             \
        }

#define SMSC_TRACE2(job, message, arg1, arg2)               \
        if ((job)&g_DebugMode) {                    \
            RETAILMSG(1, (TEXT(message), arg1, arg2));      \
        }

#define SMSC_TRACE3(job, message, arg1, arg2, arg3)         \
        if ((job)&g_DebugMode) {                    \
            RETAILMSG(1, (TEXT(message), arg1, arg2, arg3));    \
        }

#define SMSC_TRACE4(job, message, arg1, arg2, arg3, arg4)   \
        if ((job)&g_DebugMode) {                    \
            RETAILMSG(1, (TEXT(message), arg1, arg2, arg3, arg4));    \
        }

#define SMSC_TRACE5(job, message, arg1, arg2, arg3, arg4, arg5) \
        if ((job)&g_DebugMode) {                    \
            RETAILMSG(1, (TEXT(message), arg1, arg2, arg3, arg4, arg5));    \
        }

#define SMSC_TRACE6(job, message, arg1, arg2, arg3, arg4, arg5, arg6)   \
        if ((job)&g_DebugMode) {                    \
            RETAILMSG(1, (TEXT(message), arg1, arg2, arg3, arg4, arg5, arg6)); \
        }

#define SMSC_ASSERT(condition)      \
        if(!(condition)) {          \
            RETAILMSG(1, (TEXT("SMSC_ASSERTION_FAILURE: File=%s, Line=%d\r\n"), TEXT(__FILE__), __LINE__));    \
            SMSC_BREAK;             \
        }

#else   // SMSC_DBG
#define SMSC_SET_DEBUG_MODE(debugMode)
#define SMSC_BREAK
#define SMSC_WARNING(printf_exp)
#define SMSC_WARNING0(msg)
#define SMSC_WARNING1(msg,arg1)
#define SMSC_WARNING2(msg,arg1,arg2)
#define SMSC_WARNING3(msg,arg1,arg2,arg3)
#define SMSC_ASSERT(condition)
#define SMSC_TRACE0(job, msg)
#define SMSC_TRACE1(job, msg,arg1)
#define SMSC_TRACE2(job, msg,arg1,arg2)
#define SMSC_TRACE3(job, msg,arg1,arg2,arg3)
#define SMSC_TRACE4(job, msg,arg1,arg2,arg3,arg4)
#define SMSC_TRACE6(job, msg,arg1,arg2,arg3,arg4,arg5,arg6)
#endif  // SMSC_DBG
#endif // _OS_H_
