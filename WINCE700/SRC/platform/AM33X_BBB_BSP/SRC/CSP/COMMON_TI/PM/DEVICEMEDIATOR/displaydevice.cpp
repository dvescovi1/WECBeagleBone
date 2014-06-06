// All rights reserved ADENEO EMBEDDED 2010
/*
===============================================================================
*             Texas Instruments OMAP(TM) Platform Software
* (c) Copyright Texas Instruments, Incorporated. All Rights Reserved.
*
* Use of this software is controlled by the terms and conditions found
* in the license agreement under which this software has been supplied.
*
===============================================================================
*/
//
//  File: DisplayDevice.cpp
//

#include <windows.h>
#include <winuser.h>
#include <winuserm.h>
#include "_constants.h"
#include "DisplayDevice.h"

// disable PREFAST warning for use of EXCEPTION_EXECUTE_HANDLER
#pragma warning (disable: 6320)

// disable PREFAST warning for empty _except block
#pragma warning (disable: 6322)

//------------------------------------------------------------------------------
// local typedefs
//
typedef HDC (WINAPI *PFN_CreateDCW)(LPCWSTR, LPCWSTR , LPCWSTR , CONST DEVMODEW *);
typedef BOOL (WINAPI *PFN_DeleteDC)(HDC);
typedef int (WINAPI *PFN_ExtEscape)(HDC, int, int, LPCSTR, int, LPSTR);

//------------------------------------------------------------------------------
// global function pointers
//
static PFN_CreateDCW s_fnCreateDCW = NULL;
static PFN_DeleteDC s_fnDeleteDC = NULL;
static PFN_ExtEscape s_fnExtEscape = NULL;

static HMODULE  s_hCore = NULL;
static HANDLE   s_hGWESApiReady = NULL;
static BOOL     s_bGWESReady = FALSE;

//------------------------------------------------------------------------------
// This routine arranges for initialization of the API used to communicate
// with devices of this class.  It may be called more than once.  If 
// successful it returns TRUE.  Otherwise it returns FALSE.
//
static 
BOOL
InitDisplayDeviceInterface()
{
    BOOL rc = FALSE;
    
    // open coredll.dll
    //
    if (s_hCore == NULL)
        {
        s_hCore = (HMODULE)::LoadLibrary(L"coredll.dll");
        if (s_hCore == NULL) goto cleanUp;
        }
    
    // get procedure addresses
    //
    s_fnCreateDCW = (PFN_CreateDCW) GetProcAddress(s_hCore, _T("CreateDCW"));
    s_fnDeleteDC = (PFN_DeleteDC) GetProcAddress(s_hCore, _T("DeleteDC"));
    s_fnExtEscape = (PFN_ExtEscape) GetProcAddress(s_hCore, _T("ExtEscape"));    
    if (!s_fnCreateDCW || !s_fnExtEscape || !s_fnDeleteDC) 
        {
        s_fnCreateDCW = NULL;
        s_fnDeleteDC = NULL;
        s_fnExtEscape = NULL;
        goto cleanUp;
        }

    // get a handle to the GWES API set signal
    //
    if (s_hGWESApiReady == NULL)
        {
        s_hGWESApiReady = OpenEvent(EVENT_ALL_ACCESS, FALSE, 
                            L"SYSTEM/GweApiSetReady"
                            );
        
        if (s_hGWESApiReady == NULL) goto cleanUp;
        }

    rc = TRUE;

cleanUp:
    return rc;
}

//------------------------------------------------------------------------------
// This routine opens a handle to a given device, or to the parent device
// that is its proxy.  The return value is the device's handle, or 
// INVALID_HANDLE_VALUE if there's an error.
//
static 
HANDLE 
OpenDisplayDevice(
    LPCTSTR szDeviceName
    )
{    
    HANDLE hRet = NULL;

    // Make sure GWES is ready for us to access this device
    //
    if  (s_bGWESReady == FALSE) 
        {
        if (InitDisplayDeviceInterface() == FALSE) goto cleanUp;
        
        // We need to wait for GWES to finish -- since this is happening
        // at boot time, we shouldn't have to wait long.
        //
        DEBUGCHK(s_hGWESApiReady != NULL);        
        DWORD dwStatus = WaitForSingleObject(s_hGWESApiReady, INFINITE);
        if(dwStatus != WAIT_OBJECT_0) goto cleanUp;

        // free allocated resources
        //
        ::CloseHandle(s_hGWESApiReady);

        s_hGWESApiReady = NULL;        
        s_bGWESReady = TRUE;
        }

    // Get a handle to the client device.  The client's name will generally 
    // be "DISPLAY" for the default display, but it could be anything that 
    // CreateDC supports.
    //
    PREFAST_DEBUGCHK(s_fnCreateDCW != NULL);
    PREFAST_DEBUGCHK(s_fnDeleteDC != NULL);
    PREFAST_DEBUGCHK(s_fnExtEscape != NULL);

    // Get handle to dc
    hRet = s_fnCreateDCW(szDeviceName, NULL, NULL, NULL);

cleanUp:    
    return hRet;
}

//-----------------------------------------------------------------------------
BOOL
DisplayDevice::Initialize(
    _TCHAR const* szDeviceName
    )
{
    BOOL rc = FALSE;

    DeviceBase::Initialize(szDeviceName);
    m_hDevice = OpenDisplayDevice(szDeviceName);
    if (m_hDevice != NULL) rc = TRUE;
    return rc;
}


//-----------------------------------------------------------------------------
BOOL
DisplayDevice::Uninitialize()
{
    if (m_hDevice != NULL) s_fnDeleteDC((HDC)m_hDevice);
    return TRUE;
}

//-----------------------------------------------------------------------------
BOOL
DisplayDevice::SendIoControl(
    DWORD dwIoControlCode, 
    LPVOID lpInBuf, 
    DWORD nInBufSize, 
    LPVOID lpOutBuf, 
    DWORD nOutBufSize, 
    DWORD *lpBytesReturned
    )
{
    BOOL rc = FALSE;
    UNREFERENCED_PARAMETER(lpBytesReturned);   
    __try 
        {
        rc = s_fnExtEscape((HDC)m_hDevice, (int)dwIoControlCode, (int)nInBufSize, 
                (LPCSTR)lpInBuf, (int)nOutBufSize, (LPSTR)lpOutBuf
                );
        }
    __except(EXCEPTION_EXECUTE_HANDLER) 
        {
        }

    return rc;
}


//-----------------------------------------------------------------------------
