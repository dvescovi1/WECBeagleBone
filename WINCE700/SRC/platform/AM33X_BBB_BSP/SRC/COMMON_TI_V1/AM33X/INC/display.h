//------------------------------------------------------------------------------
//
//  File:  display.h
//
#ifndef __DISPLAY_H
#define __DISPLAY_H


#ifdef __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------
//
//  Function:  DisplayPddInit
//
//  This function initializes LCD display hardware. It returns handle which
//  must be used in all DisplayXXX calls.
//
HANDLE DisplayPddInit(LPCWSTR context);

//------------------------------------------------------------------------------
//
//  Function:  DisplayPddDeinit
//
//  This function deinitializes LCD display hardware.
//
VOID DisplayPddDeinit(HANDLE hContext);

//------------------------------------------------------------------------------
//
//  Function:  DisplayPddNumModes
//
//  This function returns number of display modes supported by display hardware.
//
int DisplayPddNumModes(HANDLE hContext);

//------------------------------------------------------------------------------
//
//  Function:  DisplayPddSetMode
//
//  This function sets LCD display hardware to given mode.
//
BOOL DisplayPddSetMode(HANDLE hContext, int modeNumber);

//------------------------------------------------------------------------------
//
//  Function:  DisplayPddEnableDisplay
//
//  This function performs any post-controller init LCD display enable
//
BOOL DisplayPddEnableDisplay(HANDLE hContext, int modeNumber);

//------------------------------------------------------------------------------
//
//  Function:  DisplayPddSetPower
//
VOID DisplayPddSetPower(HANDLE hContext, CEDEVICE_POWER_STATE dx);

//------------------------------------------------------------------------------
//
//  Function:  DisplayPddPowerHandler
//
VOID DisplayPddPowerHandler(HANDLE hContext, BOOL off);

//------------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif
