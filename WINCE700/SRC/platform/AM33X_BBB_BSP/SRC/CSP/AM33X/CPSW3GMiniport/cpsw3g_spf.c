//========================================================================
//   Copyright (c) Texas Instruments Incorporated 2008-2009
//
//   Use of this software is controlled by the terms and conditions found
//   in the license agreement under which this software has been supplied
//   or provided.
//========================================================================

#include "cpsw3g_miniport.h"

VOID Cpsw3g_SPF_init( PCPSW3G_ADAPTER pAdapter)
{
#if 0
    pAdapter->Spf_intr_event = CreateEvent(NULL, FALSE, FALSE, L"AM33X_SPF_INTR_EVENT");


    if (!pAdapter->Spf_intr_event) {
        RETAILMSG(TRUE, (L"Cpsw3g_SPF_init: Create event failed\r\n"));
    }
    else
        RETAILMSG(TRUE,(L"Cpsw3g_SPF_init: event=%x\r\n", pAdapter->Spf_intr_event));
#endif
}


