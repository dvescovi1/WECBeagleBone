//===================================================================
//
//  Module Name:    NLED.DLL
//
//  File Name:      nleddrv.c
//
//  Description:    Control of the notification LED(s)
//
//===================================================================
// Copyright (c) 2007- 2009 BSQUARE Corporation. All rights reserved.
//===================================================================

//-----------------------------------------------------------------------------
//
// header files
//
//-----------------------------------------------------------------------------

#pragma warning(push)
#pragma warning(disable : 4115 4214)
#include <windows.h>
#include <winuser.h>
#include <winuserm.h>
#include <ceddk.h>
#include <ceddkex.h>
#include <nled.h>
#include <led_drvr.h>
#include <initguid.h>
#pragma warning(pop)

#include "omap.h"
#include "sdk_gpio.h"
#include "oalex.h"


//-----------------------------------------------------------------------------
//
// Local Macros
//
//-----------------------------------------------------------------------------

#define ENABLE_DEBUG_MESSAGES                   TRUE

//-----------------------------------------------------------------------------
//
// private data
//
//-----------------------------------------------------------------------------

static HANDLE g_hGPIO = NULL;

// Array to hold current Blink Params
struct NLED_SETTINGS_INFO *g_BlinkParams;
// Array to hold current LED settings
int *g_NLedCurrentState;
// Array to hold event handles for waking each LED Thread
static HANDLE *g_hLedHandle;
static HANDLE *g_hNewThread;
// flag to indicate thread should exit
// Shared by all threads
static BOOL g_bExitThread = FALSE;


//-----------------------------------------------------------------------------
//
// externals
//
//-----------------------------------------------------------------------------
extern DWORD g_GPIOId[];
extern DWORD g_GPIOActiveState[];
extern DWORD g_dwNbLeds;
extern BOOL  g_LastLEDIsVibrator;
extern int   NLedCpuFamily;


extern BOOL NLedBoardInit();
extern BOOL NLedBoardDeinit();


// Helper function used to set state of NLED control bits
static void NLedDriverSetLedState(UINT LedNum, int LedState)
{
    if (LedNum >= g_dwNbLeds)
    {
        DEBUGMSG(ZONE_ERROR, (TEXT("ERROR: NLedDriverSetLedState: invalid NLED number: %d\r\n"), LedNum));
        return;
    }

    switch (LedState)
    {
        case 0:
            // turn LED off 
            if (g_GPIOActiveState[LedNum])
                GPIOClrBit(g_hGPIO, g_GPIOId[LedNum]);
            else
                GPIOSetBit(g_hGPIO, g_GPIOId[LedNum]);
            break;

        case 1:
            // turn LED on
            if (g_GPIOActiveState[LedNum])
                GPIOSetBit(g_hGPIO, g_GPIOId[LedNum]);
            else
                GPIOClrBit(g_hGPIO, g_GPIOId[LedNum]);
            break;
    }
}

int NLedControlThread(
    UINT    NLedID
    )
{
    int i;

    // turn NLED off
    NLedDriverSetLedState(NLedID, 0);

    for(;;)
    {
        StateChangeDueToApiCall:

        if (g_bExitThread == TRUE)
            break;

        // check unchanged NLED state
        if ( g_NLedCurrentState[NLedID] == g_BlinkParams[NLedID].OffOnBlink )
        {
            // handle blinking state
            if ( g_NLedCurrentState[NLedID] == 2 )
            {
                // Do meta cycle on blinks (just do regular blink cycle if Meta is 0)
                //DEBUGMSG(ZONE_FUNCTION, (TEXT("NLED%d MetaCycleOn %d periods\r\n"), NLedID, g_BlinkParams[NLedID].MetaCycleOn));
                for (i = 0; i < (g_BlinkParams[NLedID].MetaCycleOn == 0 ? 1 : g_BlinkParams[NLedID].MetaCycleOn); i++)
                {
                    // turn NLED on
                    NLedDriverSetLedState(NLedID, 1);

                    // wait for on time (or change from API call)
                    if ( WaitForSingleObject(g_hLedHandle[NLedID], (g_BlinkParams[NLedID].OnTime / 1000)) == WAIT_OBJECT_0 )
                        goto StateChangeDueToApiCall;

                    // turn NLED off
                    NLedDriverSetLedState(NLedID, 0);

                    // wait for off time (or change from API call)
                    if ( WaitForSingleObject(g_hLedHandle[NLedID], ((g_BlinkParams[NLedID].TotalCycleTime - g_BlinkParams[NLedID].OnTime) / 1000)) == WAIT_OBJECT_0 )
                        goto StateChangeDueToApiCall;
                }

                // check for meta off specified, wait for that time period (or change from API call)
                //DEBUGMSG(ZONE_FUNCTION, (TEXT("NLED%d MetaCycleOff %d periods\r\n"), NLedID, g_BlinkParams[NLedID].MetaCycleOff));
                if ( g_BlinkParams[NLedID].MetaCycleOff > 0 )
                {
                    if ( WaitForSingleObject(g_hLedHandle[NLedID], (((g_BlinkParams[NLedID].OffTime + g_BlinkParams[NLedID].OnTime) / 1000) * g_BlinkParams[NLedID].MetaCycleOff)) == WAIT_OBJECT_0 )
                        goto StateChangeDueToApiCall;
                }
            }
            else
            {
                // LED state unchanged, do nothing for on or off state except wait for API change
                WaitForSingleObject(g_hLedHandle[NLedID], INFINITE);
                #if ENABLE_DEBUG_MESSAGES
                    DEBUGMSG(ZONE_FUNCTION, (TEXT("NLED # %d driver thread awakened:\r\n"), NLedID));
                #endif
            }
        }
        else
        {
            // LED state changed, update NLED for new state
            #if ENABLE_DEBUG_MESSAGES
                DEBUGMSG(ZONE_FUNCTION, (TEXT("NLED # %d driver thread: Mode change to %d\r\n"), NLedID, g_BlinkParams[NLedID].OffOnBlink));
            #endif
            if ( g_BlinkParams[NLedID].OffOnBlink == 0 )
            {
                NLedDriverSetLedState(NLedID, 0);
                g_NLedCurrentState[NLedID] = 0;
            }
            if ( g_BlinkParams[NLedID].OffOnBlink == 1 )
            {
                NLedDriverSetLedState(NLedID, 1);
                g_NLedCurrentState[NLedID] = 1;
            }
            if ( g_BlinkParams[NLedID].OffOnBlink == 2 )
                g_NLedCurrentState[NLedID] = 2;
        }
    }

    DEBUGMSG(ZONE_INIT, (TEXT("NLedControlThread exiting!!\r\n"), NLedID));

    return 0;
}


BOOL
WINAPI
NLedDriverGetDeviceInfo(
    INT     nInfoId,
    PVOID   pOutput
    )
{
    if ( nInfoId == NLED_COUNT_INFO_ID )
    {
        struct NLED_COUNT_INFO * p = (struct NLED_COUNT_INFO*)pOutput;

        if (p == NULL)
            goto ReturnError;

        // Fill in number of leds
        p->cLeds = g_dwNbLeds;
        DEBUGMSG(ZONE_INIT, (TEXT("NLEDDRV: NLedDriverGetDeviceInfo(NLED_COUNT_INFO_ID...) returning %d NLEDs\n"), g_dwNbLeds));
        return TRUE;
    }

    if ( nInfoId == NLED_SUPPORTS_INFO_ID )
    {
        struct NLED_SUPPORTS_INFO * p = (struct NLED_SUPPORTS_INFO *)pOutput;

        if (p == NULL)
            goto ReturnError;

        if ( p->LedNum >= g_dwNbLeds )
            goto ReturnError;

        // Fill in LED capabilities
        p->lCycleAdjust = 1000;         // Granularity of cycle time adjustments (microseconds)
        p->fAdjustTotalCycleTime = TRUE;    // LED has an adjustable total cycle time
        p->fAdjustOnTime = TRUE;            // @FIELD   LED has separate on time
        p->fAdjustOffTime = TRUE;           // @FIELD   LED has separate off time
        p->fMetaCycleOn = TRUE;             // @FIELD   LED can do blink n, pause, blink n, ...
        p->fMetaCycleOff = TRUE;            // @FIELD   LED can do blink n, pause n, blink n, ...

        // override individual LED capabilities
        if (g_LastLEDIsVibrator)
        {
            if (p->LedNum == (g_dwNbLeds - 1))
            {
                // vibrate must be last NLED, reports special lCycleAdjust value
                p->lCycleAdjust = -1;               // Well that was obvious!
                p->fAdjustTotalCycleTime = FALSE;   // LED has an adjustable total cycle time
                p->fAdjustOnTime = FALSE;           // @FIELD   LED has separate on time
                p->fAdjustOffTime = FALSE;          // @FIELD   LED has separate off time
                p->fMetaCycleOn = FALSE;            // @FIELD   LED can do blink n, pause, blink n, ...
                p->fMetaCycleOff = FALSE;           // @FIELD   LED can do blink n, pause n, blink n, ...
            }
        }
        return TRUE;
    }
    else if ( nInfoId == NLED_SETTINGS_INFO_ID )
    {
        struct NLED_SETTINGS_INFO * p = (struct NLED_SETTINGS_INFO *)pOutput;

        if (p == NULL)
            goto ReturnError;

        if ( p->LedNum >= g_dwNbLeds )
            goto ReturnError;

        // Fill in current LED settings

        // Get any individual current settings
        p->OffOnBlink = g_BlinkParams[p->LedNum].OffOnBlink;
        p->TotalCycleTime = g_BlinkParams[p->LedNum].TotalCycleTime;
        p->OnTime = g_BlinkParams[p->LedNum].OnTime;
        p->OffTime = g_BlinkParams[p->LedNum].OffTime;
        p->MetaCycleOn = g_BlinkParams[p->LedNum].MetaCycleOn;
        p->MetaCycleOff = g_BlinkParams[p->LedNum].MetaCycleOff;

        return TRUE;
    }

ReturnError:

    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
}


BOOL
WINAPI
NLedDriverSetDevice(
    INT     nInfoId,
    PVOID   pInput
    )
{
    struct NLED_SETTINGS_INFO * p = (struct NLED_SETTINGS_INFO *)pInput;

    if ( nInfoId == NLED_SETTINGS_INFO_ID )
    {
        if (pInput == NULL)
            goto ReturnError;

        if ( p->LedNum >= g_dwNbLeds )
            goto ReturnError;

        // check for invalid parameters
        if ( p->OffOnBlink < 0 || p->OffOnBlink > 2 )
            goto ReturnError;

        // for blink state, check for valid times (why were integers used?)
        if ( p->OffOnBlink == 2 )
            if (p->TotalCycleTime < p->OnTime ||
                p->TotalCycleTime < p->OffTime ||
                p->MetaCycleOn < 0 ||
                p->MetaCycleOff < 0 ||
                p->TotalCycleTime < 0 ||
                p->OnTime < 0 ||
                p->OffTime < 0 ||
                p->TotalCycleTime < p->OnTime + p->OffTime
            )
                goto ReturnError;

        // check for any changed NLED settings
        if ( g_BlinkParams[p->LedNum].OffOnBlink != p->OffOnBlink ||
             g_BlinkParams[p->LedNum].TotalCycleTime != p->TotalCycleTime ||
             g_BlinkParams[p->LedNum].OnTime != p->OnTime ||
             g_BlinkParams[p->LedNum].OffTime != p->OffTime ||
             g_BlinkParams[p->LedNum].MetaCycleOn != p->MetaCycleOn ||
             g_BlinkParams[p->LedNum].MetaCycleOff != p->MetaCycleOff
        )
        {
            // Update NLED settings
            g_BlinkParams[p->LedNum].OffOnBlink = p->OffOnBlink;
            g_BlinkParams[p->LedNum].TotalCycleTime = p->TotalCycleTime;
            g_BlinkParams[p->LedNum].OnTime = p->OnTime;
            g_BlinkParams[p->LedNum].OffTime = p->OffTime;
            g_BlinkParams[p->LedNum].MetaCycleOn = p->MetaCycleOn;
            g_BlinkParams[p->LedNum].MetaCycleOff = p->MetaCycleOff;

            // wake up appropriate NLED thread
            #if ENABLE_DEBUG_MESSAGES
                DEBUGMSG(ZONE_FUNCTION, (TEXT("NLED # %d change signaled\r\n"), p->LedNum));
            #endif

            SetEvent(g_hLedHandle[p->LedNum]);
        }

        return TRUE;
    }

ReturnError:

    #if ENABLE_DEBUG_MESSAGES
        if (p != NULL)
            DEBUGMSG(ZONE_ERROR, (TEXT("NLED: NLedDriverSetDevice: NLED %x, Invalid parameter\r\n"), p->LedNum));
    #endif

    SetLastError(ERROR_INVALID_PARAMETER);

    return FALSE;
}


// Note: This function is called by the power handler, it must not make
// any system calls other than the few that are specifically allowed
// (such as SetInterruptEvent()).
VOID
NLedDriverPowerDown(
   BOOL power_down
   )
{
    UINT NledNum;

    if ( power_down )
    {
        // shut off all NLEDs
        for (NledNum = 0; NledNum < g_dwNbLeds; NledNum++)
        {
            NLedDriverSetLedState(NledNum, 0);
            g_NLedCurrentState[NledNum] = 0;
        }
    }
    else
    {
        for (NledNum = 0; NledNum < g_dwNbLeds; NledNum++)
        {
            // On Power Up (Resume) turn on any LEDs that should be "ON".
            //  the individual LED control threads will put the Blinking
            //  LEDs back in their proper pre suspend state while "OFF" LEDs
            //  will stay off.
            if ( g_BlinkParams[NledNum].OffOnBlink == 1 )
            {
                NLedDriverSetLedState(NledNum, 1);
                g_NLedCurrentState[NledNum] = 1;
            }
        }
    }
}


BOOL
NLedDriverInitialize(
    VOID
    )
{
    BOOL bResult = TRUE;

    DWORD i;

    if( NLedCpuFamily == CPU_FAMILY_DM37XX)
    {
        g_dwNbLeds=0;
    }

    if (g_dwNbLeds <=0)
    {
        return FALSE;
    }

    if (NLedBoardInit() == FALSE)
    {
        return FALSE;
    }
    
    // Open gpio driver
    g_hGPIO = GPIOOpen();
    if (g_hGPIO == NULL)
    {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: NLedDriverInitialize: Failed to open Gpio driver!\r\n"));
        return FALSE;
    }
    g_BlinkParams = (struct NLED_SETTINGS_INFO*) malloc(g_dwNbLeds*sizeof(struct NLED_SETTINGS_INFO));
    g_NLedCurrentState = (int*) malloc(g_dwNbLeds*sizeof(int));
    g_hLedHandle = (HANDLE*) malloc(g_dwNbLeds*sizeof(HANDLE));
    g_hNewThread = (HANDLE*) malloc(g_dwNbLeds*sizeof(HANDLE));



    // initialize the NLED state array and blink parameter structure
    for (i = 0; i < g_dwNbLeds; i++)
    {
        g_BlinkParams[i].LedNum = i;
        g_BlinkParams[i].OffOnBlink = 0;
        g_BlinkParams[i].TotalCycleTime = 0;
        g_BlinkParams[i].OnTime = 0;
        g_BlinkParams[i].OffTime = 0;
        g_BlinkParams[i].MetaCycleOn = 0;
        g_BlinkParams[i].MetaCycleOff = 0;
        g_NLedCurrentState[i] = 0;
    }

    for ( i = 0; i < g_dwNbLeds; i++ )
    {
        g_hLedHandle[i] = NULL;
        g_hNewThread[i] = NULL;
    }

    // make NLED GPIO pins outputs
    for ( i = 0; i < g_dwNbLeds; i++ )
    {
        GPIOSetMode(g_hGPIO, g_GPIOId[i], GPIO_DIR_OUTPUT);
    }
        
    for ( i = 0; i < g_dwNbLeds; i++ )
    {
        g_hLedHandle[i] = CreateEvent(0, FALSE, FALSE, NULL);
        g_hNewThread[i] = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)NLedControlThread, (LPVOID)i, 0, NULL);

        if ( g_hNewThread[i] == NULL || g_hLedHandle[i] == NULL)
        {
            DEBUGMSG(ZONE_ERROR, (TEXT("ERROR: NLedDriverInitialize: Could not create event and/or start thread.\r\n")));
            bResult = FALSE;
        }
        else
        {
            DEBUGMSG(ZONE_INIT, (TEXT("NLedDriverInitialize: NLED # %d driver thread: (0x%X) started.\r\n"), i, g_hNewThread[i]));
        }
    }

    return bResult;
}

BOOL
NLedDriverDeInitialize(
    VOID
    )
{
    DWORD i;

    DEBUGMSG(ZONE_INIT, (TEXT("NLEDDRV:  NLedDriverDeInitialize() Unloading driver...\r\n")));

    // Stop all threads, close handles
    g_bExitThread = TRUE;
    for ( i = 0; i < g_dwNbLeds; i++ )
    {
        if (g_hLedHandle[i])
            SetEvent(g_hLedHandle[i]);

        if (g_hNewThread[i])
        {
            WaitForSingleObject(g_hNewThread[i], INFINITE);
            CloseHandle(g_hNewThread[i]);
        }

        if (g_hLedHandle[i])
            CloseHandle(g_hLedHandle[i]);

        g_hNewThread[i] = NULL;
        g_hLedHandle[i] = NULL;
    }

    free(g_BlinkParams);
    free(g_NLedCurrentState);
    free(g_hLedHandle);
    free(g_hNewThread);

    GPIOClose(g_hGPIO);
    
    NLedBoardDeinit();

    return TRUE;
}
