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

// Copyright Texas Instruments, Inc. 2011

#pragma warning(push)
#pragma warning(disable : 4115 6067)
#include <windows.h>
#include "oalex.h"

#define IOCTL_GETSTAT_CMD 0x43210000

void RetailPrint(wchar_t *pszFormat, ...);
extern int	CreateArgvArgc(TCHAR *pProgName, TCHAR *argv[20], TCHAR *pCmdLine);

void ShowHcdDMAStat(UINT32 * inbuf)
{
	int j;
	for (j=0;j<15*6;j+=6){
		RetailPrint(TEXT("Tx %08X %9u/%9u; Rx %08X %9u/%9u\n"),
			inbuf[j], inbuf[j+1], inbuf[j+2], inbuf[j+3], inbuf[j+4], inbuf[j+5]);
	}
	RetailPrint(TEXT("\nHD %9u/%9u;\n"), inbuf[90], inbuf[91]);
}

void ShowHcdInfo(UINT32 * inbuf)
{
	int j;
	for (j=0;j<15;j++){
		RetailPrint(TEXT("m_pProcessEDIn[%02d] %08X;  m_pProcessEDOut[%02d] %08X; \n"),
			j, inbuf[j], j, inbuf[j+15]);
	}
	RetailPrint(TEXT("\nm_EDUsage %9u; m_TDUsage  %9u;\n"), inbuf[30], inbuf[31]);
}

static TCHAR otgName[15][64] = {
	L"m_UsbOtgInput",
	L"m_UsbOtgInternal",
	L"m_UsbOtgOutputValues",
	L"m_UsbOtgState",
	L"m_UsbOtgMode",
	L"m_UsbOtgTimers.a_wait_vrise_tmr",
	L"m_UsbOtgTimers.a_wait_bcon_tmr",
	L"m_UsbOtgTimers.a_aidl_bdis_tmr",
	L"m_UsbOtgTimers.b_ase0_brst_tmr",
	L"m_UsbOtgTimers.b_srp_fail_tmr",
	L"m_UsbOtgTimers.b_srp_fail_count",
	L"m_bEnablePolling",
	L"m_bFunctionMode",
	L"m_InFunctionModeFn",
	L"m_bHostMode",
};

static TCHAR otgStates[][64] = {
	L"states_unknown",
	L"a_idle",
	L"a_wait_vrise",
    L"a_wait_bcon",
    L"a_host",
    L"a_suspend",
    L"a_peripheral",
    L"a_wait_vfall",
    L"a_vbus_err",
    L"b_idle",
    L"b_srp_init",
    L"b_peripheral",
    L"b_wait_acon",
    L"b_host"
};

void GetOtgStat(int port)
{
	HANDLE hDev;
	BOOL   rc = FALSE;
	UINT32 inbuf[30];
    DWORD  returned;
	UINT32    j;
    
	hDev = CreateFile((port == 1)?L"OTG1:":L"OTG2:" , GENERIC_WRITE|GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (hDev == INVALID_HANDLE_VALUE)
	{
        RetailPrint(TEXT("Can't open %s: device driver!\n"), (port == 1)?L"OTG1:":L"OTG2:");
        goto cleanUp;
    }

    rc = DeviceIoControl(hDev, IOCTL_GETSTAT_CMD + 1, NULL, 0, inbuf, sizeof(inbuf), &returned, NULL); 

	if (!rc)
		RetailPrint(TEXT("USBSTAT('%s') rc = %d; returned %d\n"),(port == 1)?L"OTG:":L"OTG2:", rc, returned);
	else
	{
		for (j=0;j<returned; j++)
		{
			if (j == 3)
				RetailPrint(TEXT("OTG%d  %2d  %s    %s\n"), port, j, otgStates[inbuf[j]], otgName[j]);
			else
				RetailPrint(TEXT("OTG%d  %2d  %08X    %s\n"), port, j, inbuf[j], otgName[j]);
		}
	}

cleanUp:
    if (hDev != INVALID_HANDLE_VALUE) CloseHandle(hDev);
}

void GetHcdInfo(int port, int cmd)
{
	HANDLE hDev;
	BOOL   rc = FALSE;
	UINT32 inbuf[128];
    DWORD  returned;
    
	hDev = CreateFile((port == 1)?L"HCD1:":L"HCD2:" , GENERIC_WRITE|GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (hDev == INVALID_HANDLE_VALUE)
	{
        RetailPrint(TEXT("Can't open %s: device driver!\n"), (port == 1)?L"HCD1:":L"HCD2:");
        goto cleanUp;
    }

    rc = DeviceIoControl(hDev, IOCTL_GETSTAT_CMD + cmd, NULL, 0, inbuf, sizeof(inbuf), &returned, NULL); 

	if (!rc)
		RetailPrint(TEXT("USBSTAT('%s') rc = %d; returned %d\n"),(port == 1)?L"HCD1:":L"HCD2:", rc, returned);
	else
	{
		switch (cmd){
			case 1: ShowHcdDMAStat(inbuf);    break;
			case 2: ShowHcdInfo(inbuf); break;
			default: 
				RetailPrint(TEXT("Command %d is not implemented\n"), cmd);
		}
	}

cleanUp:
    if (hDev != INVALID_HANDLE_VALUE) CloseHandle(hDev);
}


static TCHAR irqName[128][64] = {
    L"IRQ_EMUINT",
    L"IRQ_COMMTX",
    L"IRQ_COMMRX",
    L"IRQ_BENCH",
    L"IRQ_ELM",
    L"IRQ_SSM_WFI",
    L"IRQ_SSM",
    L"IRQ_NMI",
    L"IRQ_SEC_EVNT",
    L"IRQ_L3DEBUG",
    L"IRQ_L3APPINT",
    L"IRQ_PRCMINT",
    L"IRQ_EDMACOMPINIT",
    L"IRQ_EDMAMPERR",
    L"IRQ_EDMAERRINT",
    L"IRQ_WDTINT0",
    L"IRQ_ADC_TSC_GENINT",
    L"IRQ_USBSSINT",
    L"IRQ_USBINT0",
    L"IRQ_USBINT1",
    L"IRQ_PRUSS1_EVTOUT0",
    L"IRQ_PRUSS1_EVTOUT1",
    L"IRQ_PRUSS1_EVTOUT2",
    L"IRQ_PRUSS1_EVTOUT3",
    L"IRQ_PRUSS1_EVTOUT4",
    L"IRQ_PRUSS1_EVTOUT5",
    L"IRQ_PRUSS1_EVTOUT6",
    L"IRQ_PRUSS1_EVTOUT7",
    L"IRQ_SDINT1",
    L"IRQ_SDINT2",
    L"IRQ_I2CINT2",
    L"IRQ_ECAP0",
    L"IRQ_GPIO2A",
    L"IRQ_GPIO2B",
    L"IRQ_USBWAKEUP",
    L"IRQ_PCIWAKEUP",
    L"IRQ_LCDCINT",
    L"IRQ_GFXINT",
    L"IRQ_2DHWA",
    L"IRQ_EPWM2",
    L"IRQ_3PGSWRXTHR0",
    L"IRQ_3PGSWRXINT0",
    L"IRQ_3PGSWTXINT0",
    L"IRQ_3PGSWMISC0",
    L"IRQ_UART3INT",
    L"IRQ_UART4INT",
    L"IRQ_UART5INT",
    L"IRQ_ECAP1",
    L"IRQ_PCIINT0",
    L"IRQ_PCIINT1",
    L"IRQ_PCIINT2",
    L"IRQ_PCIINT3",
    L"IRQ_DCAN0_INT0",
    L"IRQ_DCAN0_INT1",
    L"IRQ_DCAN0_PARITY",
    L"IRQ_DCAN1_INT0",
    L"IRQ_DCAN1_INT1",
    L"IRQ_DCAN1_PARITY",
    L"IRQ_EPWM0_TZ",
    L"IRQ_EPWM1_TZ",
    L"IRQ_EPWM2_TZ",
    L"IRQ_ECAP2",
    L"IRQ_GPIO3A",
    L"IRQ_GPIO3B",
    L"IRQ_SDINT0",
    L"IRQ_SPI0INT",
    L"IRQ_TIMER0",
    L"IRQ_TIMER1MS",
    L"IRQ_TIMER2",
    L"IRQ_TIMER3",
    L"IRQ_I2CINT0",
    L"IRQ_I2CINT1",
    L"IRQ_UART0INT",
    L"IRQ_UART1INT",
    L"IRQ_UART2INT",
    L"IRQ_RTCINT",
    L"IRQ_RTCALARM",
    L"IRQ_MBINT",
    L"IRQ_M3_TXEV",
    L"IRQ_EQEP0",
    L"IRQ_MCA0TX",
    L"IRQ_MCA0RX",
    L"IRQ_MCA1TX",
    L"IRQ_MCA1RX",
    L"IRQ_MCA2TX",
    L"IRQ_MCA2RX",
    L"IRQ_EPWM0",
    L"IRQ_EPWM1",
    L"IRQ_EQEP1",
    L"IRQ_EQEP2",
    L"IRQ_DMA_INT_PIN2",
    L"IRQ_WDT1",
    L"IRQ_TIMER4",
    L"IRQ_TIMER5",
    L"IRQ_TIMER6",
    L"IRQ_TIMER7",
    L"IRQ_GPIO0A",
    L"IRQ_GPIO0B",
    L"IRQ_GPIO1A",
    L"IRQ_GPIO1B",
    L"IRQ_GPMC",
    L"IRQ_DDRERR0",
    L"IRQ_AES0_S",
    L"IRQ_AES0_P",
    L"IRQ_AES1_S",
    L"IRQ_AES1_P",
    L"IRQ_DES_S",
    L"IRQ_DES_P",
    L"IRQ_SHA_S",
    L"IRQ_SHA_P",
    L"IRQ_FPKA_SINTREQUEST",
    L"IRQ_RNG",
    L"IRQ_TCERR0",
    L"IRQ_TCERR1",
    L"IRQ_TCERR2",
    L"IRQ_ADC_TSC_PEND",
    L"IRQ_CDMA0",
    L"IRQ_CDMA1",
    L"IRQ_CDMA2",
    L"IRQ_CDMA3",
    L"IRQ_SMRFLX_ARM",
    L"IRQ_SMRFLX_CORE",
    L"IRQ_SYSMMU",
    L"IRQ_DMA_INT_PIN0"
    L"IRQ_DMA_INT_PIN1"
    L"IRQ_SPI1INT",
    L"IRQ_SPI2INT",
    L"IRQ_SPI3INT"
};

void GetIrqCounters()
{
	DWORD dwKernelRet;
	UINT32 irqCnts[128];
	int j;
	
	memset(irqCnts, 0, sizeof(irqCnts));

    if (!KernelIoControl(IOCTL_HAL_GET_IRQ_COUNTERS, NULL, 0, &irqCnts, sizeof(irqCnts), &dwKernelRet))
    {
        RetailPrint(TEXT("Failed to read Ecc type\n"));
		return;
    }   

	for (j=0; j<128; j++){
		if (irqCnts[j] > 0)
			RetailPrint(TEXT("%11u %3d-%s\n"), irqCnts[j], j, irqName[j] );
	}
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPWSTR lpCmdLine, int nCmShow)
{
    TCHAR  *argv[20];
    int    argc;
    DWORD  dwErr=1;
    
    UNREFERENCED_PARAMETER(hInst);
    UNREFERENCED_PARAMETER(hPrevInst);
    UNREFERENCED_PARAMETER(nCmShow);
    
    argc = CreateArgvArgc(TEXT( "USBSTAT" ), argv, lpCmdLine);

	if (argc>=2){
		dwErr=0;
		if      (!_tcscmp(argv[1],TEXT("hcddma1"))) GetHcdInfo(1,1);
		else if (!_tcscmp(argv[1],TEXT("hcddma2"))) GetHcdInfo(2,1);
		else if (!_tcscmp(argv[1],TEXT("irqcnt")))  GetIrqCounters();
		else if (!_tcscmp(argv[1],TEXT("otg1"))) GetOtgStat(1);
		else if (!_tcscmp(argv[1],TEXT("otg2"))) GetOtgStat(2);
		else if (!_tcscmp(argv[1],TEXT("hcd1"))) GetHcdInfo(1,2);
		else if (!_tcscmp(argv[1],TEXT("hcd2"))) GetHcdInfo(2,2);
		else dwErr = 1;
	}

    if (dwErr) {    
        RetailPrint(TEXT("USBSTAT <command>\n"));
		RetailPrint(TEXT("        hcddma1 - get stats for HCD1: dma\n"));
		RetailPrint(TEXT("        hcddma2 - get stats for HCD2: dma\n"));
		RetailPrint(TEXT("        irqcnt  - get SA Irq counters\n"));
		RetailPrint(TEXT("        otg1    - get OTG1 state\n"));
		RetailPrint(TEXT("        otg2    - get OTG2 state\n"));
		RetailPrint(TEXT("        hcd1    - get HCD1 info\n"));
		RetailPrint(TEXT("        hcd2    - get HCD2 info\n"));

        exit(1);
    }
    

    return 0;
}

///////////////////////////////////////////////////////////////////////////////

void RetailPrint(wchar_t *pszFormat, ...)
{
    va_list al;
    wchar_t szTemp[2048];
    wchar_t szTempFormat[2048];

    va_start(al, pszFormat);
    vwprintf(pszFormat, al);
    // Show message on RETAILMSG
    swprintf(szTempFormat, L"USBSTAT: %s\r", pszFormat);
    pszFormat = szTempFormat;
    vswprintf(szTemp, pszFormat, al);
    RETAILMSG(1, (szTemp));

    va_end(al);
}
#pragma warning(pop)
