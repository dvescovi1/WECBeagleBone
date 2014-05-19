// Copyright (c) 2014, David Vescovi.  All Rights Reserved.
// All rights reserved ADENEO EMBEDDED 2010
// Copyright (c) 2007, 2008 BSQUARE Corporation. All rights reserved.
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
//  File:  main.c
//
//  This file contains X-Loader implementation for AM33X
//
#include "bsp.h"

#pragma warning(push)
#pragma warning(disable: 4201)
#include <blcommon.h>
#pragma warning(pop)

#include "boot_args.h"

#include "oal_alloc.h"
#include "oal_clock.h"

//------------------------------------------------------------------------------

#ifdef SHIP_BUILD
    #define XLDRMSGINIT
    #define XLDRMSGDEINIT
    #define XLDRMSG(msg)
#else
    #define XLDRMSGINIT         {EnableDeviceClocks(BSPGetDebugUARTConfig()->dev,TRUE); OEMInitDebugSerial();}
    #define XLDRMSGDEINIT       OEMDeinitDebugSerial()
    #define XLDRMSG(msg)        OEMWriteDebugString(msg)
#endif

//------------------------------------------------------------------------------
// External Variables
extern DEVICE_IFC_GPIO Am3xx_Gpio;

//------------------------------------------------------------------------------
//  Global variables
ROMHDR * volatile const pTOC = (ROMHDR *)-1;

BOOT_CFG g_bootCfg;

unsigned int  gCPU_family;

DBGPARAM
dpCurSettings = {
    L"XLoader", {
        L"Undefined",   L"Undefined",   L"Undefined",   L"Undefined",
        L"Undefined",   L"Undefined",   L"Undefined",   L"Undefined",
        L"Undefined",   L"Undefined",   L"Undefined",   L"Undefined",
        L"Undefined",   L"Undefined",   L"Undefined",   L"Undefined"
    },
    0x0000
};

//------------------------------------------------------------------------------
//  External Functions

extern VOID PlatformSetup();
extern VOID JumpTo();
extern VOID EnableCache_GP();
extern VOID OEMDeinitDebugSerial();

extern UINT32 BLSDCardDownload(WCHAR *filename);
extern BOOL   BLSDCardReadData(ULONG size, UCHAR *pData);

//------------------------------------------------------------------------------
//  Local Functions

static BOOL SetupCopySection(ROMHDR *const pTableOfContents);
static void OutputNumHex(unsigned long n, long depth);

#if 1
void BSPGpioInit()
{
   BSPInsertGpioDevice(0,&Am3xx_Gpio,NULL);
}
#endif

//------------------------------------------------------------------------------
//
//  Function:  XLDRMain
//
VOID XLDRMain()
{
    static UCHAR allocationPool[512];
    LPCWSTR ProcessorName   = L"33X";

    // Setup global variables
    if (!SetupCopySection(pTOC))
        goto CleanUp;

    OALLocalAllocInit(allocationPool,sizeof(allocationPool));

    EnableCache_GP();
    
    /* hard coded to AM33x, run time detection can be added later */
    gCPU_family = CPU_FAMILY_AM33X;
    ProcessorName = L"33X";

	// Initialize debug serial output
    XLDRMSGINIT;

    PlatformSetup();

    // Print information...
    XLDRMSG(
        TEXT("\r\nSD/eMMC X-Loader "));
    XLDRMSG( (UINT16 *)ProcessorName);
    XLDRMSG(TEXT("\r\nBuilt ") TEXT(__DATE__) TEXT(" ") TEXT(__TIME__) TEXT("\r\n"));
    XLDRMSG(TEXT("Version ") BSP_VERSION_STRING TEXT("\r\n"));

    memset(&g_bootCfg, 0, sizeof(g_bootCfg));
    //g_bootCfg.signature = BOOT_CFG_SIGNATURE;
    //g_bootCfg.version = BOOT_CFG_VERSION;
    //g_bootCfg.bootDevLoc.IfcType = Internal;
    //g_bootCfg.bootDevLoc.BusNumber = 0;
    //g_bootCfg.bootDevLoc.LogicalLoc = BSP_LAN9115_REGS_PA;
    g_bootCfg.kitlDevLoc.IfcType = Internal;
    g_bootCfg.kitlDevLoc.BusNumber = 0;
    g_bootCfg.kitlDevLoc.LogicalLoc = 0; // +++TODO: define BSP_LAN9115_REGS_PA;
    g_bootCfg.kitlFlags |= OAL_KITL_FLAGS_DHCP;//|OAL_KITL_FLAGS_ENABLED;
    g_bootCfg.kitlFlags |= OAL_KITL_FLAGS_EXTNAME;
    g_bootCfg.kitlFlags |= OAL_KITL_FLAGS_VMINI;
    //g_bootCfg.ipAddress = 0;
    //g_bootCfg.ipMask = 0;
    //g_bootCfg.ipRoute = 0;
    //g_bootCfg.deviceID = 0;
    //g_bootCfg.osPartitionSize = IMAGE_WINCE_CODE_SIZE;
    wcscpy(g_bootCfg.filename, L"ebootsd.nb0");

    XLDRMSG(TEXT("open ebootsd.nb0 file\r\n"));

    if (BL_ERROR == BLSDCardDownload(g_bootCfg.filename))
    {
		XLDRMSG(TEXT("Failed to open file\r\n"));
        goto CleanUp;
    }

    XLDRMSG(TEXT("read ebootsd.nb0 file\r\n"));

    if (BL_ERROR == BLSDCardReadData(0x000c0000, (UCHAR *) IMAGE_STARTUP_IMAGE_PA))
    {
        XLDRMSG(TEXT("Failed to read file\r\n"));
        goto CleanUp;
    }

    XLDRMSG(TEXT("\r\njumping to eboot\r\n"));

    // Wait for serial port
    XLDRMSGDEINIT;

    // Jump to image
    JumpTo((VOID*)IMAGE_STARTUP_IMAGE_PA);

CleanUp:
    XLDRMSG(L"\r\nHALT\r\n");
    for(;;);
}


//------------------------------------------------------------------------------
//
//  Function:  OALPAtoVA
//
VOID* OALPAtoVA(UINT32 address, BOOL cached)
{
    UNREFERENCED_PARAMETER(cached);
    return (VOID*)address;
}


//------------------------------------------------------------------------------
//
//  Function:  OALVAtoPA
//
UINT32 OALVAtoPA(VOID *pVA)
{
    return (UINT32)pVA;
}


//------------------------------------------------------------------------------
//
//  Function:  SetupCopySection
//
//  Copies image's copy section data (initialized globals) to the correct
//  fix-up location.  Once completed, initialized globals are valid. Optimized
//  memcpy is too big for X-Loader.
//
static BOOL SetupCopySection(ROMHDR *const pTableOfContents)
{
    BOOL rc = FALSE;
    UINT32 loop, count;
    COPYentry *pCopyEntry;
    UINT8 *pSrc, *pDst;

    if (pTableOfContents == (ROMHDR *const) -1) goto CleanUp;

    for (loop = 0; loop < pTableOfContents->ulCopyEntries; loop++)
        {
        pCopyEntry = (COPYentry *)(pTableOfContents->ulCopyOffset + loop*sizeof(COPYentry));

        count = pCopyEntry->ulCopyLen;
        pDst = (UINT8*)pCopyEntry->ulDest;
        pSrc = (UINT8*)pCopyEntry->ulSource;
        while (count-- > 0) *pDst++ = *pSrc++;
        count = pCopyEntry->ulDestLen - pCopyEntry->ulCopyLen;
        while (count-- > 0) *pDst++ = 0;
        }

    rc = TRUE;

CleanUp:
    return rc;
}

/*****************************************************************************
*
*
*   @func   void    |   OutputNumHex | Print the hex representation of a number through the monitor port.
*
*   @rdesc  none
*
*   @parm   unsigned long |   n |
*               The number to print.
*
*   @parm   long | depth |
*               Minimum number of digits to print.
*
*/
static void OutputNumHex(unsigned long n, long depth)
{
    if (depth)
    {
        depth--;
    }

    if ((n & ~0xf) || depth)
    {
        OutputNumHex(n >> 4, depth);
        n &= 0xf;
    }

    if (n < 10)
    {
        OEMWriteDebugByte((BYTE)(n + '0'));
    }
    else
    {
        OEMWriteDebugByte((BYTE)(n - 10 + 'A'));
    }
}


/*****************************************************************************
*
*
*   @func   void    |   OutputNumDecimal | Print the decimal representation of a number through the monitor port.
*
*   @rdesc  none
*
*   @parm   unsigned long |   n |
*               The number to print.
*
*/
static void OutputNumDecimal(unsigned long n)
{
    if (n >= 10)
    {
        OutputNumDecimal(n / 10);
        n %= 10;
    }
    OEMWriteDebugByte((BYTE)(n + '0'));
}

//------------------------------------------------------------------------------
//
//  Function:  NKvDbgPrintfW
//
//
VOID NKvDbgPrintfW(LPCWSTR pszFormat, va_list vl)
{
    // Stubbed out to shrink XLDR binary size
    TCHAR c;

    while (*pszFormat)
    {
        c = *pszFormat++;
        switch ((BYTE)c)
        {
            case '%':
                c = *pszFormat++;
                switch (c)
                {
                    case 'x':
                        OutputNumHex(va_arg(vl, unsigned long), 0);
                        break;

                    case 'B':
                        OutputNumHex(va_arg(vl, unsigned long), 2);
                        break;

                    case 'H':
                        OutputNumHex(va_arg(vl, unsigned long), 4);
                        break;

                    case 'X':
                        OutputNumHex(va_arg(vl, unsigned long), 8);
                        break;

                    case 'd':
                    {
                        long    l;

                        l = va_arg(vl, long);
                        if (l < 0)
                        {
                            OEMWriteDebugByte('-');
                            l = - l;
                        }
                        OutputNumDecimal((unsigned long)l);
                        break;
                    }

                    case 'u':
                        OutputNumDecimal(va_arg(vl, unsigned long));
                        break;

                    case 's':
                        OEMWriteDebugString(va_arg(vl, LPWSTR));
                        break;

                    case '%':
                        OEMWriteDebugByte('%');
                        break;

                    case 'c':
                        c = va_arg(vl, TCHAR);
                        OEMWriteDebugByte((BYTE)c);
                        break;

                    default:
                        OEMWriteDebugByte(' ');
                        break;
                }
                break;

            case '\n':
                OEMWriteDebugByte('\r');
                // fall through

            default:
                OEMWriteDebugByte((BYTE)c);
        }
    }
}

//------------------------------------------------------------------------------
//
//  Function:  NKDbgPrintfW
//
//
VOID NKDbgPrintfW(LPCWSTR pszFormat, ...)
{
    // Can be stubbed out to shrink XLDR binary size...
    va_list vl;

    va_start(vl, pszFormat);
    NKvDbgPrintfW(pszFormat, vl);
    va_end(vl);
}

//------------------------------------------------------------------------------
