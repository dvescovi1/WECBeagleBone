// All rights reserved ADENEO EMBEDDED 2010

#ifndef __UTIL_H
#define __UTIL_H


BOOL ISP_InReg32(
                volatile UINT32* address,
                UINT32* data
                );
                
BOOL ISP_OutReg32(
        volatile UINT32* address,
            UINT32 value
            );



#endif //__UTIL_H