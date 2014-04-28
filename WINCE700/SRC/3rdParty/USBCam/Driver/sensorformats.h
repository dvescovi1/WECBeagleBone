//-------------------------------------------------------------------------
// <copyright file="SensorFormats.h" company="Microsoft">
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//    The use and distribution terms for this software are covered by the
//    Microsoft Limited Permissive License (Ms-LPL) 
//    which can be found in the file MS-LPL.txt at the root of this distribution.
//    By using this software in any fashion, you are agreeing to be bound by
//    the terms of this license.
//
//    THE SOFTWARE IS LICENSED "AS-IS" WITH NO WARRANTIES OR INDEMNITIES. 
//
//    You must not remove this notice, or any other, from this software.
// </copyright>
// 
// <summary>
//    USB camera driver for Windows Embedded CE 6.0
// </summary>
//-------------------------------------------------------------------------
//======================================================================
// USB camera driver for Windows Embedded CE 6.0
//======================================================================

#ifndef __SENSORFORMATS_H
#define __SENSORFORMATS_H

#ifndef mmioFOURCC    
#define mmioFOURCC( ch0, ch1, ch2, ch3 )                        \
     ((DWORD)(BYTE)(ch0) | ( (DWORD)(BYTE)(ch1) << 8 ) |        \
     ((DWORD)(BYTE)(ch2) << 16 ) | ( (DWORD)(BYTE)(ch3) << 24 ) )
#endif  

#define BITRATE(SAMPLESIZE, FRAMERATE) (SAMPLESIZE * FRAMERATE)
#define BITCOUNT(SAMPLESIZE, HT, WD) ((SAMPLESIZE / (HT * WD)) * 8) 


//
// FourCC of the YUV formats
// For information about FourCC, go to:
//     http://www.fourcc.org
//

#define FOURCC_UYVY     mmioFOURCC('U', 'Y', 'V', 'Y')  // MSYUV: 1394 conferencing camera 4:4:4 mode 1 and 3
#define FOURCC_YUY2     mmioFOURCC('Y', 'U', 'Y', '2')
#define FOURCC_YV12     mmioFOURCC('Y', 'V', '1', '2')
#define FOURCC_JPEG mmioFOURCC('J','P','E','G')
#define FOURCC_MJPG mmioFOURCC('M','J','P','G')

#define MEDIASUBTYPE_RGB565 {0xe436eb7b, 0x524f, 0x11ce, 0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70}
#define MEDIASUBTYPE_RGB555 {0xe436eb7c, 0x524f, 0x11ce, 0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70}
#define MEDIASUBTYPE_RGB24  {0xe436eb7d, 0x524f, 0x11ce, 0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70}
#define MEDIASUBTYPE_RGB32  {0xe436eb7e, 0x524f, 0x11ce, 0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70}

#define MEDIASUBTYPE_YV12   {0x32315659, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}
#define MEDIASUBTYPE_YUY2   {0x32595559, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}
#define MEDIASUBTYPE_MJPG   {0x47504A4D, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}
#define MEDIASUBTYPE_IJPG   {0x47504A49, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}


#define MAKE_STREAM_MODE_YV12(StreamModeName, DX, DY, SAMPLESIZE, FRAMERATE, DBITCOUNT, MODE) \
    CS_DATARANGE_VIDEO StreamModeName =                                                       \
    {                                                                                         \
        {                                                                                     \
            sizeof (CS_DATARANGE_VIDEO),                                                      \
            0,                                                                                \
            SAMPLESIZE,                                                                       \
            0,                                                                                \
            STATIC_CSDATAFORMAT_TYPE_VIDEO,                                                   \
            MEDIASUBTYPE_YV12,                                                                \
            STATIC_CSDATAFORMAT_SPECIFIER_VIDEOINFO                                           \
        },                                                                                    \
                                                                                              \
        TRUE,                                                                                 \
        TRUE,                                                                                 \
        MODE,                                                                                 \
        0,                                                                                    \
                                                                                              \
        {                                                                                     \
            STATIC_CSDATAFORMAT_SPECIFIER_VIDEOINFO,                                          \
            CS_AnalogVideo_None,                                                              \
            DX,DY,                                                                            \
            DX,DY,                                                                            \
            DX,DY,                                                                            \
            1,                                                                                \
            1,                                                                                \
            1,                                                                                \
            1,                                                                                \
            DX, DY,                                                                           \
            DX, DY,                                                                           \
            DX,                                                                               \
            DY,                                                                               \
            0,                                                                                \
            0,                                                                                \
            0,                                                                                \
            0,                                                                                \
            FRAMERATE,                                                                        \
            FRAMERATE,                                                                        \
            BITRATE(SAMPLESIZE, FRAMERATE) / 8,                                               \
            BITRATE(SAMPLESIZE, FRAMERATE),                                                   \
        },                                                                                    \
                                                                                              \
        {                                                                                     \
            0,0,0,0,                                                                          \
            0,0,0,0,                                                                          \
            BITRATE(SAMPLESIZE, FRAMERATE),                                                   \
            0L,                                                                               \
            FRAMERATE,                                                                        \
            sizeof (CS_BITMAPINFOHEADER),                                                     \
            DX,                                                                               \
            DY,                                                                               \
            1,                                                                                \
            DBITCOUNT,                                                                        \
            FOURCC_YV12 | BI_SRCPREROTATE,                                                    \
            SAMPLESIZE,                                                                       \
            0,                                                                                \
            0,                                                                                \
            0,                                                                                \
            0,                                                                                \
            0, 0, 0                                                                           \
        }                                                                                     \
    }; 

#define MAKE_STREAM_MODE_YUY2(StreamModeName, DX, DY, SAMPLESIZE, FRAMERATE, DBITCOUNT, MODE) \
    CS_DATARANGE_VIDEO StreamModeName =                                                       \
    {                                                                                         \
        {                                                                                     \
            sizeof (CS_DATARANGE_VIDEO),                                                      \
            0,                                                                                \
            SAMPLESIZE,                                                                       \
            0,                                                                                \
            STATIC_CSDATAFORMAT_TYPE_VIDEO,                                                   \
            MEDIASUBTYPE_YUY2,                                                                \
            STATIC_CSDATAFORMAT_SPECIFIER_VIDEOINFO                                           \
        },                                                                                    \
                                                                                              \
        TRUE,                                                                                 \
        TRUE,                                                                                 \
        MODE,                                                                                 \
        0,                                                                                    \
                                                                                              \
        {                                                                                     \
            STATIC_CSDATAFORMAT_SPECIFIER_VIDEOINFO,                                          \
            CS_AnalogVideo_None,                                                              \
            DX,DY,                                                                            \
            DX,DY,                                                                            \
            DX,DY,                                                                            \
            1,                                                                                \
            1,                                                                                \
            1,                                                                                \
            1,                                                                                \
            DX, DY,                                                                           \
            DX, DY,                                                                           \
            DX,                                                                               \
            DY,                                                                               \
            0,                                                                                \
            0,                                                                                \
            0,                                                                                \
            0,                                                                                \
            FRAMERATE,                                                                        \
            FRAMERATE,                                                                        \
            BITRATE(SAMPLESIZE, FRAMERATE) / 8,                                               \
            BITRATE(SAMPLESIZE,FRAMERATE),                                                    \
        },                                                                                    \
                                                                                              \
        {                                                                                     \
            0,0,0,0,                                                                          \
            0,0,0,0,                                                                          \
            BITRATE(SAMPLESIZE,FRAMERATE),                                                    \
            0L,                                                                               \
            FRAMERATE,                                                                        \
            sizeof (CS_BITMAPINFOHEADER),                                                     \
            DX,                                                                               \
            DY,                                                                               \
            1,                                                                                \
            DBITCOUNT,                                                                        \
            FOURCC_YUY2 | BI_SRCPREROTATE,                                                    \
            SAMPLESIZE,                                                                       \
            0,                                                                                \
            0,                                                                                \
            0,                                                                                \
            0,                                                                                \
            0, 0, 0                                                                           \
        }                                                                                     \
    };

#define MAKE_STREAM_MODE_RGB565(StreamModeName, DX, DY, SAMPLESIZE, FRAMERATE, DBITCOUNT, MODE) \
    CS_DATARANGE_VIDEO StreamModeName =                                                         \
    {                                                                                           \
        {                                                                                       \
            sizeof (CS_DATARANGE_VIDEO),                                                        \
            UID,                                                                                \
            SAMPLESIZE,                                                                         \
            0,                                                                                  \
            STATIC_CSDATAFORMAT_TYPE_VIDEO,                                                     \
            MEDIASUBTYPE_RGB565,                                                                \
            STATIC_CSDATAFORMAT_SPECIFIER_VIDEOINFO                                             \
        },                                                                                      \
                                                                                                \
        TRUE,                                                                                   \
        TRUE,                                                                                   \
        MODE,                                                                                   \
        0,                                                                                      \
                                                                                                \
        {                                                                                       \
            STATIC_CSDATAFORMAT_SPECIFIER_VIDEOINFO,                                            \
            CS_AnalogVideo_None,                                                                \
            DX,DY,                                                                              \
            DX,DY,                                                                              \
            DX,DY,                                                                              \
            1,                                                                                  \
            1,                                                                                  \
            1,                                                                                  \
            1,                                                                                  \
            DX, DY,                                                                             \
            DX, DY,                                                                             \
            DX,                                                                                 \
            DY,                                                                                 \
            0,                                                                                  \
            0,                                                                                  \
            0,                                                                                  \
            0,                                                                                  \
            FRAMERATE,                                                                          \
            FRAMERATE,                                                                          \
            BITRATE(SAMPLESIZE,FRAMERATE) / 8,                                                  \
            BITRATE(SAMPLESIZE,FRAMERATE),                                                      \
        },                                                                                      \
                                                                                                \
        {                                                                                       \
            0,0,0,0,                                                                            \
            0,0,0,0,                                                                            \
            BITRATE(SAMPLESIZE,FRAMERATE),                                                      \
            0L,                                                                                 \
            FRAMERATE,                                                                          \
            sizeof (CS_BITMAPINFOHEADER),                                                       \
            DX,                                                                                 \
            DY,                                                                                 \
            1,                                                                                  \
            DBITCOUNT,                                                                          \
            CS_BI_BITFIELDS | BI_SRCPREROTATE,                                                  \
            SAMPLESIZE,                                                                         \
            0,                                                                                  \
            0,                                                                                  \
            0,                                                                                  \
            0,                                                                                  \
            0xf800, 0x07e0, 0x001f                                                              \
        }                                                                                       \
    }; 

#define MAKE_STREAM_MODE_JPEG(StreamModeName, DX, DY, SAMPLESIZE, FRAMERATE, DBITCOUNT, MODE) \
    CS_DATARANGE_VIDEO StreamModeName =                                                       \
    {                                                                                         \
        {                                                                                     \
            sizeof (CS_DATARANGE_VIDEO),                                                      \
            0,                                                                                \
            SAMPLESIZE,                                                                       \
            0,                                                                                \
            STATIC_CSDATAFORMAT_TYPE_VIDEO,                                                   \
            MEDIASUBTYPE_IJPG,                                                                \
            STATIC_CSDATAFORMAT_SPECIFIER_VIDEOINFO                                           \
        },                                                                                    \
                                                                                              \
        TRUE,                                                                                 \
        TRUE,                                                                                 \
        MODE,                                                                                 \
        0,                                                                                    \
                                                                                              \
        {                                                                                     \
            STATIC_CSDATAFORMAT_SPECIFIER_VIDEOINFO,                                          \
            CS_AnalogVideo_None,                                                              \
            DX,DY,                                                                            \
            DX,DY,                                                                            \
            DX,DY,                                                                            \
            1,                                                                                \
            1,                                                                                \
            1,                                                                                \
            1,                                                                                \
            DX, DY,                                                                           \
            DX, DY,                                                                           \
            DX,                                                                               \
            DY,                                                                               \
            0,                                                                                \
            0,                                                                                \
            0,                                                                                \
            0,                                                                                \
            FRAMERATE,                                                                        \
            FRAMERATE,                                                                        \
            BITRATE(SAMPLESIZE,FRAMERATE) / 8,                                                \
            BITRATE(SAMPLESIZE,FRAMERATE),                                                    \
        },                                                                                    \
                                                                                              \
        {                                                                                     \
            0,0,0,0,                                                                          \
            0,0,0,0,                                                                          \
            BITRATE(SAMPLESIZE,FRAMERATE),                                                    \
            0L,                                                                               \
            FRAMERATE,                                                                        \
            sizeof (CS_BITMAPINFOHEADER),                                                     \
            DX,                                                                               \
            DY,                                                                               \
            1,                                                                                \
            DBITCOUNT,                                                                        \
            FOURCC_JPEG | BI_SRCPREROTATE,                                                    \
            SAMPLESIZE,                                                                       \
            0,                                                                                \
            0,                                                                                \
            0,                                                                                \
            0,                                                                                \
            0, 0, 0                                                                           \
        }                                                                                     \
    }; 

#define MAKE_STREAM_MODE_MJPG(StreamModeName, DX, DY, SAMPLESIZE, FRAMERATE, DBITCOUNT, MODE) \
    CS_DATARANGE_VIDEO StreamModeName =                                                       \
    {                                                                                         \
        {                                                                                     \
            sizeof (CS_DATARANGE_VIDEO),                                                      \
            0,                                                                                \
            SAMPLESIZE,                                                                       \
            0,                                                                                \
            STATIC_CSDATAFORMAT_TYPE_VIDEO,                                                   \
            MEDIASUBTYPE_MJPG,                                                                \
            STATIC_CSDATAFORMAT_SPECIFIER_VIDEOINFO                                           \
        },                                                                                    \
                                                                                              \
        TRUE,                                                                                 \
        TRUE,                                                                                 \
        MODE,                                                                                 \
        0,                                                                                    \
                                                                                              \
        {                                                                                     \
            STATIC_CSDATAFORMAT_SPECIFIER_VIDEOINFO,                                          \
            CS_AnalogVideo_None,                                                              \
            DX,DY,                                                                            \
            DX,DY,                                                                            \
            DX,DY,                                                                            \
            1,                                                                                \
            1,                                                                                \
            1,                                                                                \
            1,                                                                                \
            DX, DY,                                                                           \
            DX, DY,                                                                           \
            DX,                                                                               \
            DY,                                                                               \
            0,                                                                                \
            0,                                                                                \
            0,                                                                                \
            0,                                                                                \
            FRAMERATE,                                                                        \
            FRAMERATE,                                                                        \
            BITRATE(SAMPLESIZE,FRAMERATE) / 8,                                                \
            BITRATE(SAMPLESIZE,FRAMERATE),                                                    \
        },                                                                                    \
                                                                                              \
        {                                                                                     \
            0,0,0,0,                                                                          \
            0,0,0,0,                                                                          \
            BITRATE(SAMPLESIZE,FRAMERATE),                                                    \
            0L,                                                                               \
            FRAMERATE,                                                                        \
            sizeof (CS_BITMAPINFOHEADER),                                                     \
            DX,                                                                               \
            DY,                                                                               \
            1,                                                                                \
            DBITCOUNT,                                                                        \
            FOURCC_MJPG | BI_SRCPREROTATE,                                                    \
            SAMPLESIZE,                                                                       \
            0,                                                                                \
            0,                                                                                \
            0,                                                                                \
            0,                                                                                \
            0, 0, 0                                                                           \
        }                                                                                     \
    };

#endif //__SENSORFORMATS_H
