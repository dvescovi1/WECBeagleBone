//
//  All rights reserved ADENEO EMBEDDED 2010
//  All rights reserved Texas Instruments, Inc. 2011
//
////////////////////////////////////////////////////////////////////////////////
//
//  Tux_SPI TUX DLL
//
//  Module: main.h
//          Header for all files in the project.
//
//  Revision History:
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __MAIN_H__
#define __MAIN_H__

////////////////////////////////////////////////////////////////////////////////
// Included files

#include <windows.h>
#include <stdlib.h>
#include <tchar.h>
#include <tux.h>
#include <kato.h>

////////////////////////////////////////////////////////////////////////////////
// Suggested log verbosities

#define LOG_EXCEPTION          0
#define LOG_FAIL               2
#define LOG_ABORT              4
#define LOG_SKIP               6
#define LOG_NOT_IMPLEMENTED    8
#define LOG_PASS              10
#define LOG_DETAIL            12
#define LOG_COMMENT           14
#define LOG_WARNING			  16

////////////////////////////////////////////////////////////////////////////////


#define __THIS_FILE__   TEXT("Tux_SPI.cpp")

#define DEFAULT_ERROR( TST, ACT );  \
    if( TST ) { \
        g_pKato->Log( LOG_FAIL, TEXT("FAIL in %s @ line %d: CONDITION( %s ) ACTION( %s )" ), \
                      __THIS_FILE__, __LINE__, TEXT( #TST ), TEXT( #ACT ) ); \
        ACT; \
    }

static BOOL SetTestOptions( LPTSTR lpszCmdLine );

/* ------------------------------------------------------------------------
	Global values
------------------------------------------------------------------------ */
extern DWORD       g_BufferSize;
extern DWORD       g_NbThreads;
extern DWORD	   g_Writes;
extern DWORD	   g_ChipSelect;
extern DWORD	   g_Port;

/* ------------------------------------------------------------------------
	Some constants
------------------------------------------------------------------------ */
static const  TCHAR CmdFlag         = (TCHAR)'-';
static const  TCHAR CmdSpace        = (TCHAR)' ';
static const  TCHAR CmdThread          = (TCHAR)'t';
static const  TCHAR CmdBuffer          = (TCHAR)'b';
static const  TCHAR CmdWrites          = (TCHAR)'w';
static const  TCHAR CmdChipSelect          = (TCHAR)'s';
static const  TCHAR CmdPort          = (TCHAR)'p';


#endif // __MAIN_H__
