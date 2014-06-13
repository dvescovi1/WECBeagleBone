//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this sample source code is subject to the terms of the Microsoft
// license agreement under which you licensed this sample source code. If
// you did not accept the terms of the license agreement, you are not
// authorized to use this sample source code. For the terms of the license,
// please see the license agreement between you and Microsoft or, if applicable,
// see the LICENSE.RTF on your install media or the root of your tools installation.
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//
/*
================================================================================
*             Texas Instruments OMAP(TM) Platform Software
* (c) Copyright Texas Instruments, Incorporated. All Rights Reserved.
*
* Use of this software is controlled by the terms and conditions found
* in the license agreement under which this software has been supplied.
*
================================================================================
*/
//
//  File: map.c
//
//  This file implements functionality related to mapping between IRQ and
//  SYSINTR. There is custom implementation for OMAPV1030 because hardware
//  supports 96 different IRQs compared to maximal 64 IRQs allowed in generic
//  implementation. The implementation also supports multiple IRQ per one
//  SYSINTR mapping.
//
#include <windows.h>
#include <nkintr.h>
#include <oal.h>
#include <am33x.h>

//------------------------------------------------------------------------------
//
//  Globals: s_oalSysIntr2Irq/s_oalIrq2SysIntr
//
static UINT32 s_oalSysIntr2Irq[SYSINTR_MAXIMUM][AM33X_IRQ_PER_SYSINTR];
static UINT32 s_oalIrq2SysIntr[AM33X_IRQ_MAXIMUM];

//------------------------------------------------------------------------------
//
//  Function:  OALIntrMapInit
//
//  This function must be called from OALInterruptInit to initialize mapping
//  between IRQ and SYSINTR. It simply initialize mapping arrays.
//
VOID OALIntrMapInit()
{

    UINT32 i, j;

    OALMSG(OAL_FUNC&&OAL_INTR, (L"+OEMInterruptMapInit\r\n"));

    // Initialize interrupt maps
    for (i = 0; i < SYSINTR_MAXIMUM; i++)
        {
        for (j = 0; j < AM33X_IRQ_PER_SYSINTR; j++)
            {
            s_oalSysIntr2Irq[i][j] = (UINT32)OAL_INTR_IRQ_UNDEFINED;
            }
        }
    for (i = 0; i < AM33X_IRQ_MAXIMUM; i++)
        {
        s_oalIrq2SysIntr[i] = (UINT32)SYSINTR_UNDEFINED;
        }

    OALMSG(OAL_FUNC&&OAL_INTR, (L"-OEMInterruptMapInit\r\n"));

}


//------------------------------------------------------------------------------
//
//  Function:   OALIntrStaticTranslate
//
//  This function sets static translation between IRQ and SYSINTR.
//
VOID
OALIntrStaticTranslate(
    UINT32 sysIntr, 
    UINT32 irq
    )
{

    UINT32 i;

    OALMSG(OAL_FUNC&&OAL_INTR, (
        L"+OALIntrStaticTranslate(%d, %d)\r\n", sysIntr, irq
        ));
    if ((irq >= AM33X_IRQ_MAXIMUM) || (sysIntr >= SYSINTR_MAXIMUM))
        goto cleanUp;

    for (i = 0; i < AM33X_IRQ_PER_SYSINTR; i++)
        {
        if (s_oalSysIntr2Irq[sysIntr][i] != OAL_INTR_IRQ_UNDEFINED) continue;
        s_oalSysIntr2Irq[sysIntr][i] = irq;
        s_oalIrq2SysIntr[irq] = sysIntr;
        break;
        }

cleanUp:
    OALMSG(OAL_FUNC&&OAL_INTR, (L"-OALIntrStaticTranslate\r\n"));

}


//------------------------------------------------------------------------------
//
//  Function:  OALIntrTranslateSysIntr
//
//  This function maps a SYSINTR to its corresponding IRQ. It is typically used
//  in OEMInterruptXXX to obtain IRQs for given SYSINTR.
//
BOOL
OALIntrTranslateSysIntr(
    UINT32 sysIntr,
    UINT32 *pCount,
    const UINT32 **ppIrqs
    )
{
    BOOL rc = FALSE;


    OALMSG(OAL_FUNC&&OAL_INTR, (L"+OALTranslateSysIntr(%d)\r\n", sysIntr));

    // Valid SYSINTR?
    if (sysIntr >= SYSINTR_MAXIMUM)
        {
        rc = FALSE;
        goto cleanUp;
        }
    
    *pCount = AM33X_IRQ_PER_SYSINTR;
    *ppIrqs = s_oalSysIntr2Irq[sysIntr];
    rc = TRUE;

cleanUp:
    OALMSG(OAL_FUNC&&OAL_INTR, (L"-OALTranslateSysIntr(rc = %d)\r\n", rc));

    return rc;
}


//------------------------------------------------------------------------------
//
//  Function:  OALIntrTranslateIrq
//
//  This function maps a IRQ to its corresponding SYSINTR.
//
UINT32
OALIntrTranslateIrq(
    UINT32 irq
    )
{
    UINT32 sysIntr = (UINT32)SYSINTR_UNDEFINED;

    OALMSG(OAL_FUNC&&OAL_INTR, (L"+OALIntrTranslateIrq(%d)\r\n", irq));


    if (irq < AM33X_IRQ_MAXIMUM) sysIntr = s_oalIrq2SysIntr[irq];


    OALMSG(OAL_FUNC&&OAL_INTR, (
        L"-OALIntrTranslateIrq(sysIntr = %d)\r\n", sysIntr
        ));
    return sysIntr;
}


//------------------------------------------------------------------------------
//
//  Function:  OALIntrRequestSysIntr
//
//  This function allocate new SYSINTR for given IRQ and it there isn't
//  static mapping for this IRQ it will create it.
//
UINT32
OALIntrRequestSysIntr(
    UINT32 count, 
    const UINT32 *pIrqs, 
    UINT32 flags
    )
{

    UINT32 sysIntr, i, j;


    OALMSG(OAL_INTR&&OAL_FUNC, (
        L"+OALIntrRequestSysIntr(%d, 0x%08x, 0x%08x)\r\n", count, pIrqs, flags
        ));

    // Valid IRQ?
    if (count > AM33X_IRQ_PER_SYSINTR)
        {
        OALMSG(OAL_ERROR, (L"ERROR: OALIntrRequestSysIntr: "
            L"Only %d IRQs for SYSINTR is supported\r\n",
            AM33X_IRQ_PER_SYSINTR
            ));
        sysIntr = (UINT32)SYSINTR_UNDEFINED;
        goto cleanUp;
        }

    // If static mapping is requested check if we can obtain one
    if ((flags & OAL_INTR_STATIC) != 0)
        {
        for (i = 0; i < count; i++)
            {
            if (pIrqs[i] >= AM33X_IRQ_MAXIMUM)
                {
                OALMSG(OAL_ERROR, (L"ERROR: OALIntrRequestSysIntr: "
                    L"IRQ %d out of range\r\n", pIrqs[i]
                    ));
                sysIntr = (UINT32)SYSINTR_UNDEFINED;
                goto cleanUp;
                }
            if (s_oalIrq2SysIntr[pIrqs[i]] != SYSINTR_UNDEFINED)
                {
                OALMSG(OAL_ERROR, (L"ERROR: OALIntrRequestSysIntr: "
                    L"Static mapping for IRQ %d already assigned\r\n", pIrqs[i]
                    ));
                sysIntr = (UINT32)SYSINTR_UNDEFINED;
                goto cleanUp;
                }
            }
        }

    // If translate flag is set all IRQs must be mapped to same SYSINTR
    if (pIrqs[0] >= AM33X_IRQ_MAXIMUM)
        {
        OALMSG(OAL_ERROR, (L"ERROR: OALIntrRequestSysIntr: "
            L"IRQ %d out of range\r\n", pIrqs[0]
            ));
        sysIntr = (UINT32)SYSINTR_UNDEFINED;
        goto cleanUp;
        }
    if (((flags & OAL_INTR_TRANSLATE) != 0) &&
        (s_oalIrq2SysIntr[pIrqs[0]] != SYSINTR_UNDEFINED))
        {
        sysIntr = s_oalIrq2SysIntr[pIrqs[0]];
        // Check remaining IRQs
        for (i = 1; i < count; i++)
            {
            if (pIrqs[i] >= AM33X_IRQ_MAXIMUM)
                {
                OALMSG(OAL_ERROR, (L"ERROR: OALIntrRequestSysIntr: "
                    L"IRQ %d out of range\r\n", pIrqs[i]
                    ));
                sysIntr = (UINT32)SYSINTR_UNDEFINED;
                goto cleanUp;
                }
            // If it is mapped to same SYSINTR, we are fine
            if (s_oalIrq2SysIntr[pIrqs[i]] == sysIntr) continue;
            // It isn't mapped or mapped to some other SYSINTR, error...
            sysIntr = (UINT32)SYSINTR_UNDEFINED;
            }
        goto cleanUp;
        }

    // Find next available SYSINTR value...
    for (sysIntr = SYSINTR_FIRMWARE; sysIntr < SYSINTR_MAXIMUM; sysIntr++)
        {
        if (s_oalSysIntr2Irq[sysIntr][0] == OAL_INTR_IRQ_UNDEFINED) break;
        }

    // Any available SYSINTRs left?
    if (sysIntr >= SYSINTR_MAXIMUM)
        {
        OALMSG(OAL_ERROR, (L"ERROR: OALIntrRequestSysIntr: "
            L"No available SYSINTR found\r\n"
            ));
        sysIntr = (UINT32)SYSINTR_UNDEFINED;
        goto cleanUp;
        }

    // Make SYSINTR -> IRQs association
    for (i = 0; i < count; i++)
        {
        if (pIrqs[i] >= AM33X_IRQ_MAXIMUM)
            {
            OALMSG(OAL_ERROR, (L"ERROR: OALIntrRequestSysIntr: "
                L"IRQ %d out of range\r\n", pIrqs[i]
                ));
            for (j = 0; j < i; j++)
                {
                s_oalSysIntr2Irq[sysIntr][j] = (UINT32)OAL_INTR_IRQ_UNDEFINED;
                }
            sysIntr = (UINT32)SYSINTR_UNDEFINED;
            goto cleanUp;
            }
        s_oalSysIntr2Irq[sysIntr][i] = pIrqs[i];
        }
    while (i < AM33X_IRQ_PER_SYSINTR)
        {
        s_oalSysIntr2Irq[sysIntr][i++] = (UINT32)OAL_INTR_IRQ_UNDEFINED;
        }

    // Make IRQ -> SYSINTR association, only if not multiply-mapped
    if ((flags & OAL_INTR_DYNAMIC) != 0) goto cleanUp;
    for (i = 0; i < count; i++)
        {
        if ((s_oalIrq2SysIntr[pIrqs[i]] == SYSINTR_UNDEFINED) ||
            ((flags & OAL_INTR_FORCE_STATIC) != 0))
            {
            s_oalIrq2SysIntr[pIrqs[i]] = sysIntr;
            }
        }

cleanUp:
    
    OALMSG(OAL_INTR&&OAL_FUNC, (
        L"-OALIntrRequestSysIntr(sysIntr = %d)\r\n", sysIntr
        ));
    return sysIntr;

}


//------------------------------------------------------------------------------
//
//  Function:  OALIntrReleaseSysIntr
//
//  This function release given SYSINTR and remove static mapping if exists.
//
BOOL
OALIntrReleaseSysIntr(
    UINT32 sysIntr
    )
{
    BOOL rc = TRUE;

    UINT32 i, irq;

    OALMSG(OAL_INTR&&OAL_FUNC, (L"+OALIntrReleaseSysIntr(%d)\r\n", sysIntr));

    if ((sysIntr < SYSINTR_FIRMWARE) || (sysIntr >= SYSINTR_MAXIMUM))
        {
        OALMSG(OAL_ERROR, (L"ERROR: OALIntrReleaseSysIntr: "
            L"Invalid sysIntr value %d\r\n", sysIntr
            ));
        rc = FALSE;
        goto cleanUp;
        }

        
    // Release all mapping for given SYSINTR
    for (i = 0; i < AM33X_IRQ_PER_SYSINTR; i++)
        {
        if (s_oalSysIntr2Irq[sysIntr][i] == OAL_INTR_IRQ_UNDEFINED) continue;
        // Remove the SYSINTR -> IRQ mapping
        irq = s_oalSysIntr2Irq[sysIntr][i];
        s_oalSysIntr2Irq[sysIntr][i] = (UINT32)OAL_INTR_IRQ_UNDEFINED;
        // If there is IRQ -> SYSINTR for this IRQ release it also
        if (irq >= AM33X_IRQ_MAXIMUM) 
            {
            rc = FALSE;
            continue;
            }
        if (s_oalIrq2SysIntr[irq] == sysIntr)
            {
            s_oalIrq2SysIntr[irq] = (UINT32)SYSINTR_UNDEFINED;
            }
        }

cleanUp:
    OALMSG(OAL_INTR&&OAL_FUNC, (L"-OALIntrReleaseSysIntr(rc = %d)\r\n", rc));

    return rc;
}

//------------------------------------------------------------------------------

