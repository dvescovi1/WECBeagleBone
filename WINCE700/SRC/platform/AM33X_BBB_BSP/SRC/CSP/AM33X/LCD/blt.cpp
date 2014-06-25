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
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES.
//
// -----------------------------------------------------------------------------
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Module Name:    blt.cpp
//
// -----------------------------------------------------------------------------


#include "precomp.h"

#include <dispperf.h>

SCODE LCDDDGPE::BltPrepare(GPEBltParms *blitParameters)
{
    RECTL rectl;
    int   iSwapTmp;

    DEBUGMSG (GPE_ZONE_INIT, (TEXT("LCDDDGPE::BltPrepare\r\n")));

    //  Display perf start
    DispPerfStart(blitParameters->rop4);

    // default to base EmulatedBlt routine
    blitParameters->pBlt = &GPE::EmulatedBlt;

    // see if we need to deal with cursor

    // check for destination overlap with cursor and turn off cursor if overlaps
    if (blitParameters->pDst == m_pPrimarySurface)    // only care if dest is main display surface
    {
        if (m_CursorVisible && !m_CursorDisabled)
        {
            if (blitParameters->prclDst != NULL)        // make sure there is a valid prclDst
            {
                rectl = *blitParameters->prclDst;        // if so, use it

                // There is no guarantee of a well ordered rect in blitParamters
                // due to flipping and mirroring.
                if(rectl.top > rectl.bottom)
                {
                    iSwapTmp     = rectl.top;
                    rectl.top    = rectl.bottom;
                    rectl.bottom = iSwapTmp;
                }

                if(rectl.left > rectl.right)
                {
                    iSwapTmp    = rectl.left;
                    rectl.left  = rectl.right;
                    rectl.right = iSwapTmp;
                }
            }
            else
            {
                rectl = m_CursorRect;                    // if not, use the Cursor rect - this forces the cursor to be turned off in this case
            }

            if (m_CursorRect.top <= rectl.bottom && m_CursorRect.bottom >= rectl.top &&
                m_CursorRect.left <= rectl.right && m_CursorRect.right >= rectl.left)
            {
                DisableCursor();
                m_CursorForcedOff = TRUE;
            }
        }
    }

    // check for source overlap with cursor and turn off cursor if overlaps
    if (blitParameters->pSrc == m_pPrimarySurface)    // only care if source is main display surface
    {
        if (m_CursorVisible && !m_CursorDisabled)
        {
            if (blitParameters->prclSrc != NULL)        // make sure there is a valid prclSrc
            {
                rectl = *blitParameters->prclSrc;        // if so, use it
            }
            else
            {
                rectl = m_CursorRect;                    // if not, use the CUrsor rect - this forces the cursor to be turned off in this case
            }
            if (m_CursorRect.top < rectl.bottom && m_CursorRect.bottom > rectl.top &&
                m_CursorRect.left < rectl.right && m_CursorRect.right > rectl.left)
            {
                DisableCursor();
                m_CursorForcedOff = TRUE;
            }
        }
    }

    return S_OK;
}

SCODE LCDDDGPE::BltComplete(GPEBltParms *blitParameters)
{
    DEBUGMSG (GPE_ZONE_INIT, (TEXT("LCDDDGPE::BltComplete\r\n")));

    // see if cursor was forced off because of overlap with source or destination and turn back on
    if (m_CursorForcedOff)
    {
        m_CursorForcedOff = FALSE;
        EnableCursor();
    }

    DispPerfEnd(0);

    return S_OK;
}
