// All rights reserved ADENEO EMBEDDED 2010
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
#include "omap.h"
#include <windows.h>
#include <ceddk.h>
#include <ceddkex.h>
#include <oal.h>
#include <oalex.h>
#include <omap35xxBusContext.h>
#include <omap35xxBus.h>
#include <omap_bus.h>
#include <omap_dvfs.h>


//------------------------------------------------------------------------------
//
//  Constructor:  omap35xxBusContext_t
//
omap35xxBusContext_t::
omap35xxBusContext_t(
    omap35xxBus_t *pBus
    ) : DefaultBusContext_t(pBus)
{
}

// UNDONE:
//  contexts for display and TVOUT don't have a (bus, child) pairing which
// will cause dma and system activity monitor notifications to fail for
// these drivers
//

//------------------------------------------------------------------------------
//
//  Method:  DefaultBusContext_t::Close
//
//  Called from XXX_IOControl
//
BOOL
omap35xxBusContext_t::
IOControl(
    DWORD code, 
    UCHAR *pInBuffer, 
    DWORD inSize, 
    UCHAR *pOutBuffer,
    DWORD outSize, 
    DWORD *pOutSize
    )
{
    BOOL rc = FALSE;
    CE_BUS_DEVICE_SOURCE_CLOCKS *pClockInfo;
    CE_BUS_DEVICESTATE_CALLBACKS *pDeviceStateCallbacks;
    omap35xxBus_t* pBus = reinterpret_cast<omap35xxBus_t*>(m_pBus);

    switch (code)
        {
        case IOCTL_BUS_REQUEST_CLOCK:
            if ((pInBuffer == NULL) || (inSize < sizeof(OMAP_DEVICE)))
                {
                SetLastError(ERROR_INVALID_PARAMETER);
                break;
                }
            rc = pBus->SetPowerState(
                *reinterpret_cast<UINT*>(pInBuffer), D0
                );
            break;       
            
        case IOCTL_BUS_RELEASE_CLOCK:
            if ((pInBuffer == NULL) || (inSize < sizeof(OMAP_DEVICE)))
                {
                SetLastError(ERROR_INVALID_PARAMETER);
                break;
                }
            rc = pBus->SetPowerState(
                *reinterpret_cast<UINT*>(pInBuffer), D4
                );
            break;            
            
        case IOCTL_BUS_CHILD_BIND:
            if ((pInBuffer == NULL) || (inSize < sizeof(OMAP_BIND_CHILD_INFO)))
                {
                SetLastError(ERROR_INVALID_PARAMETER);
                break;
                }
            // check if this is already binded to a child.  If so then
            // refuse request.
            if (m_pChild != NULL) break;            
            m_pChild = pBus->OpenChildBusToBind(
                reinterpret_cast<OMAP_BIND_CHILD_INFO*>(pInBuffer)
                );
            rc = TRUE;
            break;
            
        case IOCTL_BUS_CHILD_UNBIND:            
            // check if this is already binded to a child.  If so then
            // refuse request.
            if (m_pChild == NULL) break;            
            rc = pBus->CloseChildBusFromBind(m_pChild);
            m_pChild->Release();
            m_pChild = NULL;
            break;

        case IOCTL_BUS_SOURCE_CLOCKS:
            if ((pInBuffer == NULL) || (inSize < sizeof(CE_BUS_DEVICE_SOURCE_CLOCKS)))
                {
                SetLastError(ERROR_INVALID_PARAMETER);
                break;
                }
            pClockInfo = (CE_BUS_DEVICE_SOURCE_CLOCKS*)pInBuffer;
            rc = pBus->SetSourceDeviceClocks(pClockInfo->devId,
                    pClockInfo->count, pClockInfo->rgSourceClocks
                    );
            break;

        case IOCTL_BUS_DEVICESTATE_CALLBACKS:
            if ((pInBuffer == NULL) || (inSize < sizeof(CE_BUS_DEVICESTATE_CALLBACKS)))
                {
                SetLastError(ERROR_INVALID_PARAMETER);
                break;
                }

            pDeviceStateCallbacks = (CE_BUS_DEVICESTATE_CALLBACKS*)pInBuffer;
            rc = pBus->SetDevicePowerStateChangeCallbacks(
                        pDeviceStateCallbacks->PreDeviceStateChange, 
                        pDeviceStateCallbacks->PostDeviceStateChange
                        );
            break;
        
        default:
            rc = DefaultBusContext_t::IOControl(
                code, pInBuffer, inSize, pOutBuffer, outSize, pOutSize
                );
        }
    
    return rc;
}
       
//------------------------------------------------------------------------------

