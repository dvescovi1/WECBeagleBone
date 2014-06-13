//==============================================================================
//!   Copyright (c) Texas Instruments Incorporated 2011
//!
//!   Use of this software is controlled by the terms and conditions found 
//!   in the license agreement under which this software has been supplied.
//==============================================================================
#pragma warning (push, 3)
#include <windows.h>
#include <ceddk.h>
#include <nkintr.h>
#include <windev.h>
#include "am33x.h"

//------------------------------------------------------------------------------
#ifdef DEBUG

#define ZONE_ERROR          DEBUGZONE(0)
#define ZONE_WARN           DEBUGZONE(1)
#define ZONE_FUNCTION       DEBUGZONE(2)
#define ZONE_INIT           DEBUGZONE(3)
#define ZONE_INFO           DEBUGZONE(4)
#define ZONE_IST            DEBUGZONE(5)
#define ZONE_IOCTL          DEBUGZONE(6)
#define ZONE_VERBOSE        DEBUGZONE(7)

DBGPARAM dpCurSettings = {
    L"RNDTST Driver", {
        L"Errors",      L"Warnings",    L"Function",    L"Init",
        L"Info",        L"IST",         L"IOCTL",       L"Verbose",
        L"Undefined",   L"Undefined",   L"Undefined",   L"Undefined",
        L"Undefined",   L"Undefined",   L"Undefined",   L"Undefined"
    },
    0x0003
};
#endif

/************* Globals ***************/
static UINT32  f_rndtestInst;

static UINT32  f_rndtestIrq         = IRQ_GPIO_45;
static DWORD   f_rndtestSysIntr     = SYSINTR_UNDEFINED;

static DWORD   f_rndtestIstThreadPriority = 253;

static HANDLE f_rndtestIntrEvent;
static HANDLE f_rndtestIntrThread1;

DWORD WINAPI RndtstIst( __in  LPVOID lpParameter );

//==============================================================================
//	RNDTSTIntrInitialize(void)
//==============================================================================
BOOL RNDTSTIntrInitialize( void ) {

	if (!KernelIoControl(IOCTL_HAL_REQUEST_SYSINTR, &f_rndtestIrq,
                         sizeof(UINT32), &f_rndtestSysIntr, sizeof(UINT32), NULL)) {
        ERRORMSG(TRUE, (TEXT("Error obtaining RNDTST SYSINTR value!\n")));
        f_rndtestSysIntr = SYSINTR_UNDEFINED;
        return (FALSE);
    }

    f_rndtestIntrEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    if (!f_rndtestIntrEvent) {
        return (FALSE);
    }

    if( !(InterruptInitialize(f_rndtestSysIntr,    f_rndtestIntrEvent, 0, 0))) {
        DEBUGMSG(TRUE,(TEXT("ERROR: RNDTST:Interrupt Initialize failed.\r\n")));
        return (FALSE);
    }
	
	f_rndtestIntrThread1 = CreateThread(NULL, 0, RndtstIst, (void*)1, 0, NULL);
	if( !f_rndtestIntrThread1 ) {return (FALSE);}

	CeSetThreadPriority (f_rndtestIntrThread1, f_rndtestIstThreadPriority );

	return (TRUE);
}

DWORD WINAPI RndtstIst( __in  LPVOID lpParameter ) {
    ULONGLONG ullNow = 0;
    ULONGLONG ullLast = 0;
	UINT32		cnt = 0;
	BOOL        fl = FALSE;
	UINT32		thrNum = (UINT32)lpParameter;


	RETAILMSG(1, (TEXT("RndtstIst [%d]\r\n"), thrNum));
	
    for(;;) {
        DEBUGMSG(ZONE_IST, (TEXT("Waiting for RNDTST event ...\r\n")));
        RETAILMSG(1, (L"Waiting for RNDTST event ...\r\n"));
        WaitForSingleObject(f_rndtestIntrEvent, INFINITE);

		RETAILMSG(1, (L"RNDTST EVENT !!!!!!!!!!\r\n"));
        // Note: notify upper layer 
		// SetEvent(f_rndtestNotifyEvent);

        InterruptDone(f_rndtestSysIntr);
    }
	return 0;
}

//==============================================================================
//	DllEntry
BOOL WINAPI DllEntry( HANDLE hInstDll, DWORD dwReason, LPVOID lpvReserved ) {
    switch ( dwReason ) {
		case DLL_PROCESS_ATTACH:
			DEBUGMSG(TRUE, (TEXT("RNDTST : DLL_PROCESS_ATTACH\r\n")));
			DisableThreadLibraryCalls((HMODULE) hInstDll);
			break;

		case DLL_PROCESS_DETACH:
			DEBUGMSG(TRUE, (TEXT("RNDTST : DLL_PROCESS_DETACH\r\n")));
			break;
    }
    return (TRUE);
}

//==============================================================================
//	Init
HANDLE RDT_Init ( LPCWSTR ActiveKey ) {
    HANDLE  hHandle;

    DEBUGMSG(TRUE, (TEXT("RNDTST: Init\r\n")));

    if (TRUE == RNDTSTIntrInitialize()) {
        DEBUGMSG(TRUE, (TEXT("RNDTST: Invoked RNDTSTIntrInitialize\r\n")));
    }

	f_rndtestInst = 0xAA551234;
    hHandle = (HANDLE) f_rndtestInst;

    DEBUGMSG(TRUE, (TEXT("RNDTST_Init: Returning 0x%X\r\n"), hHandle));

    return (hHandle);
}

//==============================================================================
//	DeInit
void RDT_Deinit ( HANDLE hInitKey ) {
    return;
}

DWORD RDT_Open(DWORD hDeviceContext, DWORD AccessCode, DWORD ShareMode) {
                                                   return hDeviceContext;}
BOOL  RDT_Close(DWORD hOpenContext){ return TRUE; }
DWORD RDT_Read(DWORD hOpenContext, LPVOID pBuffer, DWORD Count){ return 0; }
DWORD RDT_Write(DWORD hOpenContext, LPCVOID pSourceBytes, DWORD NumberOfBytes) {
                                                                 return 0; }
DWORD RDT_Seek(DWORD hOpenContext, LONG Amount, DWORD Type){ return 0;}

//------------------------------------------------------------------------------
//  Function:  RDT_IOControl
BOOL RDT_IOControl( DWORD  hOpenContext, DWORD  dwIoctl, PBYTE  pBufIn,
                    DWORD  dwLenIn, PBYTE  pBufOut, DWORD  dwLenOut,
					PDWORD pdwActualOut )
{
    DWORD  dwError = ERROR_INVALID_PARAMETER;
    BOOL   bReturn = FALSE;

    DEBUGMSG(ZONE_IOCTL, (_T("RDT_IOControl: IOCTL:0x%x, InBuf:0x%x, InBufLen:%d, OutBuf:0x%x, OutBufLen:0x%x)\r\n"),
                    dwIoctl, pBufIn, dwLenIn, pBufOut, dwLenOut));

    // No IOCTLs supported
    DEBUGMSG(ZONE_IOCTL, (_T("RDT_IOControl: Unsupported IOCTL code %u\r\n"), dwIoctl));
    dwError = ERROR_NOT_SUPPORTED;

    // pass back appropriate response codes
    SetLastError(dwError);
	bReturn = (dwError != ERROR_SUCCESS) ? FALSE : TRUE; 

    return bReturn;
}
#pragma warning (pop)
