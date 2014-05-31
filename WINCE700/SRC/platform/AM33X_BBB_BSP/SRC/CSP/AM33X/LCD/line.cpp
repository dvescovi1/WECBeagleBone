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
//  Module Name:    line.cpp
//
// -----------------------------------------------------------------------------


#include "precomp.h"

#include <dispperf.h>

SCODE LCDDDGPE::Line(GPELineParms *lineParameters, EGPEPhase phase)
{
    DEBUGMSG (GPE_ZONE_INIT, (TEXT("LCDDDGPE::Line\r\n")));

    if (phase == gpeSingle || phase == gpePrepare)
    {
        DispPerfStart(ROP_LINE);

        if ((lineParameters->pDst != m_pPrimarySurface))
        {
            lineParameters->pLine = &GPE::EmulatedLine;
        }
        else
        {
            lineParameters->pLine = (SCODE (GPE::*)(struct GPELineParms *)) &LCDDDGPE::WrappedEmulatedLine;
        }
    }
    else if (phase == gpeComplete)
    {
        DispPerfEnd(0);
    }
    return S_OK;
}

SCODE
LCDDDGPE::WrappedEmulatedLine(
    GPELineParms *lineParameters
    )
{
    SCODE retval;
    RECT  bounds;
    int   N_plus_1;                // Minor length of bounding rect + 1

    // calculate the bounding-rect to determine overlap with cursor
    if (lineParameters->dN)            // The line has a diagonal component (we'll refresh the bounding rect)
    {
        N_plus_1 = 2 + ((lineParameters->cPels * lineParameters->dN) / lineParameters->dM);
    }
    else
    {
        N_plus_1 = 1;
    }

    switch(lineParameters->iDir)
    {
        case 0:
            bounds.left = lineParameters->xStart;
            bounds.top = lineParameters->yStart;
            bounds.right = lineParameters->xStart + lineParameters->cPels + 1;
            bounds.bottom = bounds.top + N_plus_1;
            break;
        case 1:
            bounds.left = lineParameters->xStart;
            bounds.top = lineParameters->yStart;
            bounds.bottom = lineParameters->yStart + lineParameters->cPels + 1;
            bounds.right = bounds.left + N_plus_1;
            break;
        case 2:
            bounds.right = lineParameters->xStart + 1;
            bounds.top = lineParameters->yStart;
            bounds.bottom = lineParameters->yStart + lineParameters->cPels + 1;
            bounds.left = bounds.right - N_plus_1;
            break;
        case 3:
            bounds.right = lineParameters->xStart + 1;
            bounds.top = lineParameters->yStart;
            bounds.left = lineParameters->xStart - lineParameters->cPels;
            bounds.bottom = bounds.top + N_plus_1;
            break;
        case 4:
            bounds.right = lineParameters->xStart + 1;
            bounds.bottom = lineParameters->yStart + 1;
            bounds.left = lineParameters->xStart - lineParameters->cPels;
            bounds.top = bounds.bottom - N_plus_1;
            break;
        case 5:
            bounds.right = lineParameters->xStart + 1;
            bounds.bottom = lineParameters->yStart + 1;
            bounds.top = lineParameters->yStart - lineParameters->cPels;
            bounds.left = bounds.right - N_plus_1;
            break;
        case 6:
            bounds.left = lineParameters->xStart;
            bounds.bottom = lineParameters->yStart + 1;
            bounds.top = lineParameters->yStart - lineParameters->cPels;
            bounds.right = bounds.left + N_plus_1;
            break;
        case 7:
            bounds.left = lineParameters->xStart;
            bounds.bottom = lineParameters->yStart + 1;
            bounds.right = lineParameters->xStart + lineParameters->cPels + 1;
            bounds.top = bounds.bottom - N_plus_1;
            break;
        default:
            DEBUGMSG(GPE_ZONE_ERROR,(TEXT("Invalid direction: %d\r\n"), lineParameters->iDir));
            return E_INVALIDARG;
    }

    // check for line overlap with cursor and turn off cursor if overlaps
    RECTL cursorRect = m_CursorRect;
    RotateRectl (&cursorRect);

    if (m_CursorVisible && !m_CursorDisabled &&
        cursorRect.top < bounds.bottom && cursorRect.bottom > bounds.top &&
        cursorRect.left < bounds.right && cursorRect.right > bounds.left)
    {
        DisableCursor();
        m_CursorForcedOff = TRUE;
    }

    // do emulated line
    retval = EmulatedLine (lineParameters);

    // see if cursor was forced off because of overlap with line bounds and turn back on
    if (m_CursorForcedOff)
    {
        m_CursorForcedOff = FALSE;
        EnableCursor();
    }

    return    retval;

}


