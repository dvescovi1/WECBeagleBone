// All rights reserved ADENEO EMBEDDED 2010

#include <windows.h>
#include <oal_io.h>

//------------------------------------------------------------------------------
//
//  Function:  ISP_InReg32
//
//  Read data from register
//      
BOOL ISP_InReg32(
                    volatile UINT32* address,
                    UINT32* data
                    )
{
    *data = INREG32(address);
    return TRUE;
}

//------------------------------------------------------------------------------
//
//  Function:  ISP_OutReg32
//
//  Output data to register
//      
BOOL ISP_OutReg32(
                    volatile UINT32* address,
                    UINT32 value
                    )
{

    OUTREG32(address, value);
    return TRUE;
}
