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
#include <windows.h>
#include <bsp.h>
#include <initguid.h>
#include <display_drvesc.h>	
#pragma warning(push)
#pragma warning(disable: 4201)
#pragma warning(disable:4100)
#pragma warning (disable:4127)
#pragma warning (disable: 6001)
#include <svsutil.hxx>
#pragma warning(pop)
#include <auto_xxx.hxx>
#include <usbfnioctl.h>
#include <string.h>
#include <memtxapi.h>
#include "utils.h"
#include <i2cproxy.h>
#include <proxyapi.h>
#include "oalex.h"

#include "am33x_prcm.h"
#include "am33x_base_regs.h"

// disable PREFAST warning for use of EXCEPTION_EXECUTE_HANDLER
#pragma warning (disable: 6320)

// enables cacheinfo command to display OAL cache info, used for testing only
#define CACHEINFO_ENABLE        TRUE

#define MESSAGE_BUFFER_SIZE     280

EXTERN_C const GUID APM_SECURITY_GUID;

//-----------------------------------------------------------------------------

typedef VOID (*PFN_FmtPuts)(WCHAR *s, ...);
typedef WCHAR* (*PFN_Gets)(WCHAR *s, int cch);

//-----------------------------------------------------------------------------

#if 0
WCHAR const* GetVddText(int i);
WCHAR const* GetDpllText(int i);
WCHAR const* GetDpllClockText(int i);
WCHAR const* GetDeviceClockText(int i);
UINT FindClockIdByName(WCHAR const *szName);
OMAP_DEVICE FindDeviceIdByName(WCHAR const *szName);
WCHAR const* FindDeviceNameById(OMAP_DEVICE clockId);
WCHAR const* FindClockNameById(UINT clockId, UINT nLevel);
BOOL FindClockIdByName(WCHAR const *szName, UINT *pId, UINT *pLevel);
#endif

static BOOL Help(ULONG argc, LPWSTR args[], PFN_FmtPuts pfnFmtPuts);
static BOOL InReg8(ULONG argc, LPWSTR args[], PFN_FmtPuts pfnFmtPuts);
static BOOL InReg16(ULONG argc, LPWSTR args[], PFN_FmtPuts pfnFmtPuts);
static BOOL InReg32(ULONG argc, LPWSTR args[], PFN_FmtPuts pfnFmtPuts);
static BOOL InReg32x(ULONG argc, LPWSTR args[], PFN_FmtPuts pfnFmtPuts);
static BOOL OutReg8(ULONG argc, LPWSTR args[], PFN_FmtPuts pfnFmtPuts);
static BOOL OutReg16(ULONG argc, LPWSTR args[], PFN_FmtPuts pfnFmtPuts);
static BOOL OutReg32(ULONG argc, LPWSTR args[], PFN_FmtPuts pfnFmtPuts);
static BOOL Fill32(ULONG argc, LPWSTR args[], PFN_FmtPuts pfnFmtPuts);

static BOOL DumpMailboxReg(ULONG argc, LPWSTR args[], PFN_FmtPuts pfnFmtPuts);
static BOOL DumpMailboxIrqReg(ULONG argc, LPWSTR args[], PFN_FmtPuts pfnFmtPuts);

static BOOL InI2C(ULONG argc, LPWSTR args[], PFN_FmtPuts pfnFmtPuts);
static BOOL OutI2C(ULONG argc, LPWSTR args[], PFN_FmtPuts pfnFmtPuts);

#if 0
static BOOL Device_xxx(ULONG argc, LPWSTR args[], PFN_FmtPuts pfnFmtPuts);
static BOOL Observe(ULONG argc, LPWSTR args[], PFN_FmtPuts pfnFmtPuts);
static BOOL SetContrast(ULONG argc, LPWSTR args[], PFN_FmtPuts pfnFmtPuts);
static BOOL ScreenRotate(ULONG argc, LPWSTR args[], PFN_FmtPuts pfnFmtPuts);

#endif
static BOOL OPMode(ULONG argc, LPWSTR args[], PFN_FmtPuts pfnFmtPuts);
static BOOL TouchScreenCalibrate(ULONG argc, LPWSTR args[], PFN_FmtPuts pfnFmtPuts);
static BOOL CpuIdle(ULONG argc, LPWSTR args[], PFN_FmtPuts pfnFmtPuts);
static BOOL Dump_RegisterGroup(ULONG argc, LPWSTR args[], PFN_FmtPuts pfnFmtPuts);
static BOOL Reboot(ULONG argc, LPWSTR args[], PFN_FmtPuts pfnFmtPuts );
static BOOL Display(ULONG argc, LPWSTR args[], PFN_FmtPuts pfnFmtPuts );
#if 0
static BOOL TvOut(ULONG argc, LPWSTR args[], PFN_FmtPuts pfnFmtPuts );
static BOOL DVIControl(ULONG argc, LPWSTR args[], PFN_FmtPuts pfnFmtPuts );
static BOOL InterruptLatency(ULONG argc, LPWSTR args[], PFN_FmtPuts pfnFmtPuts );
static BOOL PowerDomain(ULONG argc, LPWSTR args[], PFN_FmtPuts pfnFmtPuts );

extern BOOL ProfileDvfs(ULONG argc, LPWSTR args[], PFN_FmtPuts pfnFmtPuts );
extern BOOL ProfileInterruptLatency(ULONG argc, LPWSTR args[], PFN_FmtPuts pfnFmtPuts );
extern BOOL ProfileSdrcStall(ULONG argc, LPWSTR args[], PFN_FmtPuts pfnFmtPuts );
extern BOOL ProfileWakeupAccuracy(ULONG argc, LPWSTR args[], PFN_FmtPuts pfnFmtPuts );
static BOOL HalProfile(ULONG argc, LPWSTR args[], PFN_FmtPuts pfnFmtPuts );
#endif

static LONG   GetDec(LPCWSTR string);
static UINT32 GetHex(LPCWSTR string);

#if 0
static void Dump_PRCM(PFN_FmtPuts pfnFmtPuts);
static void Dump_PRCMStates(PFN_FmtPuts pfnFmtPuts);
static void Dump_GPMC(PFN_FmtPuts pfnFmtPuts);
static void Dump_SDRC(PFN_FmtPuts pfnFmtPuts);
static void Dump_DSS(PFN_FmtPuts pfnFmtPuts);
static void Dump_ContextRestore(PFN_FmtPuts pfnFmtPuts);
static void Dump_EFuse(PFN_FmtPuts pfnFmtPuts);
#endif

static BOOL SetSlaveAddress(HANDLE hI2C, DWORD address, DWORD mode);
static DWORD WriteI2C(HANDLE hI2C, UINT8  subaddr, VOID*  pBuffer, DWORD  count, DWORD *pWritten);
static DWORD ReadI2C(HANDLE hI2C, UINT8  subaddr, VOID*  pBuffer, DWORD  count, DWORD *pRead);


#if CACHEINFO_ENABLE
static BOOL ShowCacheInfo(ULONG argc, LPWSTR args[], PFN_FmtPuts pfnFmtPuts );
#endif

static BOOL GetNEONStat(ULONG argc,LPWSTR args[],PFN_FmtPuts pfnFmtPuts);
static BOOL GetOSVersion(ULONG argc, LPWSTR args[], PFN_FmtPuts pfnFmtPuts);


BOOL SetUSBFn(ULONG argc,LPWSTR args[],PFN_FmtPuts pfnFmtPuts);

HANDLE GetProxyDriverHandle()
{
    static HANDLE _hProxy = NULL;
    if (_hProxy == NULL){
        _hProxy = CreateFile(L"PXY1:", GENERIC_WRITE|GENERIC_READ,
                    0, NULL, OPEN_EXISTING, 0, NULL
                    );
        if (_hProxy == INVALID_HANDLE_VALUE){
            _hProxy = NULL;
        }
    }

    return _hProxy;
}

//-----------------------------------------------------------------------------

typedef struct {
    DWORD       dwStart;
    DWORD       dwSize;
    void       *pv;
} MEMORY_REGISTER_ENTRY;


static MEMORY_REGISTER_ENTRY _pRegEntries[] = {
    { 0,                            0,                          NULL}
};

void* MmMapIoSpace_Proxy( PHYSICAL_ADDRESS PhysAddr, ULONG size, BOOL bNotUsed )
{
    MEMORY_REGISTER_ENTRY *pRegEntry = _pRegEntries;
    void *pAddress = NULL;
    UINT64 phSource;
    UINT32 sourceSize, offset;
    BOOL rc;
    VIRTUAL_COPY_EX_DATA data;

	UNREFERENCED_PARAMETER(bNotUsed);

    // Check if we can use common mapping for device registers (test is
    // simplified as long as we know that we support only 32 bit addressing).
    //
    if (PhysAddr.HighPart == 0) {
        while (pRegEntry->dwSize > 0){
            if (pRegEntry->dwStart <= PhysAddr.LowPart &&
                (pRegEntry->dwStart + pRegEntry->dwSize) > PhysAddr.LowPart){

                // check if memory is already mapped to the current process space
                rc = TRUE;
                if (pRegEntry->pv == NULL){
                    // reserve virtual memory and map it to a physical address
                    pRegEntry->pv = VirtualAlloc(0, pRegEntry->dwSize, MEM_RESERVE,
                                        PAGE_READWRITE | PAGE_NOCACHE);

                    if (pRegEntry->pv == NULL){
                        DEBUGMSG(TRUE, (L"ERROR: MmMapIoSpace_Proxy failed reserve registers memory\r\n"));
                        goto cleanUp;
                    }

                    // Populate IOCTL parameters
                    data.idDestProc = GetCurrentProcessId();
                    data.pvDestMem = pRegEntry->pv;
                    data.physAddr = pRegEntry->dwStart;
                    data.size = pRegEntry->dwSize;
                    rc = DeviceIoControl(GetProxyDriverHandle(), IOCTL_VIRTUAL_COPY_EX,
                            &data, sizeof(VIRTUAL_COPY_EX_DATA), NULL, 0,
                            NULL, NULL);

                }

                if (!rc){
                    DEBUGMSG(TRUE, (L"ERROR: MmMapIoSpace_Proxy failed allocate registers memory\r\n"));
                    VirtualFree(pRegEntry->pv, 0, MEM_RELEASE);
                    pRegEntry->pv = NULL;
                    goto cleanUp;
                }

                // Calculate offset
                offset = PhysAddr.LowPart - pRegEntry->dwStart;
                pAddress = (void*)((UINT32)pRegEntry->pv + offset);
                break;
            }

            // check next register map entry
            ++pRegEntry;
        }
    }

    if (pAddress == NULL){
        phSource = PhysAddr.QuadPart & ~(PAGE_SIZE - 1);
        sourceSize = size + (PhysAddr.LowPart & (PAGE_SIZE - 1));

        pAddress = VirtualAlloc(0, sourceSize, MEM_RESERVE, PAGE_READWRITE | PAGE_NOCACHE);
        if (pAddress == NULL){
            DEBUGMSG(TRUE, (L"ERROR: MmMapIoSpace_Proxy failed reserve memory\r\n"));
            goto cleanUp;
        }

        // Populate IOCTL parameters
        data.idDestProc = GetCurrentProcessId();
        data.pvDestMem = pAddress;
        data.physAddr = (UINT)phSource;
        data.size = sourceSize;
        rc = DeviceIoControl(GetProxyDriverHandle(), IOCTL_VIRTUAL_COPY_EX,
                &data, sizeof(VIRTUAL_COPY_EX_DATA), NULL, 0,NULL, NULL);

        if (!rc){
            DEBUGMSG(TRUE, (L"ERROR: MmMapIoSpace_Proxy failed allocate memory\r\n"));
            VirtualFree(pAddress, 0, MEM_RELEASE);
            pAddress = NULL;
            goto cleanUp;
        }
        pAddress = (void*)((UINT)pAddress + (UINT)(PhysAddr.LowPart & (PAGE_SIZE - 1)));
    }

cleanUp:
    return pAddress;
}

//-----------------------------------------------------------------------------
void MmUnmapIoSpace_Proxy( void *pAddress, UINT  count )
{
	UNREFERENCED_PARAMETER(pAddress);
	UNREFERENCED_PARAMETER(count);
}

//-----------------------------------------------------------------------------

static struct {
    LPCWSTR cmd;
    BOOL (*pfnCommand)(ULONG argc, LPWSTR args[], PFN_FmtPuts pfnFmtPuts);
} s_cmdTable[] = {
    { L"?", Help },
    { L"in8", InReg8 },
    { L"in16", InReg16 },
    { L"in32", InReg32 },
    { L"in32x", InReg32x },
    { L"ini2c", InI2C },
    { L"out8", OutReg8 },
    { L"out16", OutReg16 },
    { L"out32", OutReg32 },
    { L"outi2c", OutI2C },
    { L"fill32", Fill32 },
    { L"tscal", TouchScreenCalibrate},
    { L"mailbox", DumpMailboxReg},
    { L"mailboxirq", DumpMailboxIrqReg},
    { L"usbFnSet", SetUSBFn },    
    { L"dump", Dump_RegisterGroup},    
    { L"cpuidle", CpuIdle},    
    { L"opm", OPMode},    
    { L"reboot", Reboot},
    { L"display", Display},
#if CACHEINFO_ENABLE
    { L"cacheinfo", ShowCacheInfo},
#endif
    { L"OSversion", GetOSVersion}
};

//-----------------------------------------------------------------------------

BOOL ParseCommand( LPWSTR cmd, LPWSTR cmdLine, PFN_FmtPuts pfnFmtPuts, PFN_Gets pfnGets )
{
    BOOL rc = FALSE;
    LPWSTR argv[64];
    int argc = 64;
    ULONG ix;

	UNREFERENCED_PARAMETER(pfnGets);

    // Look for command
    for (ix = 0; ix < dimof(s_cmdTable); ix++)
        if (wcscmp(cmd, s_cmdTable[ix].cmd) == 0) break;

	if (ix >= dimof(s_cmdTable)) goto cleanUp;

    // Divide command line to token
    if (cmdLine != NULL)
        CommandLineToArgs(cmdLine, &argc, argv);
    else
        argc = 0;

    // Call command
    pfnFmtPuts(L"\r\n");
    __try {
        if (!s_cmdTable[ix].pfnCommand(argc, argv, pfnFmtPuts)) {
            pfnFmtPuts(L"\r\n");
            Help(0, NULL, pfnFmtPuts);
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        pfnFmtPuts(L"exception: please check addresses and values\r\n");
    }
    pfnFmtPuts(L"\r\n");
    rc = TRUE;

cleanUp:
    return rc;
}

//-----------------------------------------------------------------------------

BOOL Help( ULONG argc, LPWSTR args[], PFN_FmtPuts pfnFmtPuts )
{
	UNREFERENCED_PARAMETER(args);
	UNREFERENCED_PARAMETER(argc);

    pfnFmtPuts(L"  in8    address [size] -> read 8-bit registers\r\n");
    pfnFmtPuts(L"  in16   address [size] -> read 16-bit registers\r\n");
    pfnFmtPuts(L"  in32   address [size] -> read 32-bit registers\r\n");
    pfnFmtPuts(L"  in32x  start end      -> read 32-bit registers\r\n");
    pfnFmtPuts(L"  ini2c  [1,2,3] address subaddress [size] -> read I2C registers\r\n");
    pfnFmtPuts(L"  out8   address value  -> write 8-bit register\r\n");
    pfnFmtPuts(L"  out16  address value  -> write 16-bit register\r\n");
    pfnFmtPuts(L"  out32  address value  -> write 32-bit register\r\n");
    pfnFmtPuts(L"  outi2c [1,2,3] address subaddress value  -> write I2C register\r\n");
    pfnFmtPuts(L"  fill32 address size value  -> Fill specified address range with 32bit value\r\n");
    pfnFmtPuts(L"  mailbox -> Dump all mailbox registers\r\n");
    pfnFmtPuts(L"  mailboxirq -> Dump all mailbox registers except the mailbox msg regs\r\n");
    pfnFmtPuts(L"  tscal -> calibrate touchscreen\r\n");
    pfnFmtPuts(L"  display [on|off] -> controls display parameters\r\n");
#if CACHEINFO_ENABLE
    pfnFmtPuts(L"  cacheinfo -> show CPU cache info\r\n");
#endif
    pfnFmtPuts(L"  usbFnSet [serial/rndis/storage [<storage_device_name ex: DSK1:>]] -> Change current USB Function Client. For storage, you can optionally specify device name (along with the colon at the end)\r\n");  
    pfnFmtPuts(L"  dump [prcm | device] -> output values in register group\r\n");    
    pfnFmtPuts(L"  cpuidle -> display amount of time spent in OEMIdle\r\n");    
    pfnFmtPuts(L"  opm ? -> prints the operating mode \r\n");    
    pfnFmtPuts(L"  reboot -> software reset of device\r\n");
    pfnFmtPuts(L"  OSversion -> Get OS version information\r\n");
    pfnFmtPuts(L"\r\n");
    pfnFmtPuts(L"    Address, size and value are hex numbers without \"0x\".\r\n");
    return TRUE;
}


//-----------------------------------------------------------------------------

BOOL
GetOSVersion(
    ULONG argc,
    LPWSTR args[],
    PFN_FmtPuts pfnFmtPuts
    )
{
    BOOL rc = TRUE;
    OSVERSIONINFO osv;
	
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(args);
    UNREFERENCED_PARAMETER(pfnFmtPuts);

    osv.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    if(GetVersionEx(&osv))
    {
        pfnFmtPuts(L" OSMajor=%d, OSMinor=%d\r\n", osv.dwMajorVersion, osv.dwMinorVersion);        
    }
    else
    {
        pfnFmtPuts(L" GetVersionEx return error=%d\r\n", GetLastError());        
    }
    return rc;
}

HANDLE GetUfnController( )
{
    HANDLE hUfn = NULL;
    union {
        BYTE rgbGuidBuffer[sizeof(GUID) + 4]; // +4 since scanf writes longs
        GUID guidBus;
    } u = { 0 };
    LPGUID pguidBus = &u.guidBus;
    LPCTSTR pszBusGuid = _T("E2BDC372-598F-4619-BC50-54B3F7848D35");

    // Parse the GUID
    int iErr = _stscanf(pszBusGuid, SVSUTIL_GUID_FORMAT, SVSUTIL_PGUID_ELEMENTS(&pguidBus));
    if (iErr == 0 || iErr == EOF)
        return INVALID_HANDLE_VALUE;

    // Get a handle to the bus driver
    DEVMGR_DEVICE_INFORMATION di;
    memset(&di, 0, sizeof(di));
    di.dwSize = sizeof(di);
    ce::auto_handle hf = FindFirstDevice(DeviceSearchByGuid, pguidBus, &di);

    if (hf != INVALID_HANDLE_VALUE) {
        hUfn = CreateFile(di.szBusName, GENERIC_READ, FILE_SHARE_READ, NULL,
            OPEN_EXISTING, 0, NULL);
    } else {
        NKDbgPrintfW(_T("No available UsbFn controller!\r\n"));
    }

    return hUfn;
}


DWORD ChangeClient( HANDLE hUfn, LPCTSTR pszNewClient )
{

    if(hUfn == INVALID_HANDLE_VALUE || pszNewClient == NULL)
        return ERROR_INVALID_PARAMETER;


    DWORD dwRet = ERROR_SUCCESS;
    UFN_CLIENT_NAME ucn;
    _tcscpy(ucn.szName, pszNewClient);
    BOOL fSuccess = DeviceIoControl(hUfn, IOCTL_UFN_CHANGE_CURRENT_CLIENT, &ucn, sizeof(ucn), NULL, 0, NULL, NULL);

    if (fSuccess) {
        UFN_CLIENT_INFO uci;
        memset(&uci, 0, sizeof(uci));

        DWORD cbActual;
        fSuccess = DeviceIoControl(hUfn, IOCTL_UFN_GET_CURRENT_CLIENT, NULL, 0, &uci, sizeof(uci), &cbActual, NULL);
        if(fSuccess == FALSE || _tcsicmp(uci.szName, pszNewClient) != 0)
            return ERROR_GEN_FAILURE;

        if (uci.szName[0]) {
            RETAILMSG(1, (L"Changed to client \"%s\"\r\n", uci.szName));
        } else {
            RETAILMSG(1, (L"There is now no current client\r\n"));
        }
    } else {
        dwRet = GetLastError();
        RETAILMSG(1, (L"Could not change to client \"%s\" error %d\r\n", pszNewClient, dwRet));
    }

    return dwRet;
}

BOOL SetUSBFn( ULONG argc, LPWSTR args[], PFN_FmtPuts pfnFmtPuts )
{
    BOOL rc = TRUE;    

    ce::auto_handle hUfn = GetUfnController();
    if (hUfn == INVALID_HANDLE_VALUE) {        
        return FALSE;
    }

    if (argc>0){
        if (wcscmp(args[0], L"serial") == 0){            
            ChangeClient(hUfn,_T("serial_class"));
        } else if (wcscmp(args[0], L"rndis") == 0) {
            ChangeClient(hUfn,_T("RNDIS"));
        } else if (wcscmp(args[0], L"storage") == 0) { 
            if (argc>1) {
                TCHAR   szRegPath[MAX_PATH] = _T("\\Drivers\\USB\\FunctionDrivers\\Mass_Storage_Class");            
                DWORD dwTemp;
                HKEY hKey = NULL;
                LPWSTR pszDeviceName = args[1];
                DWORD status;
                
                if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, szRegPath, 0, NULL, 0, 0,
                                   NULL, &hKey, &dwTemp) != ERROR_SUCCESS) {
                    return FALSE;
                }
                status = RegSetValueEx(hKey, _T("DeviceName"), 0, REG_SZ, (PBYTE)pszDeviceName,
                                        sizeof(WCHAR)*(wcslen(pszDeviceName) + 1));
                RegCloseKey(hKey);
            }

            ChangeClient(hUfn,_T("mass_storage_class"));
        } else {
            pfnFmtPuts(L"Invalid USB Function type\r\n");
            rc = FALSE;
        }
    }
    return rc;
}

//-----------------------------------------------------------------------------
BOOL InReg8( ULONG argc, LPWSTR args[], PFN_FmtPuts pfnFmtPuts )
{
    BOOL rc = FALSE;
    UINT32 address = 0;
	UINT32 count = 0;
    PHYSICAL_ADDRESS pa;
    UINT8 *pAddress = NULL;
    WCHAR line[80];
    UINT8 data;
    UINT32 ix, ip;

    if (argc < 1){
        pfnFmtPuts(L"Missing address!\r\n");
        goto cleanUp;
    }

    address = GetHex(args[0]);
	count   = (argc > 1) ? GetHex(args[1]) : 1;

    pa.QuadPart = address;
    pAddress = (UINT8*)MmMapIoSpace_Proxy(pa, count * sizeof(UINT8), FALSE);
    if (pAddress == NULL){
        pfnFmtPuts(L"Failed map physical address 0x%08x to virtual address!\r\n",address);
        goto cleanUp;
    }

    for (ix = 0, ip = 0; ix < count; ix++) {
        data = INREG8(&pAddress[ix]);
        if ((ix & 0x0F) == 0) {
            StringCchPrintf(&line[ip], dimof(line) - ip, L"%08x:", address + ix);
            ip += lstrlen(&line[ip]);
        }
        StringCchPrintf( &line[ip], dimof(line) - ip, L" %02x", data);
        ip += lstrlen(&line[ip]);
        if ((ix & 0x0F) == 0x0F) {
            pfnFmtPuts(line);
            pfnFmtPuts(L"\r\n");
            ip = 0;
        }
    }
    if (ip > 0){
        pfnFmtPuts(line);
        pfnFmtPuts(L"\r\n");
    }

    rc = TRUE;

cleanUp:
    if (pAddress != NULL) MmUnmapIoSpace_Proxy(pAddress, count * sizeof(UINT8));
    return rc;
}

//-----------------------------------------------------------------------------
BOOL InReg16( ULONG argc, LPWSTR args[], PFN_FmtPuts pfnFmtPuts )
{
    BOOL rc = FALSE;
    UINT32 address = 0;
	UINT32 count = 0;
    PHYSICAL_ADDRESS pa;
    UINT8 *pAddress = NULL;
    WCHAR line[80];
    UINT16 data;
    UINT32 ix, ip;

    if (argc < 1){
        pfnFmtPuts(L"Missing address!\r\n");
        goto cleanUp;
    }

    address = GetHex(args[0]);
	count   = (argc > 1) ? GetHex(args[1]) : 1;

    pa.QuadPart = address;
    pAddress = (UINT8*)MmMapIoSpace_Proxy(pa, count * sizeof(UINT16), FALSE);
    if (pAddress == NULL){
        pfnFmtPuts(L"Failed map physical address 0x%08x to virtual address!\r\n",address);
        goto cleanUp;
    }

    for (ix = 0, ip = 0; ix < count; ix += sizeof(UINT16)) {
        data = INREG16((UINT32)pAddress + ix);
        if ((ix & 0x0F) == 0){
            StringCchPrintf(&line[ip], dimof(line) - ip, L"%08x:", address + ix);
            ip += lstrlen(&line[ip]);
        }
        StringCchPrintf(&line[ip], dimof(line) - ip, L" %04x", data);
        ip += lstrlen(&line[ip]);
        if (((ix + sizeof(UINT16)) & 0x0F) == 0){
            pfnFmtPuts(line);
            pfnFmtPuts(L"\r\n");
            ip = 0;
        }
    }
    if (ip > 0){
        pfnFmtPuts(line);
        pfnFmtPuts(L"\r\n");
    }

    rc = TRUE;

cleanUp:
    if (pAddress != NULL) MmUnmapIoSpace_Proxy(pAddress, count * sizeof(UINT16));
    return rc;
}

//-----------------------------------------------------------------------------
BOOL InReg32( ULONG argc, LPWSTR args[], PFN_FmtPuts pfnFmtPuts )
{
    BOOL rc = FALSE;
    UINT32 address = 0;
	UINT32 count = 0;
    PHYSICAL_ADDRESS pa;
    UINT8 *pAddress = NULL;
    WCHAR line[80];
    UINT32 data;
    UINT32 ix, ip;

    if (argc < 1) {
        pfnFmtPuts(L"Missing address!\r\n");
        goto cleanUp;
    }

    address = GetHex(args[0]);
	count   = (argc > 1) ? GetHex(args[1]) : 1;

    pa.QuadPart = address;
    pAddress = (UINT8*)MmMapIoSpace_Proxy(pa, count * sizeof(UINT32), FALSE);
    if (pAddress == NULL){
        pfnFmtPuts(L"Failed map physical address 0x%08x to virtual address!\r\n",
            address);
        goto cleanUp;
    }

    for (ix = 0, ip = 0; ix < count; ix += sizeof(UINT32)){
        data = INREG32((UINT32)pAddress + ix);
        if ((ix & 0x0F) == 0){
            StringCchPrintf(&line[ip], dimof(line) - ip, L"%08x:", address + ix);
            ip += lstrlen(&line[ip]);
        }
        StringCchPrintf(&line[ip], dimof(line) - ip, L" %08x", data);
        ip += lstrlen(&line[ip]);
        if (((ix + sizeof(UINT32)) & 0x0F) == 0){
            pfnFmtPuts(line);
            pfnFmtPuts(L"\r\n");
            ip = 0;
        }
    }
    if (ip > 0){
		pfnFmtPuts(line);
        pfnFmtPuts(L"\r\n");
    }

    rc = TRUE;

cleanUp:
    if (pAddress != NULL) MmUnmapIoSpace_Proxy(pAddress, count * sizeof(UINT32));
    return rc;
}

BOOL InReg32x( ULONG argc, LPWSTR args[], PFN_FmtPuts pfnFmtPuts )
{
    BOOL rc = FALSE;
    UINT32 address = 0;
    UINT32 end_address = 0;
	UINT32 count = 0;
    PHYSICAL_ADDRESS pa;
    UINT8 *pAddress = NULL;
    WCHAR line[80];
    UINT32 data;
    UINT32 ix, ip;

    if (argc < 2) {
        pfnFmtPuts(L"Missing start/end address!\r\n");
        goto cleanUp;
    }

    address = GetHex(args[0]);
    end_address = GetHex(args[1]);
	count   = ((end_address - address + 4)/4) * 4; 

    pa.QuadPart = address;
    pAddress = (UINT8*)MmMapIoSpace_Proxy(pa, count * sizeof(UINT32), FALSE);
    if (pAddress == NULL){
        pfnFmtPuts(L"Failed map physical address 0x%08x to virtual address!\r\n",
            address);
        goto cleanUp;
    }

    for (ix = 0, ip = 0; ix < count; ix += sizeof(UINT32)){
        data = INREG32((UINT32)pAddress + ix);
        if ((ix & 0x0F) == 0){
            StringCchPrintf(&line[ip], dimof(line) - ip, L"%08x:", address + ix);
            ip += lstrlen(&line[ip]);
        }
        StringCchPrintf(&line[ip], dimof(line) - ip, L" %08x", data);
        ip += lstrlen(&line[ip]);
        if (((ix + sizeof(UINT32)) & 0x0F) == 0){
            pfnFmtPuts(line);
            pfnFmtPuts(L"\r\n");
            ip = 0;
        }
    }
    if (ip > 0){
		pfnFmtPuts(line);
        pfnFmtPuts(L"\r\n");
    }

    rc = TRUE;

cleanUp:
    if (pAddress != NULL) MmUnmapIoSpace_Proxy(pAddress, count * sizeof(UINT32));
    return rc;
}

//-----------------------------------------------------------------------------

BOOL OutReg8( ULONG argc, LPWSTR args[], PFN_FmtPuts pfnFmtPuts )
{
    BOOL rc = FALSE;
    UINT32 address, value;
    PHYSICAL_ADDRESS pa;
    UINT8 *pAddress = NULL;

    if (argc < 2) {
        pfnFmtPuts(L"Address and value required!\r\n");
        goto cleanUp;
    }

    address = GetHex(args[0]);
    value = GetHex(args[1]);

    if (value > 0x0100){
        pfnFmtPuts(L"Value must be in 0x00 to 0xFF range!\r\n");
        goto cleanUp;
    }

    pa.QuadPart = address;
    pAddress = (UINT8*)MmMapIoSpace_Proxy(pa, sizeof(UINT8), FALSE);
    if (pAddress == NULL){
        pfnFmtPuts(L"Failed map physical address 0x%08x to virtual address!\r\n",
            address);
        goto cleanUp;
    }

    OUTREG8(pAddress, value);
    pfnFmtPuts(L"Done, read back: 0x%02x\r\n", INREG8(pAddress));

    rc = TRUE;

cleanUp:
    if (pAddress != NULL) MmUnmapIoSpace_Proxy(pAddress, sizeof(UINT8));
    return rc;
}

//-----------------------------------------------------------------------------
BOOL OutReg16( ULONG argc, LPWSTR args[], PFN_FmtPuts pfnFmtPuts )
{
    BOOL rc = FALSE;
    UINT32 address, value;
    PHYSICAL_ADDRESS pa;
    UINT16 *pAddress = NULL;

    if (argc < 2) {
        pfnFmtPuts(L"Address and value required!\r\n");
        goto cleanUp;
	}

    address = GetHex(args[0]);
    value = GetHex(args[1]);

    if (value > 0x00010000) {
        pfnFmtPuts(L"Value must be in 0x0000 to 0xFFFF range!\r\n");
        goto cleanUp;
    }

    pa.QuadPart = address;
    pAddress = (UINT16*)MmMapIoSpace_Proxy(pa, sizeof(UINT16), FALSE);
    if (pAddress == NULL){
        pfnFmtPuts(L"Failed map physical address 0x%08x to virtual address!\r\n",
            address);
        goto cleanUp;
    }

    OUTREG16(pAddress, value);
    pfnFmtPuts(L"Done, read back: 0x%04x\r\n", INREG16(pAddress));

    rc = TRUE;

cleanUp:
    if (pAddress != NULL) MmUnmapIoSpace_Proxy(pAddress, sizeof(UINT16));
    return rc;
}

//-----------------------------------------------------------------------------
BOOL OutReg32( ULONG argc, LPWSTR args[], PFN_FmtPuts pfnFmtPuts )
{
    BOOL rc = FALSE;
    UINT32 address, value;
    PHYSICAL_ADDRESS pa;
    UINT32 *pAddress = NULL;

    if (argc < 2){
        pfnFmtPuts(L"Address and value required!\r\n");
        goto cleanUp;
    }

    address = GetHex(args[0]);
    value = GetHex(args[1]);

    pa.QuadPart = address;
    pAddress = (UINT32*)MmMapIoSpace_Proxy(pa, sizeof(UINT32), FALSE);
    if (pAddress == NULL){
        pfnFmtPuts(L"Failed map physical address 0x%08x to virtual address!\r\n",
            address);
        goto cleanUp;
    }

    OUTREG32(pAddress, value);
    pfnFmtPuts(L"Done, read back: 0x%08x\r\n", INREG32(pAddress));
    rc = TRUE;

cleanUp:
    if (pAddress != NULL) MmUnmapIoSpace_Proxy(pAddress, sizeof(UINT32));
    return rc;
}

//-----------------------------------------------------------------------------
BOOL Fill32( ULONG argc, LPWSTR args[], PFN_FmtPuts pfnFmtPuts )
{
    BOOL rc = FALSE;
    UINT32 address, size, value, i;
    PHYSICAL_ADDRESS pa;
    UINT32 *pAddress = NULL;

    if (argc < 3) {
        pfnFmtPuts(L"Address, Size and value required!\r\n");
        goto cleanUp;
    }

    address = GetHex(args[0]);
    size = GetHex(args[1]);
    value = GetHex(args[2]);

    pa.QuadPart = address;
    pAddress = (UINT32*)MmMapIoSpace_Proxy(pa, size * sizeof(UINT32), FALSE);
    if (pAddress == NULL) {
        pfnFmtPuts(L"Failed map physical address 0x%08x to virtual address!\r\n",
            address );
        goto cleanUp;
    }

    for (i = 0; i < size; i++){
        OUTREG32(pAddress, value);
        pAddress++;
    }

    pfnFmtPuts(L"Fill done\r\n");
    rc = TRUE;

cleanUp:
    if (pAddress != NULL) MmUnmapIoSpace_Proxy(pAddress, size * sizeof(UINT32));
    return rc;
}

//------------------------------------------------------------------------------

BOOL
Reboot(
    ULONG argc,
    LPWSTR args[],
    PFN_FmtPuts pfnFmtPuts
    )
{
    BOOL rc = FALSE;

	UNREFERENCED_PARAMETER(argc);
	UNREFERENCED_PARAMETER(args);

    pfnFmtPuts( TEXT("Rebooting device\r\n") );    

    rc = KernelIoControl( IOCTL_HAL_REBOOT, NULL, 0, NULL, 0, NULL );
    if( rc == FALSE )
    {
        pfnFmtPuts(L"IOCTL_HAL_REBOOT  failed\r\n");
        goto cleanUp;
    }

cleanUp:
    return rc;
}


//-----------------------------------------------------------------------------
BOOL
TouchScreenCalibrate(
    ULONG argc,
    LPWSTR args[],
    PFN_FmtPuts pfnFmtPuts
    )
{
	UNREFERENCED_PARAMETER(argc);
	UNREFERENCED_PARAMETER(args);
	UNREFERENCED_PARAMETER(pfnFmtPuts);

    TouchCalibrate();
    return TRUE;
}



BOOL InI2C(
    ULONG argc,
    LPWSTR args[],
    PFN_FmtPuts pfnFmtPuts
    )
{
    BOOL rc = FALSE;
    TCHAR const szI2C1[] = _T("I2C1:");
    TCHAR const szI2C2[] = _T("I2C2:");
    TCHAR const szI2C3[] = _T("I2C3:");

    TCHAR const *szI2C;
    int i2cIndex;
    UINT32 address, offset, count;
    UINT32 mode = 1;
    DWORD readCount;
    HANDLE hI2C = INVALID_HANDLE_VALUE;
    UCHAR buffer[MESSAGE_BUFFER_SIZE];
    WCHAR line[80];
    ULONG ix, ip;

    if (argc < 3)
        {
        pfnFmtPuts(L"Missing i2c id, address, and/or offset to read from!\r\n");
        goto cleanUp;
        }

    i2cIndex = GetHex(args[0]);
    switch (i2cIndex)
        {
        case 1:
            szI2C = szI2C1;
            break;

        case 2:
            szI2C = szI2C2;
            break;

        case 3:
            szI2C = szI2C3;
            break;

        default:
            pfnFmtPuts(L"Invalid i2c identifier, must be 1,2,3!\r\n");
            goto cleanUp;
        }

    address = GetHex(args[1]);

    offset =  GetHex(args[2]);
    if (offset >= 0x100)
        {
        //  Special values for address mode setting:
        //
        //      0x1xx   Mode 0  Offset = xx
        //      0x2xx   Mode 8  Offset = xx
        //      0x3xx   Mode 16 Offset = xx
        //      0x4xx   Mode 24 Offset = xx
        //      0x5xx   Mode 32 Offset = xx

        if( offset > 0x504 )
        {
            pfnFmtPuts(L"Read can't cross page boundary!\r\n");
            goto cleanUp;
        }

        mode   = (offset >> 8) - 1;
        offset = offset & 0x000000FF;
        }

    if (argc > 3)
        {
        count = GetHex(args[3]);
        }
    else
        {
        count = 1;
        }
    if (count == 0) count = 1;

    hI2C = CreateFile(
        szI2C, GENERIC_WRITE|GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL
        );
    if (hI2C == INVALID_HANDLE_VALUE)
        {
        pfnFmtPuts(L"Can't open %s: device driver!\r\n", szI2C);
        goto cleanUp;
        }


    // set slave address
    if (!SetSlaveAddress(hI2C, address, mode))
        {
        pfnFmtPuts(L"Failed set I2C slave address\r\n");
        goto cleanUp;
        }

    // read data
    SetFilePointer(hI2C, offset, NULL, FILE_BEGIN);
    if (!ReadFile(hI2C, buffer, count, &readCount, NULL))
        {
        pfnFmtPuts(L"Failed reading value(s)\r\n");
        goto cleanUp;
        }


    pfnFmtPuts(L"Read %d of %d byte(s)\r\n", count, readCount);
    PREFAST_SUPPRESS(12008, "No offerflow/underflow possible there.");
    for (ix = 0, ip = 0; ix < readCount; ix++)
        {
        if ((ix & 0x0F) == 0)
            {
            StringCchPrintf(
                &line[ip], dimof(line) - ip, L"%04x:", offset + ix
                );
            ip += lstrlen(&line[ip]);
            }
        StringCchPrintf(
            &line[ip], dimof(line) - ip, L" %02x", buffer[ix]
            );
        ip += lstrlen(&line[ip]);
        if ((ix & 0x0F) == 0x0F)
            {
            pfnFmtPuts(line);
            pfnFmtPuts(L"\r\n");
            ip = 0;
            }
        }
    if (ip > 0)
        {
        pfnFmtPuts(line);
        pfnFmtPuts(L"\r\n");
        }

    rc = TRUE;

cleanUp:
    if (hI2C != INVALID_HANDLE_VALUE) CloseHandle(hI2C);
    return rc;
}


BOOL OutI2C(
    ULONG argc,
    LPWSTR args[],
    PFN_FmtPuts pfnFmtPuts
    )
{
    BOOL rc = FALSE;
    TCHAR const szI2C1[] = _T("I2C1:");
    TCHAR const szI2C2[] = _T("I2C2:");
    TCHAR const szI2C3[] = _T("I2C3:");

    TCHAR const *szI2C;
    int i2cIndex;
    UINT32 address, offset, data;
    UINT32 mode = 1;
    DWORD readCount;
    HANDLE hI2C = INVALID_HANDLE_VALUE;

    if (argc < 3)
        {
        pfnFmtPuts(L"Missing i2c id, address, and/or offset to read from!\r\n");
        goto cleanUp;
        }

    i2cIndex = GetHex(args[0]);
    switch (i2cIndex)
        {
        case 1:
            szI2C = szI2C1;
            break;

        case 2:
            szI2C = szI2C2;
            break;

        case 3:
            szI2C = szI2C3;
            break;

        default:
            pfnFmtPuts(L"Invalid i2c identifier, must be 1 or 2!\r\n");
            goto cleanUp;
        }

    address = GetHex(args[1]);

    offset =  GetHex(args[2]);
    if (offset >= 0x100)
        {
        //  Special values for address mode setting:
        //
        //      0x1xx   Mode 0  Offset = xx
        //      0x2xx   Mode 8  Offset = xx
        //      0x3xx   Mode 16 Offset = xx
        //      0x4xx   Mode 24 Offset = xx
        //      0x5xx   Mode 32 Offset = xx

        if( offset > 0x504 )
        {
            pfnFmtPuts(L"Read can't cross page boundary!\r\n");
            goto cleanUp;
        }

        mode   = (offset >> 8) - 1;
        offset = offset & 0x000000FF;
        }

    data = GetHex(args[3]);

    hI2C = CreateFile(
        szI2C, GENERIC_WRITE|GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL
        );
    if (hI2C == INVALID_HANDLE_VALUE)
        {
        pfnFmtPuts(L"Can't open %s: device driver!\r\n", szI2C);
        goto cleanUp;
        }

    // set slave address
    if (!SetSlaveAddress(hI2C, address, mode))
        {
        pfnFmtPuts(L"Failed set I2C slave address\r\n");
        goto cleanUp;
        }

    // write data
    if (!WriteI2C(hI2C, (UINT8)offset, &data, 1, &readCount))
        {
        pfnFmtPuts(L"Failed writing value(s)\r\n");
        goto cleanUp;
        }

    // verify data
    ReadI2C(hI2C, (UINT8)offset, &data, 1, &readCount);
    pfnFmtPuts(L"Done, read back value is: 0x%02x\r\n", data);
    rc = TRUE;

cleanUp:
    if (hI2C != INVALID_HANDLE_VALUE) CloseHandle(hI2C);
    return rc;
}

LONG GetDec( LPCWSTR string )
{
    LONG result;
    UINT32 ix = 0;
    BOOL bNegative = FALSE;

    result = 0;
    while (string != NULL) {
        if ((string[ix] >= L'0') && (string[ix] <= L'9')){
            result = (result * 10) + (string[ix] - L'0');
            ix++;
        } else if( string[ix] == L'-') {
            bNegative = TRUE;
            ix++;
        } else {
            break;
        }
    }
    return (bNegative) ? -result : result;
}   

//-----------------------------------------------------------------------------

UINT32 GetHex( LPCWSTR string )
{
    UINT32 result;
    UINT32 ix = 0;

    result = 0;
    while (string != NULL){
        if ((string[ix] >= L'0') && (string[ix] <= L'9')){
            result = (result << 4) + (string[ix] - L'0');
            ix++;
        } else if ((string[ix] >= L'a') && (string[ix] <= L'f')) {
            result = (result << 4) + (string[ix] - L'a' + 10);
            ix++;
        } else if (string[ix] >= L'A' && string[ix] <= L'F') {
            result = (result << 4) + (string[ix] - L'A' + 10);
            ix++;
        } else {
            break;
        }
    }
    return result;
}

//-----------------------------------------------------------------------------

BOOL
SetSlaveAddress(
    HANDLE hI2C, DWORD address, DWORD mode
    )
{
    BOOL rc;

    rc = DeviceIoControl(
        hI2C, IOCTL_I2C_SET_SLAVE_ADDRESS, &address, sizeof(address), NULL, 0,
        NULL, NULL
        );

    rc = DeviceIoControl(
        hI2C, IOCTL_I2C_SET_SUBADDRESS_MODE, &mode, sizeof(mode), NULL, 0,
        NULL, NULL
        );

    return rc;
}

//-----------------------------------------------------------------------------

DWORD
WriteI2C(
    HANDLE hI2C,
    UINT8  subaddr,
    VOID*  pBuffer,
    DWORD  count,
    DWORD *pWritten
    )
{
    SetFilePointer(hI2C, subaddr, NULL, FILE_BEGIN);
    return WriteFile(hI2C, pBuffer, count, pWritten, NULL);
}

//-----------------------------------------------------------------------------

DWORD
ReadI2C(
    HANDLE hI2C,
    UINT8  subaddr,
    VOID*  pBuffer,
    DWORD  count,
    DWORD *pRead
    )
{
    SetFilePointer(hI2C, subaddr, NULL, FILE_BEGIN);
    return ReadFile(hI2C, pBuffer, count, pRead, NULL);
}

//-----------------------------------------------------------------------------

#define DISPLAY_REGISTER_VALUE(base, offset, name)  StringCchPrintf(szBuffer, MESSAGE_BUFFER_SIZE, L#name); \
                                      pfnFmtPuts(szBuffer); \
                                      StringCchPrintf(szBuffer, MESSAGE_BUFFER_SIZE, L": Addr=0x%08x, Offset=0x%04x, Value=0x%08x\r\n", (0x480C8000+offset), offset, INREG32((UINT32 *)((UINT32)base + (UINT32)offset))); \
                                      pfnFmtPuts(szBuffer);

static BOOL DumpMailboxReg(ULONG argc, LPWSTR args[], PFN_FmtPuts pfnFmtPuts)
{
    UINT32 *   pMailboxBase;
    PHYSICAL_ADDRESS pa;
    WCHAR szBuffer[MESSAGE_BUFFER_SIZE];

    pa.QuadPart = 0x480C8000;
    pMailboxBase = (UINT32 *)MmMapIoSpace_Proxy(pa, 0x1000, FALSE);
    StringCchPrintf(szBuffer, MESSAGE_BUFFER_SIZE, L"pMailboxBase =0x%x, sample val=0x%x\r\n", pMailboxBase, INREG32((UINT32)pMailboxBase + (UINT32)0x10));
    pfnFmtPuts(szBuffer);

    DISPLAY_REGISTER_VALUE(pMailboxBase, 0,      "Mailbox Revision        ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0x10,   "Mailbox Sysconfig       ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0x40,   "Mailbox Message 0       ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0x44,   "Mailbox Message 1       ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0x48,   "Mailbox Message 2       ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0x4c,   "Mailbox Message 3       ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0x50,   "Mailbox Message 4       ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0x54,   "Mailbox Message 5       ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0x58,   "Mailbox Message 6       ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0x5c,   "Mailbox Message 7       ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0x60,   "Mailbox Message 8       ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0x64,   "Mailbox Message 9       ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0x68,   "Mailbox Message 10      ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0x6c,   "Mailbox Message 11      ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0x80,   "Mailbox FifoStatus 0    ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0x84,   "Mailbox FifoStatus 1    ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0x88,   "Mailbox FifoStatus 2    ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0x8c,   "Mailbox FifoStatus 3    ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0x90,   "Mailbox FifoStatus 4    ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0x94,   "Mailbox FifoStatus 5    ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0x98,   "Mailbox FifoStatus 6    ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0x9c,   "Mailbox FifoStatus 7    ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0xa0,   "Mailbox FifoStatus 8    ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0xa4,   "Mailbox FifoStatus 9    ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0xa8,   "Mailbox FifoStatus 10   ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0xac,   "Mailbox FifoStatus 11   ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0xc0,   "Mailbox MsgStatus 0     ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0xc4,   "Mailbox MsgStatus 1     ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0xc8,   "Mailbox MsgStatus 2     ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0xcc,   "Mailbox MsgStatus 3     ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0xd0,   "Mailbox MsgStatus 4     ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0xd4,   "Mailbox MsgStatus 5     ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0xd8,   "Mailbox MsgStatus 6     ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0xdc,   "Mailbox MsgStatus 7     ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0xe0,   "Mailbox MsgStatus 8     ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0xe4,   "Mailbox MsgStatus 9     ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0xe8,   "Mailbox MsgStatus 10    ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0xec,   "Mailbox MsgStatus 11    ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0x100,  "Mailbox IrqStatus_Raw 0 ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0x110,  "Mailbox IrqStatus_Raw 1 ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0x120,  "Mailbox IrqStatus_Raw 2 ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0x130,  "Mailbox IrqStatus_Raw 3 ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0x104,  "Mailbox IrqStatus_Clr 0 ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0x114,  "Mailbox IrqStatus_Clr 1 ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0x124,  "Mailbox IrqStatus_Clr 2 ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0x134,  "Mailbox IrqStatus_Clr 3 ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0x108,  "Mailbox IrqEnable_Set 0 ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0x118,  "Mailbox IrqEnable_Set 1 ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0x128,  "Mailbox IrqEnable_Set 2 ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0x138,  "Mailbox IrqEnable_Set 3 ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0x10c,  "Mailbox IrqEnable_Clr 0 ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0x11c,  "Mailbox IrqEnable_Clr 1 ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0x12c,  "Mailbox IrqEnable_Clr 2 ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0x13c,  "Mailbox IrqEnable_Clr 3 ");
    return TRUE;
}

static BOOL DumpMailboxIrqReg(ULONG argc, LPWSTR args[], PFN_FmtPuts pfnFmtPuts)
{
    UINT32 *   pMailboxBase;
    PHYSICAL_ADDRESS pa;
    WCHAR szBuffer[MESSAGE_BUFFER_SIZE];

    pa.QuadPart = 0x480C8000;
    pMailboxBase = (UINT32 *)MmMapIoSpace_Proxy(pa, 0x1000, FALSE);
    StringCchPrintf(szBuffer, MESSAGE_BUFFER_SIZE, L"pMailboxBase =0x%x, sample val=0x%x\r\n", pMailboxBase, INREG32((UINT32)pMailboxBase + (UINT32)0x10));
    pfnFmtPuts(szBuffer);

    DISPLAY_REGISTER_VALUE(pMailboxBase, 0,      "Mailbox Revision        ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0x80,   "Mailbox FifoStatus 0    ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0x84,   "Mailbox FifoStatus 1    ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0x88,   "Mailbox FifoStatus 2    ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0x8c,   "Mailbox FifoStatus 3    ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0x90,   "Mailbox FifoStatus 4    ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0x94,   "Mailbox FifoStatus 5    ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0x98,   "Mailbox FifoStatus 6    ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0x9c,   "Mailbox FifoStatus 7    ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0xa0,   "Mailbox FifoStatus 8    ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0xa4,   "Mailbox FifoStatus 9    ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0xa8,   "Mailbox FifoStatus 10   ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0xac,   "Mailbox FifoStatus 11   ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0xc0,   "Mailbox MsgStatus 0     ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0xc4,   "Mailbox MsgStatus 1     ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0xc8,   "Mailbox MsgStatus 2     ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0xcc,   "Mailbox MsgStatus 3     ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0xd0,   "Mailbox MsgStatus 4     ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0xd4,   "Mailbox MsgStatus 5     ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0xd8,   "Mailbox MsgStatus 6     ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0xdc,   "Mailbox MsgStatus 7     ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0xe0,   "Mailbox MsgStatus 8     ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0xe4,   "Mailbox MsgStatus 9     ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0xe8,   "Mailbox MsgStatus 10    ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0xec,   "Mailbox MsgStatus 11    ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0x100,  "Mailbox IrqStatus_Raw 0 ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0x110,  "Mailbox IrqStatus_Raw 1 ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0x120,  "Mailbox IrqStatus_Raw 2 ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0x130,  "Mailbox IrqStatus_Raw 3 ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0x104,  "Mailbox IrqStatus_Clr 0 ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0x114,  "Mailbox IrqStatus_Clr 1 ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0x124,  "Mailbox IrqStatus_Clr 2 ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0x134,  "Mailbox IrqStatus_Clr 3 ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0x108,  "Mailbox IrqEnable_Set 0 ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0x118,  "Mailbox IrqEnable_Set 1 ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0x128,  "Mailbox IrqEnable_Set 2 ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0x138,  "Mailbox IrqEnable_Set 3 ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0x10c,  "Mailbox IrqEnable_Clr 0 ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0x11c,  "Mailbox IrqEnable_Clr 1 ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0x12c,  "Mailbox IrqEnable_Clr 2 ");
    DISPLAY_REGISTER_VALUE(pMailboxBase, 0x13c,  "Mailbox IrqEnable_Clr 3 ");
    return TRUE;
}

#define DISPLAY_REGISTER_VALUE_USING_STRUCT(x, y)  StringCchPrintf(szBuffer, MESSAGE_BUFFER_SIZE, L#y); \
                                      pfnFmtPuts(szBuffer); \
                                      StringCchPrintf(szBuffer, MESSAGE_BUFFER_SIZE, L"=0x%08X\r\n", INREG32(&x->y)); \
                                      pfnFmtPuts(szBuffer);


void
Dump_PRCM(
    PFN_FmtPuts pfnFmtPuts
    )
{
    AM33X_PRCM_REGS *pPrcmGlobalPrm;
    PHYSICAL_ADDRESS pa;
    WCHAR szBuffer[MESSAGE_BUFFER_SIZE];

    pa.QuadPart = AM33X_PRCM_REGS_PA;
    pPrcmGlobalPrm = (AM33X_PRCM_REGS*)MmMapIoSpace_Proxy(pa, sizeof(AM33X_PRCM_REGS), FALSE);

    pfnFmtPuts(TEXT("\r\n===================================================\r\n"));
    pfnFmtPuts(TEXT("\r\nPER CM:\r\n"));
    pfnFmtPuts(TEXT("\r\n---------------------------------------------------\r\n"));
    pfnFmtPuts(TEXT("\r\nPER Clock Domains:\r\n"));    
    DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_L4LS_CLKSTCTRL);		// 0x0000
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_L3S_CLKSTCTRL);		// 0x0004
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_L4FW_CLKSTCTRL);		// 0x0008
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_L3_CLKSTCTRL);		// 0x000C	
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_L4HS_CLKSTCTRL);		// 0x011C
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_OCPWP_L3_CLKSTCTRL);	// 0x012C
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_ICSS_CLKSTCTRL);		// 0x0140
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_CPSW_CLKSTCTRL);		// 0x0144
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_LCDC_CLKSTCTRL);		// 0x0148
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_CLK_24MHZ_CLKSTCTRL);	// 0x0150
    pfnFmtPuts(TEXT("\r\n---------------------------------------------------\r\n"));
    pfnFmtPuts(TEXT("\r\nPER Modules:\r\n"));    
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_PCIE_CLKCTRL);		// 0x0010
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_CPGMAC0_CLKCTRL);		// 0x0014
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_LCDC_CLKCTRL);		// 0x0018
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_USB0_CLKCTRL);		// 0x001C
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_MLB_CLKCTRL);			// 0x0020
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_TPTC0_CLKCTRL);		// 0x0024
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_EMIF_CLKCTRL);		// 0x0028
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_OCMCRAM_CLKCTRL);		// 0x002C
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_GPMC_CLKCTRL);		// 0x0030
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_MCASP0_CLKCTRL);		// 0x0034
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_UART5_CLKCTRL);		// 0x0038
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_MMC0_CLKCTRL);		// 0x003C
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_ELM_CLKCTRL);			// 0x0040
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_I2C2_CLKCTRL);		// 0x0044
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_I2C1_CLKCTRL);		// 0x0048
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_SPI0_CLKCTRL);		// 0x004C
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_SPI1_CLKCTRL);		// 0x0050
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_SPI2_CLKCTRL);		// 0x0054
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_SPI3_CLKCTRL);		// 0x0058
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_L4LS_CLKCTRL);		// 0x0060
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_L4FW_CLKCTRL);		// 0x0064
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_MCASP1_CLKCTRL);		// 0x0068
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_UART1_CLKCTRL);		// 0x006C
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_UART2_CLKCTRL);		// 0x0070
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_UART3_CLKCTRL);		// 0x0074
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_UART4_CLKCTRL);		// 0x0078
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_TIMER7_CLKCTRL);		// 0x007C
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_TIMER2_CLKCTRL);		// 0x0080
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_TIMER3_CLKCTRL);		// 0x0084
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_TIMER4_CLKCTRL);		// 0x0088
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_MCASP2_CLKCTRL);		// 0x008C
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_RNG_CLKCTRL);			// 0x0090
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_AES0_CLKCTRL);		// 0x0094
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_AES1_CLKCTRL);		// 0x0098
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_DES_CLKCTRL);			// 0x009C
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_SHA0_CLKCTRL);		// 0x00A0
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_PKA_CLKCTRL);			// 0x00A4
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_GPIO6_CLKCTRL);		// 0x00A8
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_GPIO1_CLKCTRL);		// 0x00AC
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_GPIO2_CLKCTRL);		// 0x00B0
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_GPIO3_CLKCTRL);		// 0x00B4
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_GPIO4_CLKCTRL);		// 0x00B8
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_TPCC_CLKCTRL);		// 0x00BC
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_DCAN0_CLKCTRL);		// 0x00C0
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_DCAN1_CLKCTRL);		// 0x00C4
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_SPARE_CLKCTRL);		// 0x00C8
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_EPWMSS1_CLKCTRL);		// 0x00CC
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_EMIF_FW_CLKCTRL);		// 0x00D0
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_EPWMSS0_CLKCTRL);		// 0x00D4
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_EPWMSS2_CLKCTRL);		// 0x00D8
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_L3_INSTR_CLKCTRL);	// 0x00DC
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_L3_CLKCTRL);			// 0x00E0
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_IEEE5000_CLKCTRL);	// 0x00E4
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_ICSS_CLKCTRL);		// 0x00E8
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_TIMER5_CLKCTRL);		// 0x00EC
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_TIMER6_CLKCTRL);		// 0x00F0
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_MMC1_CLKCTRL);		// 0x00F4
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_MMC2_CLKCTRL);		// 0x00F8
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_TPTC1_CLKCTRL);		// 0x00FC
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_TPTC2_CLKCTRL);		// 0x0100
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_GPIO5_CLKCTRL);		// 0x0104
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_SPINLOCK_CLKCTRL);	// 0x010C
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_MAILBOX0_CLKCTRL);	// 0x0110
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_L4HS_CLKCTRL);		// 0x0120
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_MSTR_EXPS_CLKCTRL);	// 0x0124
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_SLV_EXPS_CLKCTRL);	// 0x0128
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_OCPWP_CLKCTRL);		// 0x0130
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_MAILBOX1_CLKCTRL);	// 0x0134
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_SPARE0_CLKCTRL);		// 0x0138
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_SPARE1_CLKCTRL);		// 0x013C
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_CLKDIV32K_CLKCTRL);	// 0x014C
    pfnFmtPuts(TEXT("\r\n===================================================\r\n"));
    
    pfnFmtPuts(TEXT("\r\n===================================================\r\n"));
    pfnFmtPuts(TEXT("\r\nWKUP CM:\r\n"));
    pfnFmtPuts(TEXT("\r\n---------------------------------------------------\r\n"));
    pfnFmtPuts(TEXT("\r\nWKUP Clock Domains:\r\n"));    
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_WKUP_CLKSTCTRL);			// 0x0400
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_L3_AON_CLKSTCTRL);		// 0x0418
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_L4_WKUP_AON_CLKSTCTRL);	// 0x04CC	
    pfnFmtPuts(TEXT("\r\n---------------------------------------------------\r\n"));
    pfnFmtPuts(TEXT("\r\nWKUP Modules:\r\n"));    
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_WKUP_CONTROL_CLKCTRL);	// 0x0404
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_WKUP_GPIO0_CLKCTRL);		// 0x0408
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_WKUP_L4WKUP_CLKCTRL);		// 0x040C
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_WKUP_TIMER0_CLKCTRL);		// 0x0410
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_WKUP_DEBUGSS_CLKCTRL);	// 0x0414
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_WKUP_WKUP_M3_CLKCTRL);	// 0x04B0
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_WKUP_UART0_CLKCTRL);		// 0x04B4
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_WKUP_I2C0_CLKCTRL);		// 0x04B8
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_WKUP_ADC_TSC_CLKCTRL);	// 0x04BC
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_WKUP_SMARTREFLEX0_CLKCTRL);// 0x04C0
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_WKUP_TIMER1_CLKCTRL);		// 0x04C4
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_WKUP_SMARTREFLEX1_CLKCTRL);// 0x04C8
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_WKUP_WDT0_CLKCTRL);		// 0x04D0
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_WKUP_WDT1_CLKCTRL);		// 0x04D4
	pfnFmtPuts(TEXT("\r\n===================================================\r\n"));

    pfnFmtPuts(TEXT("\r\n===================================================\r\n"));
    pfnFmtPuts(TEXT("\r\nMPU CM:\r\n"));
    pfnFmtPuts(TEXT("\r\n---------------------------------------------------\r\n"));
    pfnFmtPuts(TEXT("\r\nMPU Clock Domains:\r\n"));   
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_MPU_CLKSTCTRL);			// 0x0600
    pfnFmtPuts(TEXT("\r\n---------------------------------------------------\r\n"));
    pfnFmtPuts(TEXT("\r\nMPU Modules:\r\n"));    
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_MPU_MPU_CLKCTRL);			// 0x0604
	pfnFmtPuts(TEXT("\r\n===================================================\r\n"));

    pfnFmtPuts(TEXT("\r\n===================================================\r\n"));
    pfnFmtPuts(TEXT("\r\nRTC CM:\r\n"));
    pfnFmtPuts(TEXT("\r\n---------------------------------------------------\r\n"));
    pfnFmtPuts(TEXT("\r\nRTC Clock Domains:\r\n"));    
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_RTC_CLKSTCTRL);			// 0x0804
    pfnFmtPuts(TEXT("\r\n---------------------------------------------------\r\n"));
    pfnFmtPuts(TEXT("\r\nRTC Modules:\r\n"));    
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_RTC_RTC_CLKCTRL);			// 0x0800
	pfnFmtPuts(TEXT("\r\n===================================================\r\n"));

    pfnFmtPuts(TEXT("\r\n===================================================\r\n"));
    pfnFmtPuts(TEXT("\r\nGFX CM:\r\n"));
    pfnFmtPuts(TEXT("\r\n---------------------------------------------------\r\n"));
    pfnFmtPuts(TEXT("\r\nGFX Clock Domains:\r\n"));    
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_GFX_L3_CLKSTCTRL);		// 0x0900
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_GFX_L4LS_GFX_CLKSTCTRL);// 0x090C
    pfnFmtPuts(TEXT("\r\n---------------------------------------------------\r\n"));
    pfnFmtPuts(TEXT("\r\nGFX Modules:\r\n"));    
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_GFX_GFX_CLKCTRL);			// 0x0904
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_GFX_BITBLT_CLKCTRL);		// 0x0908
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_GFX_MMUCFG_CLKCTRL);		// 0x0910
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_GFX_MMUDATA_CLKCTRL);		// 0x0914
	pfnFmtPuts(TEXT("\r\n===================================================\r\n"));

    pfnFmtPuts(TEXT("\r\n===================================================\r\n"));
    pfnFmtPuts(TEXT("\r\nCEFUSE CM:\r\n"));
    pfnFmtPuts(TEXT("\r\n---------------------------------------------------\r\n"));
    pfnFmtPuts(TEXT("\r\nCEFUSE Clock Domains:\r\n"));    
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_CEFUSE_CLKSTCTRL);			// 0x0A00
    pfnFmtPuts(TEXT("\r\n---------------------------------------------------\r\n"));
    pfnFmtPuts(TEXT("\r\nCEFUSE Modules:\r\n"));    
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_CEFUSE_CEFUSE_CLKCTRL);	// 0x0A20
	pfnFmtPuts(TEXT("\r\n===================================================\r\n"));

    
    
    pfnFmtPuts(TEXT("\r\n===================================================\r\n"));
    pfnFmtPuts(TEXT("\r\nPRM:\r\n"));    
    pfnFmtPuts(TEXT("\r\n---------------------------------------------------\r\n"));
    pfnFmtPuts(TEXT("\r\nOCP PRM:\r\n"));    
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, REVISION_PRM);				// 0x0B00
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, PRM_IRQSTATUS_MPU);			// 0x0B04
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, PRM_IRQENABLE_MPU);			// 0x0B08
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, PRM_IRQSTATUS_M3);			// 0x0B0C
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, PRM_IRQENABLE_M3);			// 0x0B10
    pfnFmtPuts(TEXT("\r\n---------------------------------------------------\r\n"));
    pfnFmtPuts(TEXT("\r\nPER PRM:\r\n"));    
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, RM_PER_RSTCTRL);				// 0x0C00
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, RM_PER_RSTST);				// 0x0C04
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, PM_PER_PWRSTST);				// 0x0C08
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, PM_PER_PWRSTCTRL);			// 0x0C0C
    pfnFmtPuts(TEXT("\r\n---------------------------------------------------\r\n"));
    pfnFmtPuts(TEXT("\r\nWKUP PRM:\r\n"));    
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, RM_WKUP_RSTCTRL);			// 0x0D00
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, PM_WKUP_PWRSTCTRL);			// 0x0D04
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, PM_WKUP_PWRSTST);			// 0x0D08
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, RM_WKUP_RSTST);				// 0x0D0C
    pfnFmtPuts(TEXT("\r\n---------------------------------------------------\r\n"));
    pfnFmtPuts(TEXT("\r\nMPU PRM:\r\n"));    
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, PM_MPU_PWRSTCTRL);			// 0x0E00
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, PM_MPU_PWRSTST);				// 0x0E04
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, RM_MPU_RSTST);				// 0x0E08
    pfnFmtPuts(TEXT("\r\n---------------------------------------------------\r\n"));
    pfnFmtPuts(TEXT("\r\nDEVICE PRM:\r\n"));    
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, PRM_RSTCTRL);				// 0x0F00
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, PRM_RSTTIME);				// 0x0F04
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, PRM_RSTST);					// 0x0F08
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, PRM_SRAM_COUNT);				// 0x0F0C
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, PRM_LDO_SRAM_CORE_SETUP);	// 0x0F10
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, PRM_LDO_SRAM_CORE_CTRL);		// 0x0F14
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, PRM_LDO_SRAM_MPU_SETUP);		// 0x0F18
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, PRM_LDO_SRAM_MPU_CTRL);		// 0x0F1C
    pfnFmtPuts(TEXT("\r\n---------------------------------------------------\r\n"));
    pfnFmtPuts(TEXT("\r\nRTC PRM:\r\n"));    
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, PM_RTC_PWRSTCTRL);			// 0x1000
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, PM_RTC_PWRSTST);				// 0x1004
    pfnFmtPuts(TEXT("\r\n---------------------------------------------------\r\n"));
    pfnFmtPuts(TEXT("\r\nGFX PRM:\r\n"));    
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, PM_GFX_PWRSTCTRL);			// 0x1100
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, RM_GFX_RSTCTRL);				// 0x1104
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, PM_GFX_PWRSTST);				// 0x1110
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, RM_GFX_RSTST);				// 0x1114
    pfnFmtPuts(TEXT("\r\n---------------------------------------------------\r\n"));
    pfnFmtPuts(TEXT("\r\nCEFUSE PRM:\r\n"));    
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, PM_CEFUSE_PWRSTCTRL);		// 0x1200
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, PM_CEFUSE_PWRSTST);			// 0x1204	
    pfnFmtPuts(TEXT("\r\n===================================================\r\n"));
    

    pfnFmtPuts(TEXT("\r\n===================================================\r\n"));
    pfnFmtPuts(TEXT("\r\nCLKOUT:\r\n"));
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_CLKOUT_CTRL);				// 0x0700
	pfnFmtPuts(TEXT("\r\n===================================================\r\n"));
    

    pfnFmtPuts(TEXT("\r\n===================================================\r\n"));
    pfnFmtPuts(TEXT("\r\nDPLLs:\r\n"));        
    pfnFmtPuts(TEXT("\r\n---------------------------------------------------\r\n"));
    pfnFmtPuts(TEXT("\r\nDPLL MPU:\r\n"));    
    DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_AUTOIDLE_DPLL_MPU);		// 0x041C
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_IDLEST_DPLL_MPU);			// 0x0420
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_SSC_DELTAMSTEP_DPLL_MPU);	// 0x0424
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_SSC_MODFREQDIV_DPLL_MPU);	// 0x0428
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_CLKSEL_DPLL_MPU);			// 0x042C	
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_CLKMODE_DPLL_MPU);		// 0x0488
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_DIV_M2_DPLL_MPU);			// 0x04A8	
    pfnFmtPuts(TEXT("\r\n---------------------------------------------------\r\n"));
    pfnFmtPuts(TEXT("\r\nDPLL DDR:\r\n"));    
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_AUTOIDLE_DPLL_DDR);		// 0x0430
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_IDLEST_DPLL_DDR);			// 0x0434
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_SSC_DELTAMSTEP_DPLL_DDR);	// 0x0438
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_SSC_MODFREQDIV_DPLL_DDR);	// 0x043C
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_CLKSEL_DPLL_DDR);			// 0x0440
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_CLKMODE_DPLL_DDR);		// 0x0494
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_DIV_M2_DPLL_DDR);			// 0x04A0	
    pfnFmtPuts(TEXT("\r\n---------------------------------------------------\r\n"));
    pfnFmtPuts(TEXT("\r\nDPLL DISP:\r\n"));    
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_AUTOIDLE_DPLL_DISP);		// 0x0444
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_IDLEST_DPLL_DISP);		// 0x0448
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_SSC_DELTAMSTEP_DPLL_DISP);// 0x044C
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_SSC_MODFREQDIV_DPLL_DISP);// 0x0450
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_CLKSEL_DPLL_DISP);		// 0x0454
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_CLKMODE_DPLL_DISP);		// 0x0498
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_DIV_M2_DPLL_DISP);		// 0x04A4	
    pfnFmtPuts(TEXT("\r\n---------------------------------------------------\r\n"));
    pfnFmtPuts(TEXT("\r\nDPLL CORE:\r\n"));    
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_AUTOIDLE_DPLL_CORE);		// 0x0458
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_IDLEST_DPLL_CORE);		// 0x045C
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_SSC_DELTAMSTEP_DPLL_CORE);// 0x0460
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_SSC_MODFREQDIV_DPLL_CORE);// 0x0464
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_CLKSEL_DPLL_CORE);		// 0x0468	
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_CLKMODE_DPLL_CORE);		// 0x0490
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_DIV_M4_DPLL_CORE);		// 0x0480
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_DIV_M5_DPLL_CORE);		// 0x0484
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_DIV_M6_DPLL_CORE);		// 0x04D8	
    pfnFmtPuts(TEXT("\r\n---------------------------------------------------\r\n"));
    pfnFmtPuts(TEXT("\r\nDPLL PER:\r\n"));    
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_AUTOIDLE_DPLL_PER);		// 0x046C
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_IDLEST_DPLL_PER);			// 0x0470
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_SSC_DELTAMSTEP_DPLL_PER);	// 0x0474
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_SSC_MODFREQDIV_DPLL_PER);	// 0x0478
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_CLKSEL_DPLL_PERIPH);		// 0x049C
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_CLKMODE_DPLL_PER);		// 0x048C
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_CLKDCOLDO_DPLL_PER);		// 0x047C
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_DIV_M2_DPLL_PER);			// 0x04AC	
	pfnFmtPuts(TEXT("\r\n===================================================\r\n"));
    
    
    pfnFmtPuts(TEXT("\r\n===================================================\r\n"));
    pfnFmtPuts(TEXT("\r\nCLKSEL:\r\n"));        
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CLKSEL_TIMER7_CLK);			// 0x0504
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CLKSEL_TIMER2_CLK);			// 0x0508
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CLKSEL_TIMER3_CLK);			// 0x050C
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CLKSEL_TIMER4_CLK);			// 0x0510
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_MAC_CLKSEL);				// 0x0514
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CLKSEL_TIMER5_CLK);			// 0x0518
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CLKSEL_TIMER6_CLK);			// 0x051C
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_CPTS_RFT_CLKSEL);			// 0x0520
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CLKSEL_TIMER1MS_CLK);		// 0x0528
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CLKSEL_GFX_FCLK);			// 0x052C
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CLKSEL_ICSS_OCP_CLK);		// 0x0530
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CLKSEL_LCDC_PIXEL_CLK);		// 0x0534
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CLKSEL_WDT1_CLK);			// 0x0538
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CLKSEL_GPIO0_DBCLK);			// 0x053C
    pfnFmtPuts(TEXT("\r\n===================================================\r\n"));

}

void
Dump_Device(
    PFN_FmtPuts pfnFmtPuts
    )
{
    DWORD id = 1;
    BOOL ret = FALSE;
    ret = KernelIoControl( IOCTL_HAL_DUMP_REGISTERS, &id, sizeof(DWORD), NULL, 0, NULL );
    pfnFmtPuts(L"Dump_Device(%d)\r\n", ret);
}

//-----------------------------------------------------------------------------

BOOL
Dump_RegisterGroup(
    ULONG argc,
    LPWSTR args[],
    PFN_FmtPuts pfnFmtPuts
    )
{
    int i = 0;

    while (argc--)
        {
            pfnFmtPuts(L"Dump_RegisterGroup(%s)\r\n", args[i]);
       if (wcscmp(args[i], L"prcm") == 0)
            {
            Dump_PRCM(pfnFmtPuts);
            }     
       else if (wcscmp(args[i], L"device") == 0)
            {
            Dump_Device(pfnFmtPuts);
            }
        else
            {
            pfnFmtPuts(L"undefined registers(%s)\r\n", args[i]);
            }
        i++;
        }

    return TRUE;
}

#define REG_SHELL_PATH TEXT("Software\\Texas Instruments\\Shell")
BOOL CpuIdle(ULONG argc, LPWSTR args[], PFN_FmtPuts pfnFmtPuts)
{
    
    DWORD _idleLast = 0;
    DWORD _tickLast = 0;
    DWORD idle;
    DWORD tick;
    WCHAR szBuffer[MESSAGE_BUFFER_SIZE];
    
    HKEY hKey = NULL;    
    DWORD size;
    DWORD dwType = REG_DWORD;

	UNREFERENCED_PARAMETER(args);
	UNREFERENCED_PARAMETER(argc);

    //get the last parameters
    if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_LOCAL_MACHINE, (LPWSTR)REG_SHELL_PATH, 0, NULL, 
                    REG_OPTION_NON_VOLATILE, 0, NULL, &hKey, NULL))
    {  
        size=sizeof(DWORD);
        if (ERROR_SUCCESS != RegQueryValueEx(hKey, L"CPUIdle_LastTick", 0, &dwType, (BYTE*)&_tickLast, &size))
        {   
            _tickLast=0;
        }                
        size=sizeof(DWORD);
        if (ERROR_SUCCESS != RegQueryValueEx(hKey, L"CPUIdle_LastIdle", 0, &dwType, (BYTE*)&_idleLast, &size))
        {   
            _idleLast=0;
        }        
    }   

    idle = GetIdleTime();
    tick = GetTickCount();

    StringCchPrintf(szBuffer, MESSAGE_BUFFER_SIZE, L"Curr: idle count=0x%08X, tick count=0x%08X\r\n", idle, tick);
    pfnFmtPuts(szBuffer);
    
    StringCchPrintf(szBuffer, MESSAGE_BUFFER_SIZE, L"Last: idle count=0x%08X, tick count=0x%08X\r\n", _idleLast, _tickLast);
    pfnFmtPuts(szBuffer);

    StringCchPrintf(szBuffer, MESSAGE_BUFFER_SIZE, L"The difference from last check is...\r\n");
    pfnFmtPuts(szBuffer);

    StringCchPrintf(szBuffer, MESSAGE_BUFFER_SIZE, L"idle delta=0x%08X, tick delta=0x%08X\r\n", (idle - _idleLast), (tick - _tickLast));
    pfnFmtPuts(szBuffer);

    StringCchPrintf(szBuffer, MESSAGE_BUFFER_SIZE, L"cpu load is %f%%\r\n", (1.0f - ((float)(idle - _idleLast)/(float)(tick - _tickLast))) * 100.0f);
    pfnFmtPuts(szBuffer);

    _idleLast = idle;
    _tickLast = tick;

    //store the last parameters
    if (hKey!=NULL) 
    {
        size=sizeof(DWORD);
        RegSetValueEx(hKey,
                    L"CPUIdle_LastTick",
                    0,
                    dwType,
                    (BYTE*)&_tickLast,
                    size);
        size=sizeof(DWORD);
        RegSetValueEx(hKey,
                    L"CPUIdle_LastIdle",
                    0,
                    dwType,
                    (BYTE*)&_idleLast,
                    size);
        RegCloseKey(hKey);
    }
    return TRUE;
}
DWORD DumpCurrentFrequencies(
    ULONG argc,
    LPWSTR args[],
    PFN_FmtPuts pfnFmtPuts
    )
{
    UINT32 CPUspeed=0;    
    DWORD ret;    
    _TCHAR szBuffer[MAX_PATH];
    
    KernelIoControl(IOCTL_HAL_GET_CPUSPEED,
                (LPVOID)&CPUspeed, sizeof(CPUspeed), (LPVOID)&CPUspeed, 4, &ret);

    StringCchPrintf(szBuffer, MAX_PATH,_T("Current Frequencies: MPU-%dMHz\r\n"),CPUspeed);

    pfnFmtPuts(szBuffer);
	return 0;
}


//-----------------------------------------------------------------------------

BOOL
OPMode(
    ULONG argc,
    LPWSTR args[],
    PFN_FmtPuts pfnFmtPuts
    )
{
    if (argc == 0) return FALSE;
    if (wcsicmp(args[0], L"?") == 0)
    {
        // dump OPP settings
        pfnFmtPuts(L"Operating Mode Choices\r\n");
        pfnFmtPuts(L" 3 - MPU[720Mhz @ 1.26V]\r\n");
        pfnFmtPuts(L" 2 - MPU[600Mhz @ 1.20V]\r\n");
        pfnFmtPuts(L" 1 - MPU[500Mhz @ 1.10V]\r\n");
        pfnFmtPuts(L" 0 - MPU[275Mhz @ 0.95V]\r\n");
        DumpCurrentFrequencies(argc, args, pfnFmtPuts);
        return TRUE;
    }
    else
        return FALSE;
#if 0    
    else
        {
        if ((wcsicmp(args[0], L"0") != 0) &&            
            (wcsicmp(args[0], L"1") != 0) &&
            (wcsicmp(args[0], L"2") != 0) &&
            (wcsicmp(args[0], L"3") != 0) &&
            (wcsicmp(args[0], L"4") != 0) )
            {
            return FALSE;
            }

        if (argc > 1)
            {
            if (wcsicmp(args[1], L"-f") == 0)
                {
                bForce = TRUE;
                }
            }

        DeviceIoControl(
            GetProxyDriverHandle(), 
            bForce ? IOCTL_DVFS_FORCE : IOCTL_DVFS_REQUEST, 
            (void*)&opm, 
            sizeof(DWORD), 
            NULL, 
            0, 
            NULL, 
            NULL
            );
        }
    return TRUE;
#endif
    
}


//1163
//------------------------------------------------------------------------------

BOOL
Display(
    ULONG argc,
    LPWSTR args[],
    PFN_FmtPuts pfnFmtPuts
    )
{
    BOOL rc = TRUE;
    int i = 0;
    HDC hDC;
    CEDEVICE_POWER_STATE  dx;
    WCHAR  device[1024] = L"BKL1:";
    DWORD  dwErr=0;
            
    //  Get handle to display driver
    hDC = GetDC(NULL);
    if( hDC == NULL )
    {
        pfnFmtPuts(L"Error getting display driver handle\r\n");
        goto cleanUp;
    }
    

    while (argc--)
    {
        if (wcsicmp(args[i], L"on") == 0)
        {
            //  Enable display
            dx = D0;

            ExtEscape( hDC, IOCTL_POWER_SET, 0, NULL, sizeof(CEDEVICE_POWER_STATE), (LPSTR)&dx );
            argc = 0;
			
			/* turning on backlight */
			dwErr = SetDevicePower(device, 1, dx);
			if(dwErr != ERROR_SUCCESS) {
				pfnFmtPuts(TEXT("turn off backlight('%s', %d) ERROR:%d\n"), device, dx, dwErr);
			}
			
        }
        else if (wcsicmp(args[i], L"off") == 0)
        {
            dx = D4;
			
    		/* turning off backlight */
    		dwErr = SetDevicePower(device, 1, dx);
    		if(dwErr != ERROR_SUCCESS) {
    			pfnFmtPuts(TEXT("SetDevicePower('%s', %d) ERROR:%d\n"), device, dx, dwErr);
    		}
			
            //  Disable display
            ExtEscape( hDC, IOCTL_POWER_SET, 0, NULL, sizeof(CEDEVICE_POWER_STATE), (LPSTR)&dx );
            argc = 0;		
        }
        else
        {
            pfnFmtPuts(L"Unknown command\r\n");
            argc = 0;
        }

        i++;
    }


    //  Release the handle
    ReleaseDC( NULL, hDC );
    
cleanUp:
    return rc;
}

#if CACHEINFO_ENABLE

//------------------------------------------------------------------------------

BOOL
ShowCacheInfo(
    ULONG argc,
    LPWSTR args[],
    PFN_FmtPuts pfnFmtPuts
    )
{
    BOOL rc = TRUE;
    CacheInfo CpuCacheInfo;
    
    if (!CeGetCacheInfo(sizeof(CpuCacheInfo), &CpuCacheInfo))
    {
        pfnFmtPuts(L"Can't get cache info!\r\n");
        goto cleanUp;
    }

    pfnFmtPuts(L"CPU cache info:\r\n");
    if (CpuCacheInfo.dwL1Flags & CF_UNIFIED)
        pfnFmtPuts(L" L1 cache is unified\r\n");
    pfnFmtPuts(L" L1 ICacheSize     = %d\r\n", CpuCacheInfo.dwL1ICacheSize);
    pfnFmtPuts(L" L1 ICacheLineSize = %d\r\n", CpuCacheInfo.dwL1ICacheLineSize);
    pfnFmtPuts(L" L1 ICacheNumWays  = %d\r\n", CpuCacheInfo.dwL1ICacheNumWays);
    pfnFmtPuts(L" L1 DCacheSize     = %d\r\n", CpuCacheInfo.dwL1DCacheSize);
    pfnFmtPuts(L" L1 DCacheLineSize = %d\r\n", CpuCacheInfo.dwL1DCacheLineSize);
    pfnFmtPuts(L" L1 DCacheNumWays  = %d\r\n", CpuCacheInfo.dwL1DCacheNumWays);
    if (CpuCacheInfo.dwL2Flags & CF_UNIFIED)
        pfnFmtPuts(L" L2 cache is unified\r\n");
    pfnFmtPuts(L" L2 ICacheSize     = %d\r\n", CpuCacheInfo.dwL2ICacheSize);
    pfnFmtPuts(L" L2 ICacheLineSize = %d\r\n", CpuCacheInfo.dwL2ICacheLineSize);
    pfnFmtPuts(L" L2 ICacheNumWays  = %d\r\n", CpuCacheInfo.dwL2ICacheNumWays);
    pfnFmtPuts(L" L2 DCacheSize     = %d\r\n", CpuCacheInfo.dwL2DCacheSize);
    pfnFmtPuts(L" L2 DCacheLineSize = %d\r\n", CpuCacheInfo.dwL2DCacheLineSize);
    pfnFmtPuts(L" L2 DCacheNumWays  = %d\r\n", CpuCacheInfo.dwL2DCacheNumWays);
    
cleanUp:
    return rc;
}

#endif
