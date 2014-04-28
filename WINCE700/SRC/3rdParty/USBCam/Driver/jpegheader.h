//-------------------------------------------------------------------------
// <copyright file="JPEGHeader.h" company="Microsoft">
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


// Inserted before each frame of MJPG data from the camera; converts it to
// a proper JPEG file with which CE's Imaging API can work.
//
// See the spec for JPEG File Interchange Format, version 1.1:
// http://www.jpeg.org/public/jfif.pdf
BYTE JFIFHdr[] = 
{
    0xff,0xd8,                // SOI
    0xff,0xe0,                // APP0
    0x00,0x10,                // APP0 Hdr size
    0x4a,0x46,0x49,0x46,0x00, // ID string: "JFIF"
    0x01,0x01,                // Version
    0x00,                     // Bits per type
    0x00, 0x00,               // X density
    0x00, 0x00,               // Y density
    0x00,                     // X Thumbnail size
    0x00,                     // Y Thumbnail size
};

