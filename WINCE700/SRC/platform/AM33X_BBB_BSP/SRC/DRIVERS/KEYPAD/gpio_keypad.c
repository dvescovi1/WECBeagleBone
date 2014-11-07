// All rights reserved ADENEO EMBEDDED 2010
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
//------------------------------------------------------------------------------
//
//  File: gpio_keypad.c
//
//  This file implements device driver for keypad. The driver isn't implemented
//  as classical keyboard driver. Instead implementation uses stream driver
//  model and it calls keybd_event to pass information to GWE subsystem.
//


#include "omap.h"
#include "ceddkex.h"
#include "gpio.h"
#include "sdk_gpio.h"
#include "gpio_keypad.h"
#include "sdk_padcfg.h"
#include "bsp_padcfg.h"
#include "bsp.h"

#include <nkintr.h>

#include <winuserm.h>
#include <pmpolicy.h>

//------------------------------------------------------------------------------
//
//  Global:  dpCurSettings
//
//  Set debug zones names and initial setting for driver
//
#ifdef DEBUG

//#define ZONE_ERROR          DEBUGZONE(0)
#define ZONE_WARN           DEBUGZONE(1)
#define ZONE_FUNCTION       DEBUGZONE(2)
//#define ZONE_INIT           DEBUGZONE(3)
#define ZONE_INFO           DEBUGZONE(4)
#define ZONE_IST            DEBUGZONE(5)

//------------------------------------------------------------------------------
//
//  Global:  dpCurSettings
//
DBGPARAM dpCurSettings = {
    L"keypad", {
        L"Errors",      L"Warnings",    L"Function",    L"Init",
        L"Info",        L"IST",         L"Undefined",   L"Undefined",
        L"Undefined",   L"Undefined",   L"Undefined",   L"Undefined",
        L"Undefined",   L"Undefined",   L"Undefined",   L"Undefined"
    },
    0x3
};

#endif

//------------------------------------------------------------------------------
//  Local Definitions

/* LCD4 keypad */
#define BSP_KEY1_GPIO			GPIO1_16
#define BSP_KEY2_GPIO			GPIO1_17
#define BSP_KEY3_GPIO			GPIO1_19
#define BSP_KEY4_GPIO			GPIO3_16
#define BSP_KEY5_GPIO			GPIO0_15
#define BSP_KEY6_GPIO			GPIO0_3


#define VK_KEYS                     256
#define DWORD_BITS                  32

#define NB_KEYS 6

DWORD g_GPIOId[NB_KEYS] = {BSP_KEY1_GPIO,BSP_KEY2_GPIO,BSP_KEY3_GPIO,BSP_KEY4_GPIO,BSP_KEY5_GPIO,BSP_KEY6_GPIO};

//------------------------------------------------------------------------------
//  Local Structures
typedef struct {
    DWORD  priority256;
    DWORD  samplePeriod;
    DWORD  debounceTime;
    DWORD  firstRepeat;
    DWORD  nextRepeat;
    CRITICAL_SECTION cs;
    HANDLE hThread;
    BOOL   bThreadExit;
    HANDLE hGpio;
} KPD_DEVICE;

typedef struct {
    BOOL pending;
    BOOL remapped;
    DWORD time;
    DWORD state;
} KEYPAD_REMAP_STATE;

typedef struct {
    BOOL pending;
    DWORD time;
    BOOL blocked;
} KEYPAD_REPEAT_STATE;


static const PAD_INFO KeypadPads[]    = {KEYPAD_PADS END_OF_PAD_ARRAY};

//------------------------------------------------------------------------------
//  Device registry parameters

static const DEVICE_REGISTRY_PARAM g_deviceRegParams[] = {
    {
        L"Priority256", PARAM_DWORD, FALSE, offset(KPD_DEVICE, priority256),
        fieldsize(KPD_DEVICE, priority256), (VOID*)100
    }, 
    {
        L"SamplePeriod", PARAM_DWORD, FALSE, offset(KPD_DEVICE, samplePeriod),
        fieldsize(KPD_DEVICE, samplePeriod), (VOID*)40
    }, {
        L"DebounceTime", PARAM_DWORD, FALSE, offset(KPD_DEVICE, debounceTime),
        fieldsize(KPD_DEVICE, debounceTime), (VOID*)0x50
    }, {
        L"FirstRepeat", PARAM_DWORD, FALSE, offset(KPD_DEVICE, firstRepeat),
        fieldsize(KPD_DEVICE, firstRepeat), (VOID*)500
    }, {
        L"NextRepeat", PARAM_DWORD, FALSE, offset(KPD_DEVICE, nextRepeat),
        fieldsize(KPD_DEVICE, nextRepeat), (VOID*)125
    }
};

//------------------------------------------------------------------------------
//  Local functions

BOOL KPD_Deinit(DWORD context);
static DWORD KPD_Thread(VOID *pParam);

//------------------------------------------------------------------------------
DWORD KPD_Init(LPCTSTR szContext, LPCVOID pBusContext)
//  Called by device manager to initialize device.
{
    int i;
    DWORD rc = (DWORD)NULL;
    KPD_DEVICE *pDevice = NULL;

	UNREFERENCED_PARAMETER(pBusContext);

    DEBUGMSG(ZONE_FUNCTION, (L"+KPD_Init(%s, 0x%08x)\r\n", szContext, pBusContext));
    RETAILMSG(1, (L"+KPD_Init\r\n"));

	ReleasePadArray(KeypadPads);

	if (RequestAndConfigurePadArray(KeypadPads)==FALSE){
        DEBUGMSG(ZONE_ERROR, (L"ERROR: KPD_Init: Cannot configure keypad pads\r\n"));
        goto cleanUp;
	}

    pDevice = (KPD_DEVICE *)LocalAlloc(LPTR, sizeof(KPD_DEVICE));
    if (pDevice == NULL) {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: KPD_Init: Failed allocate KDP driver structure\r\n"));
        goto cleanUp;
    }

	memset(pDevice, 0, sizeof(KPD_DEVICE));

    pDevice->hGpio = GPIOOpen();
    if (pDevice->hGpio == NULL){
        DEBUGMSG(ZONE_ERROR, (L"ERROR: KPD_Init: Failed to open GPIO driver\r\n"));
        goto cleanUp;
    }

    // make key GPIO pins inputs
    for ( i = 0; i < NB_KEYS; i++ )
    {
        GPIOSetMode(pDevice->hGpio, g_GPIOId[i], GPIO_DIR_INPUT);
    }

    InitializeCriticalSection(&pDevice->cs);

    // Read device parameters
    if (GetDeviceRegistryParams(szContext, pDevice, dimof(g_deviceRegParams), g_deviceRegParams) 
		!= ERROR_SUCCESS) {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: KPD_Init: Failed read KPD driver registry parameters\r\n"));
       
        pDevice->priority256=100;
        pDevice->samplePeriod=40;
        pDevice->debounceTime=0x50;
        pDevice->firstRepeat=500;
        pDevice->nextRepeat=125;
    }

    if ((pDevice->hThread = CreateThread(NULL, 0, KPD_Thread, pDevice, 0,NULL)) == NULL) {
        DEBUGMSG (ZONE_ERROR, (L"ERROR: KPD_Init: Failed create interrupt thread\r\n"));
        goto cleanUp;
    }

	CeSetThreadPriority(pDevice->hThread, pDevice->priority256);

    rc = (DWORD)pDevice;

cleanUp:
    if (rc == 0) KPD_Deinit((DWORD)pDevice);
    DEBUGMSG(ZONE_FUNCTION, (L"-KPD_Init(rc = %d\r\n", rc));

    return rc;
}

//------------------------------------------------------------------------------
BOOL KPD_Deinit(DWORD context)
{
    BOOL rc = FALSE;
    KPD_DEVICE *pDevice = (KPD_DEVICE*)context;

    DEBUGMSG(ZONE_FUNCTION, (L"+KPD_Deinit(0x%08x)\r\n", context));

    // Check if we get correct context
    if (pDevice == NULL) {
        DEBUGMSG (ZONE_ERROR, (L"ERROR: KPD_Deinit: Incorrect context parameter\r\n"));
        goto cleanUp;
    }

    // Close interrupt thread
    if (pDevice->hThread != NULL) {
        pDevice->bThreadExit = TRUE;   // Signal stop to thread
        WaitForSingleObject(pDevice->hThread, INFINITE); // Wait until thread exits
    }

    DeleteCriticalSection(&pDevice->cs);

    if (pDevice->hGpio == NULL)
        GPIOClose(pDevice->hGpio);

	ReleasePadArray(KeypadPads);

    LocalFree(pDevice);
    rc = TRUE;

cleanUp:
    DEBUGMSG(ZONE_FUNCTION, (L"-KPD_Deinit(rc = %d)\r\n", rc));
    return rc;
}

//------------------------------------------------------------------------------
DWORD KPD_Open(DWORD context, DWORD accessCode, DWORD shareMode)
{
    UNREFERENCED_PARAMETER(context);
    UNREFERENCED_PARAMETER(accessCode);
    UNREFERENCED_PARAMETER(shareMode);
    return context;
}

//------------------------------------------------------------------------------
BOOL KPD_Close(DWORD context)
{
    UNREFERENCED_PARAMETER(context);
    return TRUE;
}

//------------------------------------------------------------------------------
BOOL KPD_IOControl(
    DWORD context, DWORD code, BYTE *pInBuffer, DWORD inSize, BYTE *pOutBuffer,
    DWORD outSize, DWORD *pOutSize
) {
    UNREFERENCED_PARAMETER(context);
    UNREFERENCED_PARAMETER(code);
    UNREFERENCED_PARAMETER(pInBuffer);
    UNREFERENCED_PARAMETER(inSize);
    UNREFERENCED_PARAMETER(pOutBuffer);
    UNREFERENCED_PARAMETER(outSize);
    UNREFERENCED_PARAMETER(pOutSize);
    DEBUGMSG(ZONE_INIT, (L"KPD_IOControl"));
    return FALSE;
}

//------------------------------------------------------------------------------
DWORD KPD_Thread(VOID *pContext)
{
//    int index;
    KPD_DEVICE *pDevice = (KPD_DEVICE*)pContext;
    DWORD timeout, time, ix;
    DWORD change, mask;
    UINT16 i;
    USHORT data;
	USHORT old_data;
    UINT16 ic=0;
    DWORD vkState[VK_KEYS/DWORD_BITS], vkNewState[VK_KEYS/DWORD_BITS];
    KEYPAD_REMAP_STATE *pRemapState = NULL;
    KEYPAD_REPEAT_STATE *pRepeatState = NULL;
    BOOL keyDown;
    UCHAR vk=0;

    DEBUGMSG(ZONE_IST, (L"KPD - Start IntrThread\r\n"));


    // Init data
    memset(vkState, 0, sizeof(vkState));

	// Initialize remap informations
    if (g_keypadRemap.count > 0) {
        // Allocate state structure for remap
        if ((pRemapState = LocalAlloc(LPTR, g_keypadRemap.count * sizeof(KEYPAD_REMAP_STATE)))  == NULL) {
            DEBUGMSG(ZONE_ERROR, (L" KPD_Thread: Failed allocate memory for virtual key remap\r\n"));
            goto cleanUp;
        }
    }

    // Initialize repeat informations
    if (g_keypadRepeat.count > 0) {
        // Allocate state structure for remap
        if ((pRepeatState = LocalAlloc(LPTR, g_keypadRepeat.count * sizeof(KEYPAD_REPEAT_STATE))) == NULL){
            DEBUGMSG(ZONE_ERROR, (L" KPD_Thread: Failed allocate memory for virtual key auto repeat\r\n"));
            goto cleanUp;
        }
    }

    timeout = pDevice->samplePeriod;
	old_data = 0;
    // Loop until we are not stopped...
    while (!pDevice->bThreadExit) {
		Sleep(timeout/2);
		data = 0;
		data |= (GPIOGetBit(pDevice->hGpio,BSP_KEY1_GPIO)) ? 0 : 0x01;			  
		data |= (GPIOGetBit(pDevice->hGpio,BSP_KEY2_GPIO)) ? 0 : 0x02;			  
		data |= (GPIOGetBit(pDevice->hGpio,BSP_KEY3_GPIO)) ? 0 : 0x04;			  
		data |= (GPIOGetBit(pDevice->hGpio,BSP_KEY4_GPIO)) ? 0 : 0x08;			  
		data |= (GPIOGetBit(pDevice->hGpio,BSP_KEY5_GPIO)) ? 0 : 0x10;			  
		data |= (GPIOGetBit(pDevice->hGpio,BSP_KEY6_GPIO)) ? 0 : 0x20;			  

		//--------------------------------------------------------------
        // Convert physical state to virtual keys state
        //--------------------------------------------------------------
        // Get new state for virtual key table
        memset(vkNewState, 0, sizeof(vkNewState));
        keyDown = FALSE;

        for (i = 0; i < NB_KEYS; i++){
            if ((data & (1 << i)) != 0){
                vk = g_keypadVK[i].vkey;
                vkNewState[vk >> 5] |= 1 << (vk & 0x1F);
                keyDown = TRUE;
                DEBUGMSG(ZONE_INFO, (L"keyDown = TRUE \r\n"));
            }            
        }

        //--------------------------------------------------------------
        // Remap multi virtual keys to final virtual key
        //--------------------------------------------------------------
        time = GetTickCount();
        for (ix = 0; ix < g_keypadRemap.count; ix++) {
            const KEYPAD_REMAP_ITEM *pItem = &g_keypadRemap.pItem[ix];
            KEYPAD_REMAP_STATE *pState = &pRemapState[ix];
            DWORD state = 0;
            USHORT down = 0;

            // Count number of keys down & save down/up state
            for (i = 0; i < pItem->keys; i ++) {
                vk = pItem->pVKeys[i];
                if ((vkNewState[vk >> 5] & (1 << (vk & 0x1F))) != 0) {
                    state |= 1 << i;
                    down++;
                }
            }
            // Depending on number of keys down
            if (down >= pItem->keys && pItem->keys > 1) {
                // Clear all mapping keys
                for (i = 0; i < pItem->keys; i++) {
                    vk = pItem->pVKeys[i];
                    vkNewState[vk >> 5] &= ~(1 << (vk & 0x1F));
                }
                // All keys are down set final key
                vk = pItem->vkey;
                vkNewState[vk >> 5] |= 1 << (vk & 0x1F);
                DEBUGMSG(ZONE_IST, (L" KPD_Thread: Mapped vkey: 0x%x\r\n", vk));
                pState->pending = FALSE;          // Clear remap pending flag
                pState->remapped = TRUE;          // Set remap processing flag
            } else if (down > 0) {
                // If already remapping or remapping is not pending or pending time expired
                if (pState->remapped || !pState->pending ||
                    (INT32)(time - pState->time) < 0 ) {
                    // If we are not pending and not already remapping, start
                    if (!pState->pending && !pState->remapped) {
                        pState->pending = TRUE;
                        pState->time = time + pItem->delay;
                    }
                } else if (
                    pItem->keys == 1 && (INT32)(time - pState->time) >= 0
                ) {
                    // This is press and hold key
                   DEBUGMSG(ZONE_IST, (L" KPD_Thread: Mapped press and hold vkey: 0x%x\r\n", vk));
                    vk = pItem->vkey;
                    vkNewState[vk >> 5] |= 1 << (vk & 0x1F);
                    keyDown = TRUE;
                    pState->pending = FALSE;
                }
            } else {
                // All keys are up, if remapping was pending set keys
                if (pState->pending) {
                    for (i = 0; i < pItem->keys; i++) {
                        if ((pState->state & (1 << i)) != 0) {
                            vk = pItem->pVKeys[i];
                            vkNewState[vk >> 5] |= 1 << (vk & 0x1F);
                            DEBUGMSG(ZONE_INFO, (L"keyDown = TRUE 2\r\n"));
                            keyDown = TRUE;
                        }
                    }
                    pState->pending = FALSE;
                }
                pState->remapped = FALSE;
            }
            // Save key state
            pState->state = state;
        }

        //--------------------------------------------------------------
        // Find pressed/released keys
        //--------------------------------------------------------------
        for (ic = 0, vk = 0; ic < VK_KEYS/DWORD_BITS; ic++) {
            change = vkState[ic] ^ vkNewState[ic];
            if (change == 0) {
                vk += DWORD_BITS;
            } else for (mask = 1; mask != 0; mask <<= 1, vk++) {
                // Check for change
                if ((change & mask) != 0) {
                    if ((vkNewState[ic] & mask) != 0){
                        DEBUGMSG(ZONE_IST, (L" KPD_Thread: Key Down: 0x%x\r\n", vk ));
                        keybd_event(vk, 0, 0, 0);

                        //Notify for power manager events
                        if(vk == VK_TPOWER){
                            PowerPolicyNotify(PPN_POWERBUTTONPRESSED, 0);
                        } else {
                            //Application button pressed
                            //PM uses this to indicate user activity and reset timers
                            PowerPolicyNotify(PPN_APPBUTTONPRESSED, 0);
                        }

                        if(vk == VK_TSTAR)
                            DEBUGMSG(ZONE_IST, (L"VK_TSTAR\r\n"));
                    } else {
                        DEBUGMSG(ZONE_IST, (L" KPD_Thread: Key Up: 0x%x\r\n", vk ));
                        keybd_event(vk, 0, KEYEVENTF_KEYUP, 0);
                    }
                }
            }
        }

        //--------------------------------------------------------------
        //  Check for auto-repeat keys
        //--------------------------------------------------------------
        for (ix = 0; ix < g_keypadRepeat.count; ix++) {
            const KEYPAD_REPEAT_ITEM *pItem = &g_keypadRepeat.pItem[ix];
            const KEYPAD_REPEAT_BLOCK *pBlock = pItem->pBlock;
            KEYPAD_REPEAT_STATE *pState = &pRepeatState[ix];
            DWORD delay;
            UCHAR vkBlock;

            vk = pItem->vkey;
            if ((vkNewState[vk >> 5] & (1 << (vk & 0x1F))) != 0) {
                if (!pState->pending) {
                    // Key was just pressed
                    delay = pItem->firstDelay;
                    if (delay == 0) delay = pDevice->firstRepeat;
                    pState->time = time + delay;
                    pState->pending = TRUE;
                    pState->blocked = FALSE;
                } else if ((INT32)(time - pState->time) >= 0) {
                    // Check if any blocking keys are pressed
                    if (pBlock != 0) {
                        for (i = 0; i < pBlock->count; i++) {
                            vkBlock = pBlock->pVKey[i];
                            if ((vkNewState[vkBlock >> 5] & (1 << (vkBlock & 0x1F))) != 0) {
                                pState->blocked = TRUE;
                                DEBUGMSG(ZONE_IST, (L" KPD_Thread: Block repeat: 0x%x because of 0x%x\r\n",vk, vkBlock));
                                break;
                            }
                        }
                    }
                    // Repeat if not blocked
                    if (!pState->blocked) {
                        DEBUGMSG(ZONE_IST, (L" KPD_Thread: Key Repeat: 0x%x\r\n", vk));
                        keybd_event(vk, 0, pItem->silent?KEYEVENTF_SILENT:0, 0);
                    }
                    // Set time for next repeat
                    delay = pItem->nextDelay;
                    if (delay == 0) delay = pDevice->nextRepeat;
                    pState->time = time + delay;
                }
            } else {
                pState->pending = FALSE;
                pState->blocked = FALSE;
            }
        }

        DEBUGMSG(ZONE_IST, (L" KPD_Thread: Prepare for next run\r\n"));
        memcpy(vkState, vkNewState, sizeof(vkState)); // New state become old

    }

cleanUp:

    if (pRemapState != NULL) LocalFree(pRemapState);
    if (pRepeatState != NULL) LocalFree(pRepeatState);
	return ERROR_SUCCESS;
}

//------------------------------------------------------------------------------
BOOL __stdcall DllMain(HANDLE hDLL, DWORD reason, VOID *pReserved)
{
    UNREFERENCED_PARAMETER(pReserved);
    switch (reason) {
    case DLL_PROCESS_ATTACH:
        DEBUGREGISTER(hDLL);
        DisableThreadLibraryCalls((HMODULE)hDLL);
        break;
    }
    return TRUE;
}

//------------------------------------------------------------------------------
