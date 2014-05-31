/*
================================================================================
*             Texas Instruments OMAP(TM) Platform Software
* (c) Copyright Texas Instruments, Incorporated. All Rights Reserved.
*
* Use of this software is controlled by the terms and conditions found
* in the license agreement under which this software has been supplied.
*
================================================================================
*/

#ifndef __ISP_RESIZER_H__
#define __ISP_RESIZER_H__

typedef struct {
    ULONG ulReadAddr;
    ULONG ulReadOffset;
    ULONG ulReadAddrOffset;
    ULONG ulWriteAddr;
    ULONG ulInputImageWidth;
    ULONG ulInputImageHeight;
    ULONG ulOutputImageWidth;
    ULONG ulOutputImageHeight;
    ULONG h_startphase;
    ULONG v_startphase;
    ULONG h_resz;
    ULONG v_resz;
    ULONG algo;
    ULONG width;
    ULONG height;
    ULONG cropTop;
    ULONG cropLeft;
    ULONG cropHeight;
    ULONG cropWidth;
    BOOL  bReadFromMemory;
    BOOL  enableZoom;
    ULONG ulZoomFactor;    
    ULONG ulOutOffset;
    ULONG ulPixFmt; 
    ULONG ulInpType;
}RSZParams_t;

#define RSZ_IOCTL_GET_PARAMS    1
#define RSZ_IOCTL_SET_PARAMS    2
#define RSZ_IOCTL_GET_STATUS    3
#define RSZ_IOCTL_START_RESIZER 4
 

#define RSZ_INTYPE_YCBCR422_16BIT   0
#define RSZ_INTYPE_PLANAR_8BIT      1
#define RSZ_PIX_FMT_UYVY        0   /* cb:y:cr:y */
#define RSZ_PIX_FMT_YUYV        1   /* y:cb:y:cr */

#define RSZ_DEFAULTSTPHASE                 1



#endif
