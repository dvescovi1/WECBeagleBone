// All rights reserved ADENEO EMBEDDED 2010
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

#ifndef __SENSORFORMATS_H
#define __SENSORFORMATS_H

#ifndef mmioFOURCC    
#define mmioFOURCC( ch0, ch1, ch2, ch3 )          \
     ( (DWORD)(BYTE)(ch0) | ( (DWORD)(BYTE)(ch1) << 8 ) |  \
     ( (DWORD)(BYTE)(ch2) << 16 ) | ( (DWORD)(BYTE)(ch3) << 24 ) )
#endif  

#define BITRATE(DX,DY,DBITCOUNT,FRAMERATE)    (DX * abs(DY) * DBITCOUNT * FRAMERATE)
#define SAMPLESIZE(DX,DY,DBITCOUNT) (DX * abs(DY) * DBITCOUNT / 8)

#define REFTIME_30FPS  333333
#define REFTIME_25FPS  400000
#define REFTIME_24FPS  416667
#define REFTIME_15FPS  666667
#define REFTIME_10FPS 1000000
#define REFTIME_3FPS  3333333

//
// FourCC of the YUV formats
// For information about FourCC, go to:
//     http://www.webartz.com/fourcc/indexyuv.htm
//     http://www.fourcc.org
//

#define FOURCC_UYVY     mmioFOURCC('U', 'Y', 'V', 'Y')  // MSYUV: 1394 conferencing camera 4:4:4 mode 1 and 3
#define FOURCC_YUY2     mmioFOURCC('Y', 'U', 'Y', '2')
#define FOURCC_YV12     mmioFOURCC('Y', 'V', '1', '2')

#define MEDIASUBTYPE_RGB565 {0xe436eb7b, 0x524f, 0x11ce, 0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70}
#define MEDIASUBTYPE_RGB555 {0xe436eb7c, 0x524f, 0x11ce, 0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70}
#define MEDIASUBTYPE_RGB24  {0xe436eb7d, 0x524f, 0x11ce, 0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70}
#define MEDIASUBTYPE_RGB32  {0xe436eb7e, 0x524f, 0x11ce, 0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70}

#define MEDIASUBTYPE_YV12   {0x32315659, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}
#define MEDIASUBTYPE_YUY2   {0x32595559, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}
#define MEDIASUBTYPE_UYVY   {0x59565955, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}

#define MAKE_STREAM_MODE_UYVY(StreamModeName, DX, DY, DBITCOUNT, FRAMERATE) \
    CS_DATARANGE_VIDEO StreamModeName =  \
    { \
        {    \
            sizeof (CS_DATARANGE_VIDEO),     \
            0, \
            SAMPLESIZE(DX,DY,DBITCOUNT),     \
            0,                               \
            STATIC_CSDATAFORMAT_TYPE_VIDEO,   \
            MEDIASUBTYPE_UYVY, \
            STATIC_CSDATAFORMAT_SPECIFIER_VIDEOINFO \
        }, \
 \
        TRUE,                   \
        TRUE,                   \
        CS_VIDEOSTREAM_CAPTURE, \
        0,                      \
 \
        { \
            STATIC_CSDATAFORMAT_SPECIFIER_VIDEOINFO,  \
            CS_AnalogVideo_None, \
            DX,DY,    \
            DX,DY,    \
            DX,DY,    \
            1,        \
            1,        \
            1,        \
            1,        \
            DX, DY,   \
            DX, DY,   \
            DX,       \
            DY,       \
            0,        \
            0,        \
            0,        \
            0,        \
            REFTIME_##FRAMERATE##FPS,                      \
            REFTIME_##FRAMERATE##FPS,                      \
            BITRATE(DX,DY,DBITCOUNT,FRAMERATE) / 8,        \
            BITRATE(DX,DY,DBITCOUNT,FRAMERATE),            \
        },  \
 \
        { \
            0,0,0,0,                            \
            0,0,0,0,                            \
            BITRATE(DX,DY,DBITCOUNT,FRAMERATE), \
            0L,                                 \
            REFTIME_##FRAMERATE##FPS,                      \
            sizeof (CS_BITMAPINFOHEADER),       \
            DX,                                 \
            DY,                                 \
            1,                        \
            DBITCOUNT,                \
            FOURCC_UYVY | BI_SRCPREROTATE,      \
            SAMPLESIZE(DX,DY,DBITCOUNT), \
            0,                        \
            0,                        \
            0,                        \
            0,                        \
            0, 0, 0                   \
        } \
    };  

#define MAKE_STREAM_MODE_YV12(StreamModeName, DX, DY, DBITCOUNT, FRAMERATE) \
    CS_DATARANGE_VIDEO StreamModeName =  \
    { \
        {    \
            sizeof (CS_DATARANGE_VIDEO),     \
            0, \
            SAMPLESIZE(DX,DY,DBITCOUNT),     \
            0,                               \
            STATIC_CSDATAFORMAT_TYPE_VIDEO,   \
            MEDIASUBTYPE_YV12, \
            STATIC_CSDATAFORMAT_SPECIFIER_VIDEOINFO \
        }, \
 \
        TRUE,                   \
        TRUE,                   \
        CS_VIDEOSTREAM_CAPTURE, \
        0,                      \
 \
        { \
            STATIC_CSDATAFORMAT_SPECIFIER_VIDEOINFO,  \
            CS_AnalogVideo_None, \
            DX,DY,    \
            DX,DY,    \
            DX,DY,    \
            1,        \
            1,        \
            1,        \
            1,        \
            DX, DY,   \
            DX, DY,   \
            DX,       \
            DY,       \
            0,        \
            0,        \
            0,        \
            0,        \
            REFTIME_##FRAMERATE##FPS,                      \
            REFTIME_##FRAMERATE##FPS,                      \
            BITRATE(DX,DY,DBITCOUNT,FRAMERATE) / 8,        \
            BITRATE(DX,DY,DBITCOUNT,FRAMERATE),            \
        },  \
 \
        { \
            0,0,0,0,                            \
            0,0,0,0,                            \
            BITRATE(DX,DY,DBITCOUNT,FRAMERATE), \
            0L,                                 \
            REFTIME_##FRAMERATE##FPS,                      \
            sizeof (CS_BITMAPINFOHEADER),       \
            DX,                                 \
            DY,                                 \
            3,                        \
            DBITCOUNT,                \
            FOURCC_YV12 | BI_SRCPREROTATE,      \
            SAMPLESIZE(DX,DY,DBITCOUNT), \
            0,                        \
            0,                        \
            0,                        \
            0,                        \
            0, 0, 0                   \
        } \
    }; 

#define MAKE_STREAM_MODE_YUY2(StreamModeName, DX, DY, DBITCOUNT, FRAMERATE) \
    CS_DATARANGE_VIDEO StreamModeName =  \
    { \
        {    \
            sizeof (CS_DATARANGE_VIDEO),     \
            0, \
            SAMPLESIZE(DX,DY,DBITCOUNT),     \
            0,                               \
            STATIC_CSDATAFORMAT_TYPE_VIDEO,   \
            MEDIASUBTYPE_YUY2, \
            STATIC_CSDATAFORMAT_SPECIFIER_VIDEOINFO \
        }, \
 \
        TRUE,                   \
        TRUE,                   \
        CS_VIDEOSTREAM_CAPTURE, \
        0,                      \
 \
        { \
            STATIC_CSDATAFORMAT_SPECIFIER_VIDEOINFO,  \
            CS_AnalogVideo_None, \
            DX,DY,    \
            DX,DY,    \
            DX,DY,    \
            1,        \
            1,        \
            1,        \
            1,        \
            DX, DY,   \
            DX, DY,   \
            DX,       \
            DY,       \
            0,        \
            0,        \
            0,        \
            0,        \
            REFTIME_##FRAMERATE##FPS,                      \
            REFTIME_##FRAMERATE##FPS,                      \
            BITRATE(DX,DY,DBITCOUNT,FRAMERATE) / 8,        \
            BITRATE(DX,DY,DBITCOUNT,FRAMERATE),            \
        },  \
 \
        { \
            0,0,0,0,                            \
            0,0,0,0,                            \
            BITRATE(DX,DY,DBITCOUNT,FRAMERATE), \
            0L,                                 \
            REFTIME_##FRAMERATE##FPS,                      \
            sizeof (CS_BITMAPINFOHEADER),       \
            DX,                                 \
            DY,                                 \
            1,                        \
            DBITCOUNT,                \
            FOURCC_YUY2 | BI_SRCPREROTATE,      \
            SAMPLESIZE(DX,DY,DBITCOUNT), \
            0,                        \
            0,                        \
            0,                        \
            0,                        \
            0, 0, 0                   \
        } \
    }; 

#define MAKE_STREAM_MODE_RGB565(StreamModeName, DX, DY, DBITCOUNT, FRAMERATE) \
    CS_DATARANGE_VIDEO StreamModeName =  \
    { \
        {    \
            sizeof (CS_DATARANGE_VIDEO),     \
            0, \
            SAMPLESIZE(DX,DY,DBITCOUNT),     \
            0,                               \
            STATIC_CSDATAFORMAT_TYPE_VIDEO,   \
            MEDIASUBTYPE_RGB565, \
            STATIC_CSDATAFORMAT_SPECIFIER_VIDEOINFO \
        }, \
 \
        TRUE,                   \
        TRUE,                   \
        CS_VIDEOSTREAM_CAPTURE, \
        0,                      \
 \
        { \
            STATIC_CSDATAFORMAT_SPECIFIER_VIDEOINFO,  \
            CS_AnalogVideo_None, \
            DX,DY,    \
            DX,DY,    \
            DX,DY,    \
            1,        \
            1,        \
            1,        \
            1,        \
            DX, DY,   \
            DX, DY,   \
            DX,       \
            DY,       \
            0,        \
            0,        \
            0,        \
            0,        \
            REFTIME_##FRAMERATE##FPS,                      \
            REFTIME_##FRAMERATE##FPS,                      \
            BITRATE(DX,DY,DBITCOUNT,FRAMERATE) / 4,        \
            BITRATE(DX,DY,DBITCOUNT,FRAMERATE),            \
        },  \
 \
        { \
            0,0,0,0,                            \
            0,0,0,0,                            \
            BITRATE(DX,DY,DBITCOUNT,FRAMERATE), \
            0L,                                 \
            REFTIME_##FRAMERATE##FPS,           \
            sizeof (CS_BITMAPINFOHEADER),       \
            DX,                                 \
            DY,                                 \
            1,                                  \
            DBITCOUNT,                          \
            CS_BI_BITFIELDS | BI_SRCPREROTATE,  \
            SAMPLESIZE(DX,DY,DBITCOUNT),        \
            0,                                  \
            0,                                  \
            0,                                  \
            0,                                  \
            0xf800, 0x07e0, 0x001f              \
        } \
    }; 

#include "params.h"
MAKE_STREAM_MODE_UYVY(DCAM_StreamMode_TV_D1_NTSC_UYVY, TV_D1_NTSC_HORZ_RES, -TV_D1_NTSC_VERT_RES, 16, 30);
MAKE_STREAM_MODE_YUY2(DCAM_StreamMode_TV_D1_NTSC_YUY2, TV_D1_NTSC_HORZ_RES, -TV_D1_NTSC_VERT_RES, 16, 30);
MAKE_STREAM_MODE_UYVY(DCAM_StreamMode_DVD_NTSC_UYVY, DVD_NTSC_HORZ_RES, -DVD_NTSC_VERT_RES, 16, 30);
MAKE_STREAM_MODE_YUY2(DCAM_StreamMode_DVD_NTSC_YUY2, DVD_NTSC_HORZ_RES, -DVD_NTSC_VERT_RES, 16, 30);
MAKE_STREAM_MODE_UYVY(DCAM_StreamMode_CIF_NTSC_UYVY, CIF_NTSC_HORZ_RES, -CIF_NTSC_VERT_RES, 16, 30);
MAKE_STREAM_MODE_YUY2(DCAM_StreamMode_CIF_NTSC_YUY2, CIF_NTSC_HORZ_RES, -CIF_NTSC_VERT_RES, 16, 30);
MAKE_STREAM_MODE_UYVY(DCAM_StreamMode_QCIF_NTSC_UYVY, QCIF_NTSC_HORZ_RES, -QCIF_NTSC_VERT_RES, 16, 30);
MAKE_STREAM_MODE_YUY2(DCAM_StreamMode_QCIF_NTSC_YUY2, QCIF_NTSC_HORZ_RES, -QCIF_NTSC_VERT_RES, 16, 30);
MAKE_STREAM_MODE_UYVY(DCAM_StreamMode_TV_D1_PAL_UYVY, TV_D1_PAL_HORZ_RES, -TV_D1_PAL_VERT_RES, 16, 25);
MAKE_STREAM_MODE_YUY2(DCAM_StreamMode_TV_D1_PAL_YUY2, TV_D1_PAL_HORZ_RES, -TV_D1_PAL_VERT_RES, 16, 25);
MAKE_STREAM_MODE_UYVY(DCAM_StreamMode_DVD_PAL_UYVY, DVD_PAL_HORZ_RES, -DVD_PAL_VERT_RES, 16, 25);
MAKE_STREAM_MODE_YUY2(DCAM_StreamMode_DVD_PAL_YUY2, DVD_PAL_HORZ_RES, -DVD_PAL_VERT_RES, 16, 25);
MAKE_STREAM_MODE_UYVY(DCAM_StreamMode_CIF_PAL_UYVY, CIF_PAL_HORZ_RES, -CIF_PAL_VERT_RES, 16, 25);
MAKE_STREAM_MODE_YUY2(DCAM_StreamMode_CIF_PAL_YUY2, CIF_PAL_HORZ_RES, -CIF_PAL_VERT_RES, 16, 25);
MAKE_STREAM_MODE_UYVY(DCAM_StreamMode_QCIF_PAL_UYVY, QCIF_PAL_HORZ_RES, -QCIF_PAL_VERT_RES, 16, 25);
MAKE_STREAM_MODE_YUY2(DCAM_StreamMode_QCIF_PAL_YUY2, QCIF_PAL_HORZ_RES, -QCIF_PAL_VERT_RES, 16, 25);
MAKE_STREAM_MODE_UYVY(DCAM_StreamMode_VGA_UYVY_30, VGA_HORZ_RES, -VGA_VERT_RES, 16, 30);
MAKE_STREAM_MODE_UYVY(DCAM_StreamMode_QVGA_UYVY_30, QVGA_HORZ_RES, -QVGA_VERT_RES, 16, 30);
MAKE_STREAM_MODE_YUY2(DCAM_StreamMode_VGA_YUY2_30, VGA_HORZ_RES, -VGA_VERT_RES, 16, 30);
MAKE_STREAM_MODE_YUY2(DCAM_StreamMode_QVGA_YUY2_30, QVGA_HORZ_RES, -QVGA_VERT_RES, 16, 30);

#endif //__SENSORFORMATS_H
