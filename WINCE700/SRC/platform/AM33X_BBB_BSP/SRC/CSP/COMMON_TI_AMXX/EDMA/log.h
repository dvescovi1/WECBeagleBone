//
// Copyright (c) MPC-Data Limited 2009.  All rights reserved.
//
//------------------------------------------------------------------------------
//
//  File:  log.h
//
#ifndef __LOG_H
#define __LOG_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct LOG_Obj {
    int dummy;
} LOG_Obj;

typedef struct LOG_Obj *LOG_Handle; 

extern void LOG_printf4(LOG_Handle log, const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif // __LOG_H
