//-------------------------------------------------------------------------
// <copyright file="usbcode.cpp" company="Microsoft">
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//    The use and distribution terms for this software are covered by the
//    Microsoft Limited Permissive License (Ms-LPL) 
//    which can be found in the file MS-LPL.txt at the root of this distribution.
//    By using this software in any fashion, you are agreeing to be bound by
//    the terms of this license.
//
//    THE SOFTWARE IS LICENSED "AS-IS" WITH NO WARRANTIES OR INDEMNITIES. 
//
//    You must not remove this notice, or any other, from this software.
// </copyright>
// 
// <summary>
//    USB camera driver for Windows Embedded CE 6.0
// </summary>
//-------------------------------------------------------------------------
//======================================================================
// USB camera driver for Windows Embedded CE 6.0
//======================================================================


#include <windows.h>
#include <USBdi.h>                  // USB includes
#include <usb100.h>                 // USB includes
#include <usbclient.h>              // USB client driver helper code

#include "USBCode.h"                // USB Video Specification defs
#include "dbgsettings.h"


static void FreeAllResources(PDRVCONTEXT pDrv)
{
    if (pDrv && pDrv->pStreamInfo )
    {
        // Free Control Interface extended descriptor
        if (pDrv->pStreamInfo->usbctlIF.lpepExtDesc) 
        {
            LocalFree(pDrv->pStreamInfo->usbctlIF.lpepExtDesc);   
            pDrv->pStreamInfo->usbctlIF.lpepExtDesc = NULL;
        }
        

        // Free Stream Interface extended descriptor(s)
        if (pDrv->pStreamInfo->usbstrmIF) 
        {
            for (int i = 0; i < pDrv->pStreamInfo->nStreamInterfaces; i++)
            {
                LocalFree( pDrv->pStreamInfo->usbstrmIF[i].lpepExtDesc );
                pDrv->pStreamInfo->usbstrmIF[i].lpepExtDesc = NULL;
            }

            LocalFree(pDrv->pStreamInfo->usbstrmIF);
            pDrv->pStreamInfo->usbstrmIF = NULL;
        }

        LocalFree(pDrv->pStreamInfo);
        pDrv->pStreamInfo = NULL;
    }
    
    if (pDrv != NULL)
    {
        LocalFree(pDrv);
        pDrv = NULL;
    }
}


// USBDeviceAttach - Called when Host controller wants to load the driver
// 
BOOL USBDeviceAttach (USB_HANDLE hDevice, LPCUSB_FUNCS lpUsbFuncs,
                      LPCUSB_INTERFACE lpInterface, LPCWSTR szUniqueDriverId,
                      LPBOOL fAcceptControl,
                      LPCUSB_DRIVER_SETTINGS lpDriverSettings, DWORD dwUnused)
{
    WCHAR wsSubClassRegKey[] = CLIENT_REGKEY_SZ;
    DWORD dwStreamInfo;
    BOOL  rval;

    DEBUGMSG (ZONE_INIT, (DTAG TEXT("USBDeviceAttach++\r\n")));

    // Default is that we won't accept this device
    *fAcceptControl = FALSE;
    
    LPCUSB_DEVICE lpUsbDev = (lpUsbFuncs->lpGetDeviceInfo)(hDevice);
    if (!lpUsbDev || !lpInterface) {
        // Not a valid USB device (shouldn't happen)
        rval = TRUE;
        goto Exit;
    }


    // See if this device has the interface we need...
    // Only accept video control interface or vendor-specific (some older cameras)
    switch(lpInterface->Descriptor.bInterfaceClass) {
        case USB_DEVICE_CLASS_VIDEO:
            DEBUGMSG(ZONE_DEVICE, (DTAG TEXT("Detected USB_DEVICE_CLASS_VIDEO\r\n")));
            break;
        case USB_DEVICE_CLASS_VENDOR_SPECIFIC:
            DEBUGMSG(ZONE_DEVICE, (DTAG TEXT("Detected USB_DEVICE_CLASS_VENDOR_SPECIFIC\r\n")));
            DEBUGMSG(ZONE_WARN,   (DTAG TEXT("*** Device appears to pre-date final Video Class standard, may not be fully supported by this driver! ***\r\n")));
            break;
        default:
            // Not a video class device
            rval = TRUE;   
            goto Exit;
    }

    if(lpInterface->Descriptor.bInterfaceSubClass == 0x01)
    {
        int rc = ExtractInfo (hDevice, lpUsbFuncs, lpInterface, szUniqueDriverId,  lpDriverSettings, &dwStreamInfo);
        if(rc == INF_REJECTED) {
            rval = FALSE;
            goto Exit;
        }

        LPCUSB_DEVICE_DESCRIPTOR lpDeviceDesc = &lpUsbDev->Descriptor;
        DEBUGMSG(ZONE_DEVICE, (DTAG TEXT("\r\n")));
        DEBUGMSG(ZONE_DEVICE, (DTAG TEXT("-----USB Video Device-------\r\n")));
        DEBUGMSG(ZONE_DEVICE, (DTAG TEXT("bDeviceClass       = 0x%X\r\n"), lpDeviceDesc->bDeviceClass));
        DEBUGMSG(ZONE_DEVICE, (DTAG TEXT("bDeviceSubClass    = 0x%X\r\n"), lpDeviceDesc->bDeviceSubClass));
        DEBUGMSG(ZONE_DEVICE, (DTAG TEXT("bDeviceProtocol    = 0x%X\r\n"), lpDeviceDesc->bDeviceProtocol));
        DEBUGMSG(ZONE_DEVICE, (DTAG TEXT("bMaxPacketSize0    = 0x%X\r\n"), lpDeviceDesc->bMaxPacketSize0));
        DEBUGMSG(ZONE_DEVICE, (DTAG TEXT("idVendor           = 0x%X\r\n"), lpDeviceDesc->idVendor));
        DEBUGMSG(ZONE_DEVICE, (DTAG TEXT("idProduct          = 0x%X\r\n"), lpDeviceDesc->idProduct));
        DEBUGMSG(ZONE_DEVICE, (DTAG TEXT("bcdDevice          = 0x%X\r\n"), lpDeviceDesc->bcdDevice));
        DEBUGMSG(ZONE_DEVICE, (DTAG TEXT("bNumConfigurations = 0x%X\r\n"), lpDeviceDesc->bNumConfigurations));
        DEBUGMSG(ZONE_DEVICE, (DTAG TEXT("----------------------------\r\n")));

        LPCUSB_INTERFACE_DESCRIPTOR lpInterfaceDesc = &lpInterface->Descriptor;
        DEBUGMSG(ZONE_DEVICE, (DTAG TEXT("\r\n")));
        DEBUGMSG(ZONE_DEVICE, (DTAG TEXT("---USB Video Ctrl Interface---\r\n")));
        DEBUGMSG(ZONE_DEVICE, (DTAG TEXT("bInterfaceClass    = 0x%X\r\n"), lpInterfaceDesc->bInterfaceClass));
        DEBUGMSG(ZONE_DEVICE, (DTAG TEXT("bInterfaceSubClass = 0x%X\r\n"), lpInterfaceDesc->bInterfaceSubClass));
        DEBUGMSG(ZONE_DEVICE, (DTAG TEXT("bInterfaceProtocol = 0x%X\r\n"), lpInterfaceDesc->bInterfaceProtocol));
        DEBUGMSG(ZONE_DEVICE, (DTAG TEXT("------------------------------\r\n")));


        // Create and initialize the driver instance structure.  
        PDRVCONTEXT pDrv = (PDRVCONTEXT)LocalAlloc (LPTR, sizeof (DRVCONTEXT));
        if (pDrv == 0)
        { 
            SetLastError (ERROR_NOT_ENOUGH_MEMORY);
            rval = FALSE;
            goto Exit;
        }

        pDrv->dwSize = sizeof (DRVCONTEXT);
        pDrv->hDevice = hDevice;
        pDrv->lpUsbFuncs = lpUsbFuncs;
        InitializeCriticalSection (&pDrv->csDCall);
        pDrv->hUnload = CreateEvent (NULL, FALSE, FALSE, NULL);
        pDrv->fUnloadFlag = FALSE;
        memcpy( &pDrv->usbDeviceInfo, lpUsbDev, sizeof(USB_DEVICE) );

        // This structure contains the stream interface information collected from the USB context
        pDrv->pStreamInfo = (PSTREAMINFO)dwStreamInfo;


        // The Stream Driver (MDD and then the PDD) is activated here 
        pDrv->hStreamDevice = ActivateDevice (wsSubClassRegKey, (DWORD)pDrv);

        if (pDrv->hStreamDevice) 
        {
            // register for USB callbacks
            if (lpUsbFuncs->lpRegisterNotificationRoutine (hDevice, USBDeviceNotificationCallback, pDrv))
            {
                *fAcceptControl = TRUE;
            } 
        } 
        else
        {
            DEBUGMSG (ZONE_ERROR, (DTAG TEXT("Can't activate stream device! rc=%d\r\n"), GetLastError()));
            FreeAllResources(pDrv);
            rval = FALSE;
            goto Exit;
        }  

        rval = TRUE;
    }
    else
    {
        *fAcceptControl = TRUE;
        rval = TRUE;
    }

Exit:
    DEBUGMSG (ZONE_INIT, (DTAG TEXT("USBDeviceAttach--\r\n")));
    return rval;
}


BOOL USBDeviceNotificationCallback (LPVOID lpvNotifyParameter, DWORD dwCode,
                                    LPDWORD* dwInfo1, LPDWORD* dwInfo2,
                                    LPDWORD* dwInfo3, LPDWORD* dwInfo4)
{
    if (dwCode == USB_CLOSE_DEVICE)
    {
        // Get the pointer to our device context.
        PDRVCONTEXT pDrv = (PDRVCONTEXT) lpvNotifyParameter;


        // Set flags and handles to notify stream driver its about to unload.
        pDrv->fUnloadFlag = TRUE;
        SetEvent (pDrv->hUnload);
        Sleep (0);

        // Unload the stream driver
        DeactivateDevice (pDrv->hStreamDevice);

        Sleep (1000);
        FreeAllResources(pDrv);
        return TRUE;
    }
    
    return FALSE;
}


// This routine checks to see if the device supports required interfaces. 
// Extracts the stream interface exposed by the USB context and stores in 
// StreamInfo struct for the PDD to use for device communication 

int ExtractInfo (USB_HANDLE hDevice, LPCUSB_FUNCS lpUsbFuncs,
                  LPCUSB_INTERFACE lpInterface, LPCWSTR szUniqueDriverId,
                  LPCUSB_DRIVER_SETTINGS lpDriverSettings, DWORD *pdwStreamInfo)
{
    int rc = 0;

    // Call the device to query all interfaces
    LPCUSB_DEVICE lpUsbDev = (lpUsbFuncs->lpGetDeviceInfo)(hDevice);
    if (!lpUsbDev)
    {
        // Not a valid USB device (shouldn't happen)
        return INF_REJECTED;
    }


    // See if this device has the interface we need...
    if ((DWORD)lpInterface == 0)
    {
        return INF_REJECTED;
    }

    
    PSTREAMINFO pInfo = (PSTREAMINFO)LocalAlloc (LPTR, sizeof (STREAMINFO)); 
    if (pInfo == NULL)
    {
        SetLastError (ERROR_NOT_ENOUGH_MEMORY);
        return INF_REJECTED;
    }
    
    pInfo->dwSize = sizeof (STREAMINFO);
    // See if we can find the proper interfaces.  First, look for the
    // standard Video Class Device interfaces...
    if (!ParseStreamInterfaces (pInfo, lpUsbDev, 0x0e, 1, 0x0e, 2))
    {
        // If failed to open standard video class interface, try vendor specific
        
        // Older Logitech Quick Cam Pro 5000 firmware:
        if (!ParseStreamInterfaces (pInfo, lpUsbDev, 0xff, 1, 0xff, 2))
        {   
            if (pInfo && pInfo->usbstrmIF)
            {
                LocalFree(pInfo->usbstrmIF);
                pInfo->usbstrmIF = NULL;
            }
            if (pInfo)
            {
                LocalFree (pInfo);
                pInfo = NULL;
            }
            
            return INF_REJECTED;
        } 
    }
    
    DEBUGMSG(ZONE_INIT, (DTAG TEXT("Video Class interface found.\n"), rc)); 

    rc = INF_ACCEPTED;  
    *pdwStreamInfo = (DWORD)pInfo;
    
    return rc;
}


//-----------------------------------------------------------------------
// ParseStreamInterfaces - Walks the camera's USB interface descriptors
// to determine the supported features of the camera.
//
BOOL ParseStreamInterfaces (PSTREAMINFO pInfo, LPCUSB_DEVICE lpUsbDev, BYTE bIFCtl,
                            BYTE bIFSubCtl, BYTE bIFStrm, BYTE bIFSubStrm)
{
    DEBUGMSG (ZONE_INIT, (DTAG TEXT("ParseStreamInterfaces++\r\n")));
    bool foundcntrl = false;

    _try {
        if (lpUsbDev->lpConfigs->dwCount != sizeof (USB_CONFIGURATION))
        {
            DEBUGMSG (ZONE_ERROR, (DTAG TEXT("Unexpected CUSB_CONFIGURATION dwCount: %d  ***********\r\n"), lpUsbDev->lpConfigs->dwCount));
            return FALSE;
        }

        LPCUSB_INTERFACE lpIF;
        int nStreams = 0;
        int nStreamAlts = 0;
        int nOtherStreams = 0;
        int nStreamIdx = 0;
        int nStreamID = -1;

        // Walk interface table to find the number of Video stream interface alternates and if there are 
        // other video streams that aren't alts.
        for (int i = 0; i < (int)lpUsbDev->lpConfigs->dwNumInterfaces; i++)
        {
            lpIF = &lpUsbDev->lpConfigs->lpInterfaces[i];

            if ((lpIF->Descriptor.bInterfaceClass == bIFStrm) && (lpIF->Descriptor.bInterfaceSubClass == bIFSubStrm))
            {
                // for the first stream
                if (nStreamID == -1) 
                {
                    nStreamID = lpIF->Descriptor.bInterfaceNumber;
                }
                else if (nStreamID == lpIF->Descriptor.bInterfaceNumber)
                {
                    nStreamAlts++;
                }
                else
                {
                    nOtherStreams++;
                }
            }
        }
        
        if (nStreamID == -1)
        {
            DEBUGMSG (ZONE_ERROR, (DTAG TEXT("No stream interfaces\r\n")));
            return FALSE;
        }
        
        
        nStreams = nStreamAlts + 1;
        DEBUGMSG (ZONE_INIT, (DTAG TEXT("Found %d stream interface(s)\r\n"), nStreams));
        if (nOtherStreams)
        {
            DEBUGMSG (ZONE_WARN, (DTAG TEXT("Multiple stream interfaces found on device. Using first one.\r\n")));
        }

        // Allocate the array for the stream interface descriptors
        if (nStreams)
        {
            pInfo->usbstrmIF = (PUSBSTRMIF)LocalAlloc (LPTR, nStreams*sizeof(USBSTRMIF));
        }
        
        if(pInfo->usbstrmIF == 0)
            return FALSE;
        
        for (int i = 0; i < (int)lpUsbDev->lpConfigs->dwNumInterfaces; i++)
        {
            lpIF = &lpUsbDev->lpConfigs->lpInterfaces[i];
            
            // See if this is the video control interface
            if ((lpIF->Descriptor.bInterfaceClass == bIFCtl) && (lpIF->Descriptor.bInterfaceSubClass == bIFSubCtl))
            {
                if (lpIF->lpvExtended)
                {
                    // Cast the extended ptr as a video control extended header ptr
                    const PUSBVIDCTLIFDESCRIPTOR lpExDesc = (PUSBVIDCTLIFDESCRIPTOR)lpIF->lpvExtended;
                    if(lpExDesc->bType == USB_VIDEO_CS_INTERFACE)
                    {
                        // Copy descriptor info
                        pInfo->usbctlIF.ifDesc = lpIF->Descriptor;
                        foundcntrl = TRUE;
                    }
                    else 
                    {
                        continue;
                    }
                        
                    // Verify the header length
                    if (lpExDesc->bLen > 0x0a)
                    {
                        // Allocate the space for the descriptor
                        pInfo->usbctlIF.lpepExtDesc = LocalAlloc (LPTR, lpExDesc->wTotalLen);
                        if (pInfo->usbctlIF.lpepExtDesc)
                        {
                            // Copy over the extended descriptor
                            pInfo->usbctlIF.wExtDescSize = lpExDesc->wTotalLen;
                            memcpy (pInfo->usbctlIF.lpepExtDesc, lpExDesc, lpExDesc->wTotalLen); 
                        } 
                        else
                        {
                            DEBUGMSG (ZONE_ERROR, (DTAG TEXT("Out of memory allocating extended if descriptor\r\n")));
                            return FALSE;
                        }
                    }
                    else
                    {
                            DEBUGMSG (ZONE_ERROR | ZONE_INIT, (DTAG TEXT("Unexpected extended Ctl header format")));
                            return FALSE;
                    }
                } 
                else
                {
                    // no extention desc found
                    pInfo->usbctlIF.lpifExtDesc = 0;
                }

                // Grab the interrupt end point if specified
                if (lpIF->lpEndpoints)
                {
                    pInfo->usbctlIF.epDesc = lpIF->lpEndpoints->Descriptor;
                    pInfo->usbctlIF.fEndpoint = TRUE;
                }
                else
                {
                    pInfo->usbctlIF.fEndpoint = FALSE;
                }
            }
            
            // See if this is the video stream interface
            if ((lpIF->Descriptor.bInterfaceClass == bIFStrm) && 
                    (lpIF->Descriptor.bInterfaceSubClass == bIFSubStrm) &&
                    (lpIF->Descriptor.bInterfaceNumber == nStreamID))   // Also check for matching interface number found above
            {
                // Copy descriptor info
                pInfo->usbstrmIF[nStreamIdx].ifDesc = lpIF->Descriptor;
                pInfo->nStreamInterfaces++;
                
                if (lpIF->lpvExtended)
                {
                    // Cast the extended ptr as a video control extended header ptr
                    const PUSBVIDSTREAMIFDESCRIPTOR lpExDesc = (PUSBVIDSTREAMIFDESCRIPTOR)lpIF->lpvExtended;
//                    if(lpExDesc->bType == USB_VIDEO_CS_INTERFACE)
//                    {
//                        pInfo->nStreamInterfaces++;
//                    }
                    
                    // Verify the header length
                    if (lpExDesc->bLen > 0x0a)
                    {
                        // Allocate the space for the descriptor
                        pInfo->usbstrmIF[nStreamIdx].lpepExtDesc = LocalAlloc (LPTR, lpExDesc->wTotalLen);
                        if (pInfo->usbstrmIF[nStreamIdx].lpepExtDesc)
                        {
                            // Copy over the extended descriptor
                            pInfo->usbstrmIF[nStreamIdx].wExtDescSize = lpExDesc->wTotalLen;
                            memcpy (pInfo->usbstrmIF[nStreamIdx].lpepExtDesc, lpExDesc, lpExDesc->wTotalLen); 
                        } 
                        else
                        {
                            DEBUGMSG (ZONE_ERROR, (DTAG TEXT("Out of memory allocating extended if descriptor\r\n")));
                            return FALSE;
                        }
                    } 
                    else
                    {
                        DEBUGMSG (ZONE_ERROR | ZONE_INIT, 
                           (DTAG TEXT("Unexpected extended Stream header format")));
                        return FALSE;
                    }
    
                }
                else
                {
                    pInfo->usbstrmIF[nStreamIdx].lpifExtDesc = 0;
                }


                // Grab the interrupt end point if specified
                if (lpIF->lpEndpoints)
                {
                    pInfo->usbstrmIF[nStreamIdx].epDesc = lpIF->lpEndpoints->Descriptor;
                    pInfo->usbstrmIF[nStreamIdx].fEndpoint = TRUE;
                }
                else
                {
                    pInfo->usbstrmIF[nStreamIdx].fEndpoint = FALSE;
                }
                
                nStreamIdx++;
            } // END IF this was a video stream interface
        }  // END FOR each interface found
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        DEBUGMSG (ZONE_ERROR, (DTAG TEXT("Exception walking device interfaces\r\n")));
        return FALSE;
    }

    if(foundcntrl == false || pInfo->nStreamInterfaces == 0)
    {
        return false;
    }

    DEBUGMSG (ZONE_INIT, (DTAG TEXT("ParseStreamInterfaces--\r\n")));

    return TRUE;
}

