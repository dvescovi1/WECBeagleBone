// All rights reserved ADENEO EMBEDDED 2010
//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// This source code is licensed under Microsoft Shared Source License
// Version 1.0 for Windows CE.
// For a copy of the license visit http://go.microsoft.com/fwlink/?LinkId=3223.
//

#include <windows.h>
#include <tchar.h>
#include <winbase.h>

#ifdef UNDER_CE
#include "types.h"

#else // UNDER_CE

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <stddef.h>
#include <limits.h>


#define ASSERT assert

#define RETAILMSG(exp,p)    ((exp)?OutStr p,1:0)

#ifdef DEBUG
#define DEBUGMSG(exp,p) ((exp)?OutStr p,1:0)
#undef NDEBUG
#define DEBUGZONE(x)        1<<x
#else // DEBUG

#define DEBUGMSG(exp,p) (0)
#define NDEBUG

#endif // DEBUG

#endif // UNDER_CE

#define countof(a) (sizeof(a)/sizeof(*(a)))

extern void
OutStr(TCHAR *format, ...);
