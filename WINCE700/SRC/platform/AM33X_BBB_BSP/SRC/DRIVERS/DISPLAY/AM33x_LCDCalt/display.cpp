//------------------------------------------------------------------------------
//
//  File:  display.cpp
//
#include <windows.h>
#include <types.h>
#include <winddi.h>
#include <ddgpe.h>
#include <emul.h>
#include <ceddk.h>
#include <ceddk.h>

#include <bsp.h>
#include <gpio.h>
#include "display.h"


//------------------------------------------------------------------------------
// Global Variable
//

//------------------------------------------------------------------------------

#define DISPLAY_CONTEXT_COOKIE      'dspX'

typedef struct {
    DWORD cookie;
    CEDEVICE_POWER_STATE currentDX;
} DISPLAY_CONTEXT;

//------------------------------------------------------------------------------
//
//  Function:  DisplayPddInit
//
//  This function initialises the display.
//
HANDLE DisplayPddInit(LPCWSTR context)
{
    DISPLAY_CONTEXT *pDisplay;

    DEBUGMSG(GPE_ZONE_INIT, ((L"DisplayPddInit: %S %S\r\n"), __DATE__, __TIME__));

    //DEBUGMSG(GPE_ZONE_INIT, ((L"DisplayPddInit: Set GPIO_40 2[8] as output\r\n")));    
    //GPIOSetDirection(GPIO_40, GPIO_DIRECTION_OUTPUT);
    //GPIOSetOutputState(GPIO_40, GPIO_STATE_HIGH);
    //
    //DEBUGMSG(GPE_ZONE_INIT, ((L"DisplayPddInit: Set GPIO_47 2[15] as output\r\n")));    
    //GPIOSetDirection(GPIO_47, GPIO_DIRECTION_OUTPUT);
    //GPIOSetOutputState(GPIO_47, GPIO_STATE_HIGH);
    //
    //DEBUGMSG(GPE_ZONE_INIT, ((L"DisplayPddInit: Set GPIO_46 2[14] as output\r\n")));    
    //GPIOSetDirection(GPIO_46, GPIO_DIRECTION_OUTPUT);
    //GPIOSetOutputState(GPIO_46, GPIO_STATE_HIGH);

    // Create device structure
    pDisplay = (DISPLAY_CONTEXT *)LocalAlloc(LPTR, sizeof(DISPLAY_CONTEXT));
    if (pDisplay == NULL) {
        DEBUGMSG(GPE_ZONE_ERROR, (L"ERROR: DisplayPddInit: "
            L"Failed allocate display context structure\r\n"
        ));
        goto cleanUp;
    }

    // Set cookie
    pDisplay->cookie = DISPLAY_CONTEXT_COOKIE;
    
    // Display is in D0 power state now
    pDisplay->currentDX = D0;

cleanUp:
    return (HANDLE)pDisplay;
}

//------------------------------------------------------------------------------
//
//  Function:  DisplayPddDeinit
//
//  This function de-initializes the display. 
//
VOID DisplayPddDeinit(HANDLE hContext)
{
    DISPLAY_CONTEXT *pDisplay = (DISPLAY_CONTEXT*)hContext;
    
    // Check if we get correct context
    if (pDisplay == NULL || pDisplay->cookie != DISPLAY_CONTEXT_COOKIE) {
        DEBUGMSG(GPE_ZONE_ERROR, (L"ERROR: DisplayPddDeinit: "
            L"Incorrect context paramer\r\n"
        ));
        goto cleanUp;
    }

    // Free device structure
    LocalFree(pDisplay);

cleanUp:
    return;
}

//------------------------------------------------------------------------------
//
//  Function:  DisplayPddNumModes
//
//  This method returns number of different hardware modes supported by
//  LCD display hardware. In most cases it will be only one mode.
//  
int DisplayPddNumModes(HANDLE hContext)
{
    // We support only one mode
    return 1;
}

//------------------------------------------------------------------------------
//
//  Function:  DisplayPddSetMode
//
//  This function should set LCD display hardware to given mode.
//
BOOL DisplayPddSetMode(HANDLE hContext, int modeNumber)
{
    BOOL rc = FALSE;

    if (modeNumber != 0) goto cleanUp;

    // Nothing for the PDD to do

    // Done
    rc = TRUE;

cleanUp:
    return rc;
}


//------------------------------------------------------------------------------
//
//  Function:  DisplayPddEnableDisplay
//
//  This function performs any post-controller init LCD display enable
//
BOOL DisplayPddEnableDisplay(HANDLE hContext, int modeNumber)
{
    BOOL rc = FALSE;

    if (modeNumber != 0) goto cleanUp;

    // Nothing for the PDD to do

    // Done
    rc = TRUE;

cleanUp:
    return rc;
}


//------------------------------------------------------------------------------
//
//  Function:  DisplayPddSetPower
//
VOID DisplayPddSetPower(HANDLE hContext, CEDEVICE_POWER_STATE dx)
{
    DISPLAY_CONTEXT *pDisplay = (DISPLAY_CONTEXT*)hContext;

    pDisplay->currentDX = dx;

    // Power on/off backlight
    if (dx <= D2)
    {     
        DEBUGMSG(GPE_ZONE_INIT, (L"DisplayPddSetPower: Switching on backlight\r\n"));
        //GPIOSetDirection(GPIO_47, GPIO_DIRECTION_OUTPUT);
        //GPIOSetOutputState(GPIO_47, GPIO_STATE_HIGH);
    }
    else
    {
        DEBUGMSG(GPE_ZONE_INIT, (L"DisplayPddSetPower: Switching off backlight\r\n"));
        //GPIOSetDirection(GPIO_47, GPIO_DIRECTION_OUTPUT);
        //GPIOSetOutputState(GPIO_47, GPIO_STATE_LOW);
    }

    return;
}

//------------------------------------------------------------------------------
//
//  Function:  DisplayPddPowerHandler
//
VOID DisplayPddPowerHandler(HANDLE hContext, BOOL off)
{
    // Nothing for the PDD to do

    return;
}

//------------------------------------------------------------------------------
