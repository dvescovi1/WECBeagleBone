// All rights reserved ADENEO EMBEDDED 2010
#ifndef _TPS65023_H_
#define _TPS65023_H_

typedef enum {
	VLDO1,
	VLDO2,
	VDCDC1,
	VDCDC2,
	VDCDC3,
//----------------
	LOWBATT,	//used for status only
	PWRFAIL,	//used for status only
} TWL_VOLTAGE;

#endif