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
#include <windows.h>
#include <DefaultBusContext.h>

//------------------------------------------------------------------------------
//
//  Constructor:  DefaultBusContext_t
//
DefaultBusContext_t::
DefaultBusContext_t(
    BusDriver_t *pBus
    ) : m_pBus(pBus), m_pChild(NULL)
{
    // We made copy of pointer
    m_pBus->AddRef();
}

//------------------------------------------------------------------------------
//
//  Destructor:  DefaultBusContext_t
//
DefaultBusContext_t::
~DefaultBusContext_t(
    )
{
    // If we have associated child device, release it
    if (m_pChild != NULL) m_pChild->Release();
    // We need to release reference
    m_pBus->Release();
}

//------------------------------------------------------------------------------
//
//  Method:  DefaultBusContext_t::Close
//
//  Called from XXX_Close
//
BOOL
DefaultBusContext_t::
Close(
    )
{
    return m_pBus->Close(this);
}

//------------------------------------------------------------------------------
//
//  Method:  DefaultBusContext_t::Close
//
//  Called from XXX_IOControl
//
BOOL
DefaultBusContext_t::
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

    switch (code)
        {
        
        case IOCTL_BUS_POSTINIT: 
            rc = m_pBus->IoCtlPostInit();
            break;

        case IOCTL_BUS_NAME_PREFIX:
            {
            DWORD size = m_pBus->IoCtlBusNamePrefix(
                reinterpret_cast<LPWSTR>(pOutBuffer),  outSize
                );
            if (pOutSize != NULL) *pOutSize = size;
            }
            break;
            
        case IOCTL_BUS_TRANSLATE_BUS_ADDRESS:
            if ((pInBuffer == NULL) || 
                (inSize < sizeof(CE_BUS_TRANSLATE_BUS_ADDR)))
                {
                SetLastError(ERROR_INVALID_PARAMETER);
                break;
                }
            {
            CE_BUS_TRANSLATE_BUS_ADDR busTrans;

            // Cast pInBuffer
            busTrans = *(CE_BUS_TRANSLATE_BUS_ADDR *)pInBuffer;

            // Get child to be called
            BusDriverChild_t* pChild = FindChildByName(
                busTrans.lpDeviceBusName
                );
            // If we can't find child by name, we get incorrect name
            if (pChild == NULL)
                {
                SetLastError(ERROR_INVALID_PARAMETER);
                break;
                }
            // Call child function
            rc = pChild->IoCtlTranslateBusAddress(
                busTrans.InterfaceType, busTrans.BusNumber, 
                busTrans.BusAddress, busTrans.AddressSpace, 
                busTrans.TranslatedAddress
                );
            }
            break;
            
        case IOCTL_BUS_TRANSLATE_SYSTEM_ADDRESS:
            if ((pInBuffer == NULL) || 
                (inSize < sizeof(CE_BUS_TRANSLATE_SYSTEM_ADDR)))
                {
                SetLastError(ERROR_INVALID_PARAMETER);
                break;
                }
            {
            CE_BUS_TRANSLATE_SYSTEM_ADDR busTrans;

            // Cast pInbuffer    
            busTrans = *(CE_BUS_TRANSLATE_SYSTEM_ADDR *)pInBuffer;

            // Get child to be called
            BusDriverChild_t* pChild = FindChildByName(
                busTrans.lpDeviceBusName
                );
            // If we can't find child by name, we get incorrect name
            if (pChild == NULL)
                {
                SetLastError(ERROR_INVALID_PARAMETER);
                break;
                }
            // Call child function
            rc = pChild->IoCtlTranslateSystemAddress(
                    busTrans.InterfaceType, busTrans.BusNumber, 
                    busTrans.SystemAddress, busTrans.TranslatedAddress
                    );
            }
            break;

        case IOCTL_BUS_GET_POWER_STATE:
        case IOCTL_BUS_SET_POWER_STATE:
            if ((pInBuffer == NULL) || 
                (inSize < sizeof(CE_BUS_POWER_STATE)))
                {
                SetLastError(ERROR_INVALID_PARAMETER);
                break;
                }
            {
            CE_BUS_POWER_STATE state;

            // Cast pInBuffer
            state = *(CE_BUS_POWER_STATE *)pInBuffer;

            // Get child to be called
            BusDriverChild_t* pChild = FindChildByName(
                state.lpDeviceBusName
                );
            // If we can't find child by name, we get incorrect name
            if (pChild == NULL)
                {
                SetLastError(ERROR_INVALID_PARAMETER);
                break;
                }
            // Call child function
            if (code == IOCTL_BUS_GET_POWER_STATE)
                {
                rc = pChild->IoCtlGetPowerState(state.lpceDevicePowerState);
                }
            else if (code == IOCTL_BUS_SET_POWER_STATE)
                {
                rc = pChild->IoCtlSetPowerState(state.lpceDevicePowerState);
                }
            }
            break;

        case IOCTL_BUS_GET_CONFIGURE_DATA:        
        case IOCTL_BUS_SET_CONFIGURE_DATA:
            if ((pInBuffer == NULL) || 
                (inSize < sizeof(CE_BUS_DEVICE_CONFIGURATION_DATA)))
                {
                SetLastError(ERROR_INVALID_PARAMETER);
                break;
                }
            {
            CE_BUS_DEVICE_CONFIGURATION_DATA data;

            // Cast pInBuffer
            data = *(CE_BUS_DEVICE_CONFIGURATION_DATA*)pInBuffer;

            // Get child to be called
            BusDriverChild_t* pChild = FindChildByName(data.lpDeviceBusName);
            // If we can't find child by name, we get incorrect name
            if (pChild == NULL)
                {
                SetLastError(ERROR_INVALID_PARAMETER);
                break;
                }
            // Call child function
            if (code == IOCTL_BUS_GET_CONFIGURE_DATA)
                {
                rc = pChild->IoCtlGetConfigurationData(
                    data.dwSpace, data.dwOffset, data.dwLength, data.pBuffer
                    );
                }
            else if (code == IOCTL_BUS_SET_CONFIGURE_DATA)
                {
                rc = pChild->IoCtlSetConfigurationData(
                    data.dwSpace, data.dwOffset, data.dwLength, data.pBuffer
                    );
                }
            }
            break;

        case IOCTL_BUS_ACTIVATE_CHILD:
        case IOCTL_BUS_DEACTIVATE_CHILD:
            if ((pInBuffer == NULL) || 
                (inSize < (wcslen((WCHAR*)pInBuffer) + 1) * sizeof(WCHAR)))
                {
                SetLastError(ERROR_INVALID_PARAMETER);
                break;
                }
            {
            // Get child to be called
            BusDriverChild_t* pChild = FindChildByName((WCHAR*)pInBuffer);

            // If we can't find child by name, we get incorrect name
            if (pChild == NULL)
                {
                SetLastError(ERROR_INVALID_PARAMETER);
                break;
                }
            // Call child function
            if (code == IOCTL_BUS_ACTIVATE_CHILD)
                {
                rc = pChild->IoCtlActivateChild();
                }
            else if (code == IOCTL_BUS_DEACTIVATE_CHILD)
                {
                rc = pChild->IoCtlDeactivateChild();
                }
            }
            break;

        case IOCTL_BUS_IS_CHILD_REMOVED:
            if (pOutSize != NULL) *pOutSize = sizeof(BOOL);
            if ((pInBuffer == NULL) || 
                (inSize < (wcslen((WCHAR*)pInBuffer) + 1) * sizeof(WCHAR)) ||
                (pOutBuffer == NULL) || (outSize < sizeof(BOOL)))
                {
                SetLastError(ERROR_INVALID_PARAMETER);
                break;
                }
            {
            // Get child to be called
            BusDriverChild_t* pChild = FindChildByName((WCHAR*)pInBuffer);

            // If we can't find child by name, we get incorrect name
            if (pChild == NULL)
                {
                SetLastError(ERROR_INVALID_PARAMETER);
                break;
                }
            // Call child function
            rc = pChild->IoCtlIsChildRemoved();
            }
            break;
        }
    
    return rc;
}
       
//------------------------------------------------------------------------------
//
//  Method:  DefaultBusContext_t::FindChildByName
//
//  This helper function finds IOCTL target child device. It first looks
//  if target device is one used in previous IOCTL calls from this open
//  context. If name doesn't match, it will ask bus driver to locate
//  child. If child is found, it is stored for later calls.
//
BusDriverChild_t* 
DefaultBusContext_t::
FindChildByName(
    LPCWSTR childName
    )
{
    BusDriverChild_t* pChild;

    if (m_pChild == NULL)
        {
        // We don't need add reference count as it is done by
        // FindChildByName method...
        pChild = m_pChild = m_pBus->FindChildByName(childName);
        }
    else
        {
        pChild = m_pChild;
        }

    return pChild;
}    

//------------------------------------------------------------------------------

