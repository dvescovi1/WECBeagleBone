/*
 * edma3_log.h
 *
 * EDMA3 logging/tracing service
 *
 * Copyright (C) 2009 Texas Instruments Incorporated - http://www.ti.com/
 *
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
*/

#ifndef _EDMA3_LOG_H_
#define _EDMA3_LOG_H_

/*#include <std.h>*/
#include <log.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup EDMA3 Log Service
 *
 *  EDMA3 error/event/message logging/tracing service
 */
/*@{*/

/*
    Note : This header file is used to BIOS logs only. For porting purposes this
           file need to replace EDMA3_LOG_EVENT macro.

           This header file uses log format as defined by Socrates Instrumentation.
           Any change in the file will result in incorrect interpretation by Socrates Tool.

                                  BIOS Log Format
-------------------------------------------------------------------------------
| TimeStamp | Sequence # | Event # | Event Descriptor | Data1 | Data2 | Data3 |
| (2 words) | by BIOS    | by BIOS |                  |       |       |       |
-------------------------------------------------------------------------------

LogBuffer:          A single log buffer called DVTEvent_Log will be used for all event logs
TimeStamp:          Automatically inserted by BIOS Log_Printf4.
Sequence #:         Automatically inserted by BIOS Log_Printf4. This is used to detect data loss.
Event:              Name to uniquely identify event. This is the name that will be used in the visualization.
                    (Limitation: RTA requires the name to be a static global)
Event Descriptor:   The Event Descriptor is created using the EDMA3_DVT_DESC Macro.
                    This Macro is of the following format (see tables below for EventType and ArgType):
                    EDMA3_DVT_DESC(EventType, Arg1Type, Arg2Type, Arg3Type)

Note: Event # is generated by BIOS based on unique String provided as input to "Log_Printf4".
This string is extracted by Sorcates via the map file.

Note:
1. Only three pieces of data can be included in any log
2. Data needs to be included in the same order as in table.
   i.e. if logging INITIATOR, SIZE and CORR then INITIATOR
   must be in arg1, CORR in arg2 and SIZE in arg3

Example:

ISRx()
{
    Log_Printf4(&DVTEvent_Log, "ISR1", EDMA3_DVT_DESC(EDMA3_DVT_INT_eSTART, EDMA3_DVT_dNONE, EDMA3_DVT_dNONE,
        EDMA3_DVT_dNONE))
    :
    :
    Log_Printf4(&DVTEvent_Log, "ISR1", EDMA3_DVT_DESC(EDMA3_DVT_INT_eEND, EDMA3_DVT_dSIZE, EDMA3_DVT_dNONE,
        EDMA3_DVT_dNONE), sizeof(Buffer))
}


*/
extern /*far*/ LOG_Obj DVTEvent_Log;
typedef enum
{
    EDMA3_DVT_eINT          ,   /* Interrupt Event. Use this only is logging
                                       that the interrupt occurred. Use INT_START/END
                                       if tracking the entry and exit of interrupt */
    EDMA3_DVT_eINT_START,               /* Enter Interrupt service routine */

    EDMA3_DVT_eINT_END,             /* Exit Interrupt service routine */

    EDMA3_DVT_eFUNC,                    /* Interrupt Event. Use this only is logging that
                                       the function was called. Use FUNC_START/END if
                                       tracking the entry and exit of function */
    EDMA3_DVT_eFUNC_START,          /* Enter function service routine */

    EDMA3_DVT_eFUNC_END,                /* Exit function service routine */

    EDMA3_DVT_ePACKET_START,            /* Start of a Packet */

    EDMA3_DVT_ePACKET_END,          /* End of a Packet */

    EDMA3_DVT_eDATA_SND,                /* An event that has a free running counter value
                                       associated with it */
    EDMA3_DVT_eDATA_SND_START,      /* A COUNTER Event that has a corresponding
                                       ENDCOUNTER event */
    EDMA3_DVT_eDATA_SND_END,            /* End of a STARTCOUNTER event */

    EDMA3_DVT_eDATA_RCV,                /* An Event that has a value associated with it */

    EDMA3_DVT_eRCV_START,               /* A VALUE event that has a corresponding ENDVALUE
                                       event */
    EDMA3_DVT_eRCV_END,             /* End of a STARTVALUE event */

    EDMA3_DVT_eSMPL_COUNTER,            /* Sample some free running counter */

    EDMA3_DVT_eEVENT,                   /* Events not explicitly defined above */

    EDMA3_DVT_eEVENT_START,         /* Start of an Event not mentioned in the above list */

    EDMA3_DVT_eEVENT_END                /* End of an Event not mentioned in the above list */

} EDMA3_logEventType;

typedef enum
{
    EDMA3_DVT_dNONE ,   /* No Data */

    EDMA3_DVT_dINST,                    /* ID for this instance of the driver. This is necessary
                                       if separate analysis is required for different instances
                                       of a multiple instance driver. */
    EDMA3_DVT_dINITIATOR,               /* ID of the component that initiated this driver request */

    EDMA3_DVT_dMSG_ID,              /* Use to correlate START and END events. This is not
                                       necessary if  events are sequenced, i.e. no more than
                                       1 START is pending at any given time. */
    EDMA3_DVT_dCOUNTER,             /* Value of a free running counter */

    EDMA3_DVT_dSIZE_BYTES,          /* Size in number of bytes */

    EDMA3_DVT_dSIZE_WORDS,          /* Size in number of words */

    EDMA3_DVT_dPADD,                    /* Program Address */

    EDMA3_DVT_dDADD,                    /* Data address */

    EDMA3_DVT_dDATA,                    /* Some Data */

    EDMA3_DVT_dPACKET_ID,               /* Packet ID */

    EDMA3_DVT_dCHANNEL_ID               /* Channel ID */

} EDMA3_logDataDesc;


#define ARG1(arg1) (arg1 << 8)
#define ARG2(arg2) (arg2 << 16)
#define ARG3(arg3) (arg3 << 24)

#define EDMA3_DVT_DESC(event, arg1, arg2, arg3) (event | ARG1(arg1) | ARG2(arg2) | ARG3(arg3))

/*
 * EDMA3 Event Log Macro
 *
 * Macro to log the event
 */
#define EDMA3_LOG_EVENT     LOG_printf4


/* Examples : For instrumenting a driver which is working in interrupt mode.

Driver Write Function:

    Drv_Write(...)
    {
        EDMA3_LOG_EVENT(hLog, "DRV", EDMA3_DVT_DESC(EDMA3_DVT_eFUNC_START, EDMA3_DVT_dNONE, EDMA3_DVT_dNONE, EDMA3_DVT_dNONE));
        EDMA3_LOG_EVENT(hLog, "DRV", EDMA3_DVT_DESC(EDMA3_DVT_ePACKET_START, EDMA3_DVT_dPACKET_ID, EDMA3_DVT_dNONE, EDMA3_DVT_dNONE), packetId);

        .
        .
        EDMA3_LOG_EVENT(hLog, "DRV", EDMA3_DVT_DESC(EDMA3_DVT_eDATA_SND_START, EDMA3_DVT_dNONE, EDMA3_DVT_dNONE, EDMA3_DVT_dNONE));
        .
        .
        .

        EDMA3_LOG_EVENT(hLog, "DRV", EDMA3_DVT_DESC(EDMA3_DVT_eFUNC_END, EDMA3_DVT_dSIZE_BYTES, EDMA3_DVT_dNONE, EDMA3_DVT_dNONE), Data Transferred);
    }

Note: In case the driver is asychronous, then the FUNC_END event will be placed before calling
      the completion call back.


    DRV_ISR(...)
    {
        Case: Intermediate Transfer complete
            EDMA3_LOG_EVENT(hLog, "DRV", EDMA3_DVT_DESC(EDMA3_DVT_eDATA_SND_END, EDMA3_DVT_dSIZE_BYTES, EDMA3_DVT_dNONE, EDMA3_DVT_dNONE), Data written to Hardware);

        case: More Data Pending
            EDMA3_LOG_EVENT(hLog, "DRV", EDMA3_DVT_DESC(EDMA3_DVT_eDATA_SND_START, EDMA3_DVT_dNONE, EDMA3_DVT_dNONE, EDMA3_DVT_dNONE));
    }
*/
/*@}*/

#ifdef __cplusplus
}
#endif /* extern "C" */

#endif /* _EDMA3_LOG_H_ */