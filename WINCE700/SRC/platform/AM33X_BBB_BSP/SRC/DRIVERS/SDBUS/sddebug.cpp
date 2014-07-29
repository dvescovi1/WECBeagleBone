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
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//
//
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Module Name:  
    SDDebug.cpp
Abstract:
    SDBus Client Driver Interface Implementation.

Notes: 
--*/


#include <windows.h>
#include <bldver.h>
#include <cesdbus.h>
#include <marshal.hpp>


extern "C" {

#ifdef DEBUG
DBGPARAM dpCurSettings = {
    _T("SDBus"), {
            _T("HC Driver Load"),_T("Dispatcher"),_T("Shutdown"),_T("Power"),
            _T("Device Load"),_T("Bus Requests"),_T("Buffer Dumps"),_T("Soft-Block"),
            _T(""), _T(""), _T(""), _T(""),
            _T(""), _T("Init"), _T("Warnings"), _T("Errors") },
            ZONE_ENABLE_INIT | ZONE_ENABLE_ERROR | ZONE_ENABLE_WARN };
#endif

///////////////////////////////////////////////////////////////////////////////
//  SDCardDebugOutput - Debug output function 
//  Input:  
//          pDebugText - debug text to output
//          ...  - variable argument list
//  Output: 
//  Return:
//  Notes: 
///////////////////////////////////////////////////////////////////////////////
VOID SDCardDebugOutput(TCHAR const*const pDebugText, ...)
{
#ifndef SHIP_BUILD // Not DEBUG since this is used by DbgPrintRetail
    va_list     argList;                                        // argument list
    TCHAR       debugBuffer[MAXIMUM_DEBUG_STRING_LENGTH];       // maximum size of debug buffer

    // get the argument list        
    va_start(argList, pDebugText);
    // print it to our debug buffer
    VERIFY(SUCCEEDED(StringCchVPrintf(debugBuffer, _countof(debugBuffer), pDebugText, argList)));
    va_end(argList);

    OutputDebugString(debugBuffer);
#endif
}

///////////////////////////////////////////////////////////////////////////////
//  SDProcessException - main SD exception handler
//  Input:  
//          pException - exception record passed in from the kernel
//  Output: 
//  Return:
//  Notes:  
///////////////////////////////////////////////////////////////////////////////
DWORD SDProcessException(LPEXCEPTION_POINTERS pException) 
{

    EXCEPTION_RECORD *pExceptionRecord;

    pExceptionRecord = pException->ExceptionRecord;

    while(pExceptionRecord != NULL) {
        RETAILMSG(1, (TEXT("SDBusDriver: Exception caught ExceptionCode:0x%08X, flags:0x%08X, Code Address 0x%08X \n"), 
            pExceptionRecord->ExceptionCode, pExceptionRecord->ExceptionFlags,
            pExceptionRecord->ExceptionAddress));

        switch(pExceptionRecord->ExceptionCode) {

            case  EXCEPTION_ACCESS_VIOLATION :
                RETAILMSG(1, ((TEXT("        Exception: EXCEPTION_ACCESS_VIOLATION \n" ))));
                break;
            case  EXCEPTION_DATATYPE_MISALIGNMENT :
                RETAILMSG(1, ((TEXT("        Exception: EXCEPTION_DATATYPE_MISALIGNMENT \n" ))));
                break;
            case  EXCEPTION_BREAKPOINT :
                RETAILMSG(1, ((TEXT("        Exception: EXCEPTION_BREAKPOINT \n" ))));
                break;
            case  EXCEPTION_SINGLE_STEP :
                RETAILMSG(1, ((TEXT("        Exception: EXCEPTION_SINGLE_STEP \n" ))));
                break;
            case  EXCEPTION_ARRAY_BOUNDS_EXCEEDED :
                RETAILMSG(1, ((TEXT("        Exception: EXCEPTION_ARRAY_BOUNDS_EXCEEDED \n" ))));
                break;
            case  EXCEPTION_FLT_DENORMAL_OPERAND  :
                RETAILMSG(1, ((TEXT("        Exception: EXCEPTION_FLT_DENORMAL_OPERAND \n" ))));
                break;
            case  EXCEPTION_FLT_DIVIDE_BY_ZERO  :
                RETAILMSG(1, ((TEXT("        Exception: EXCEPTION_FLT_DIVIDE_BY_ZERO \n" ))));
                break;
            case  EXCEPTION_FLT_INEXACT_RESULT  :
                RETAILMSG(1, ((TEXT("        Exception: EXCEPTION_FLT_INEXACT_RESULT \n" ))));
                break;
            case  EXCEPTION_FLT_INVALID_OPERATION  :
                RETAILMSG(1, ((TEXT("        Exception: EXCEPTION_FLT_INVALID_OPERATION \n" ))));
                break;
            case  EXCEPTION_FLT_OVERFLOW :
                RETAILMSG(1, ((TEXT("        Exception: EXCEPTION_FLT_OVERFLOW \n" ))));
                break;
            case  EXCEPTION_FLT_STACK_CHECK :
                RETAILMSG(1, ((TEXT("        Exception: EXCEPTION_FLT_STACK_CHECK \n" ))));
                break;
            case  EXCEPTION_FLT_UNDERFLOW :
                RETAILMSG(1, ((TEXT("        Exception: EXCEPTION_FLT_UNDERFLOW \n" ))));
                break;
            case  EXCEPTION_INT_DIVIDE_BY_ZERO:
                RETAILMSG(1, ((TEXT("        Exception: EXCEPTION_INT_DIVIDE_BY_ZERO \n" ))));
                break;
            case  EXCEPTION_INT_OVERFLOW :
                RETAILMSG(1, ((TEXT("        Exception: EXCEPTION_INT_OVERFLOW \n" ))));
                break;
            case  EXCEPTION_PRIV_INSTRUCTION :
                RETAILMSG(1, ((TEXT("        Exception: EXCEPTION_PRIV_INSTRUCTION \n" ))));
                break;
            case  EXCEPTION_IN_PAGE_ERROR :
                RETAILMSG(1, ((TEXT("        Exception: EXCEPTION_IN_PAGE_ERROR \n" ))));
                break;
            case  EXCEPTION_ILLEGAL_INSTRUCTION :
                RETAILMSG(1, ((TEXT("        Exception: EXCEPTION_ILLEGAL_INSTRUCTION \n" ))));
                break;
            case  EXCEPTION_NONCONTINUABLE_EXCEPTION :
                RETAILMSG(1, ((TEXT("        Exception: EXCEPTION_NONCONTINUABLE_EXCEPTION \n" ))));
                break;
            case  EXCEPTION_STACK_OVERFLOW :
                RETAILMSG(1, ((TEXT("        Exception: EXCEPTION_STACK_OVERFLOW \n" ))));
                break;
            case  EXCEPTION_INVALID_DISPOSITION :
                RETAILMSG(1, ((TEXT("        Exception: EXCEPTION_INVALID_DISPOSITION \n" ))));
                break;
            case  EXCEPTION_GUARD_PAGE :
                RETAILMSG(1, ((TEXT("        Exception: EXCEPTION_GUARD_PAGE \n" ))));
                break;
            case  EXCEPTION_INVALID_HANDLE :
                RETAILMSG(1, ((TEXT("        Exception: EXCEPTION_INVALID_HANDLE \n" ))));
                break;
            default:
                RETAILMSG(1, ((TEXT("        Exception: UNKNOWN \n" ))));

        }

        if (pExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION) {
            if (pExceptionRecord->NumberParameters >= 2) {
                // figure if this was a read or write and dump the virtual address
                if (pExceptionRecord->ExceptionInformation[0] == 0) {
                    RETAILMSG(1, (TEXT("        Read Access Exceptioned at VAddress : 0x%08X \n" ),pExceptionRecord->ExceptionInformation[1]));
                } else {
                    RETAILMSG(1, (TEXT("        Write Access Exceptioned at VAddress : 0x%08X \n" ),pExceptionRecord->ExceptionInformation[1]));
                }
            } else {
                RETAILMSG(1, (TEXT(" EXCEPTION_ACCESS_VIOLATION raised but not enough parameters ; %d \n" ),pExceptionRecord->NumberParameters));
            }
        }

        pExceptionRecord = pExceptionRecord->ExceptionRecord;    
    }

    return EXCEPTION_EXECUTE_HANDLER;
}

///////////////////////////////////////////////////////////////////////////////
//  SDPerformSafeCopy - perform a safe memory copy
//  Input:  pSource   - source data
//          Length    - number of bytes to copy
//  Output: pDestination - Destination of the copy
//  Return:  returns TRUE if the copy succeeded, FALSE if an exception  occured
//  Notes:  
//
///////////////////////////////////////////////////////////////////////////////
BOOLEAN SDPerformSafeCopy(
    __out_bcount(Length)    PVOID pDestination, 
    __in_bcount(Length)     const VOID *pSource,
                            ULONG Length)
{
    BOOLEAN success = FALSE;

    if ( !( ( (ULONG) pDestination + Length < Length) || ( (ULONG) pSource + Length < Length) ) ) {
        __try {
            // do the mem copy in a try except block
            memcpy(pDestination, pSource, Length);
            success = TRUE;
        } __except (SDProcessException(GetExceptionInformation())) {
            // Nothing to do.
        }
    }
    // else overflow would occur

    if (success == FALSE) {
        RETAILMSG(1, (TEXT("SDPerformSafeCopy (Dest: 0x%08X) (Src: 0x%08X) (Size:%d) access violation \n"), 
            pDestination, pSource, Length));
    }

    return success;
}

///////////////////////////////////////////////////////////////////////////////
//  SDOutputBuffer - dump buffer to debugger
//  Input:  
//          pBuffer - the buffer to dump
//          BufferSize - size of buffer in bytes
//  Output:
//  Return: 
//  Notes:  
//          This function prints the buffer to the debugger using
//          16 bytes per line and displays the ASCII character respresentation 
//          per line, if the character is not displayable, a '.' is used
///////////////////////////////////////////////////////////////////////////////
VOID SDOutputBuffer(__in_bcount(BufferSize) PVOID pBuffer, ULONG BufferSize) 
{
#ifndef SHIP_BUILD
#define OUTPUT_BYTES_PER_LINE 16
    TCHAR lineString[OUTPUT_BYTES_PER_LINE + 1];    // line string
    TCHAR debugString[256];                         // the debug string
    TCHAR byteString[4];                            // string representing the byte
    ULONG ii;                                       // line string index
    ULONG bufferIndex = 0;                          // current buffer index
    ULONG bytesInThisLine;                          // number of bytes in this line
    UCHAR byte;                                     // the byte

    if (0 == BufferSize || pBuffer == NULL) {
        return;
    }

    RETAILMSG(-1, (TEXT("----------------------------------------------------------------------------------------\n")));

    RETAILMSG(-1, (TEXT("SDOutputBuffer: 0x%08X , Size: %d bytes\n" ),pBuffer, BufferSize));

    // Caller is responsible for setting proper proc permissions for the buffer.
    // We will catch and ignore exceptions that occur in case the caller forgot
    // to do this.
    __try {
        while (BufferSize) {
            // start the line
            VERIFY(SUCCEEDED(StringCchPrintf(debugString, _countof(debugString), TEXT(" 0x%08X  "), bufferIndex)));   

            bytesInThisLine = min(BufferSize, OUTPUT_BYTES_PER_LINE);

            // build the line
            for (ii = 0; ii < OUTPUT_BYTES_PER_LINE; ii++) {   

                if (ii < bytesInThisLine) {
                    byte = ((PUCHAR)pBuffer)[bufferIndex];
                    // format char
                    VERIFY(SUCCEEDED(StringCchPrintf(byteString, _countof(byteString), TEXT("%02X "),byte)));

                    if ((byte >= 0x20) && 
                        (byte <= 0x7E)) {
                            // right justify the char to a TCHAR
                            lineString[ii] = (TCHAR)byte;
                        } else {
                            // convert to a period
                            lineString[ii] = TEXT('.');
                        }

                        bufferIndex++;
                } else {
                    // pad characters with spaces (3)
                    VERIFY(SUCCEEDED(StringCchCopy(byteString, _countof(byteString), TEXT("   "))));
                    // convert to a space
                    lineString[ii] = TEXT(' ');
                }

                // build up the debug string
                VERIFY(SUCCEEDED(StringCchCat(debugString, _countof(debugString),byteString)));         
            }

            // NULL terminate the line string
            lineString[OUTPUT_BYTES_PER_LINE] = 0;

            // put some spaces
            VERIFY(SUCCEEDED(StringCchCat(debugString,_countof(debugString),TEXT("   "))));  
            // now finish the debug string, by appeding the line string
            VERIFY(SUCCEEDED(StringCchCat(debugString, _countof(debugString), lineString)));  

            // we print one line because the debugger inserts a carriage return per print
            RETAILMSG(-1, (TEXT("%s\n" ),debugString));
            BufferSize -= bytesInThisLine;
        }
    }
    __except(SDProcessException(GetExceptionInformation())) {
        DEBUGMSG(SDCARD_ZONE_ERROR, (_T("SDCardLib: Exception in SDOutpuBuffer\n")));
        DEBUGCHK(FALSE);
    }

    RETAILMSG(-1, (TEXT("----------------------------------------------------------------------------------------\n")));

#endif // SHIP_BUILD
}

}



