/*
 * ======================================================================
 *             Texas Instruments OMAP(TM) Platform Software
 * (c) Copyright Texas Instruments, Incorporated. All Rights Reserved.
 *
 * Use of this software is controlled by the terms and conditions found
 * in the license agreement under which this software has been supplied.
 *
 *=======================================================================
 */

// NOTES:
// - solid fill is meant to be handled by using a 1x1 brush
// - a physical pointer for the surface is for later use
// - there is a different virtual stride from physical stride due to the
//   possibility of a difference when activating something like the
//   security wrapper for the TILER
// - normal one-color color key is meant to be handled by making both
//   key values the same
// - color/chroma key format must match the surface being used as the key,
//   with the same endian format as the system (e.g. a BGR24 color key
//   would appear as xRGB in the unsigned long keylow/high members)

#ifndef TIBLT_H
#define TIBLT_H

enum TIBLTERROR_
{
  TIBLT_ERR_NONE = 0,
  TIBLT_ERR_UNKNOWN = -1,
  TIBLT_ERR_NOT_ENOUGH_MEMORY = -2,
  TIBLT_ERR_SYSTEM_RESOURCE = -3,
  TIBLT_ERR_UNSUPPORTED_BLTPARAMS_VERSION = -4,
  TIBLT_ERR_UNSUPPORTED_SURF_VERSION = -5,
  TIBLT_ERR_UNSUPPORTED_DST_FORMAT = -6,
  TIBLT_ERR_UNSUPPORTED_SRC_FORMAT = -7,
  TIBLT_ERR_UNSUPPORTED_MASK_FORMAT = -8,
  TIBLT_ERR_UNSUPPORTED_BRUSH_FORMAT = -9,
  TIBLT_ERR_UNSUPPORTED_SRC2_FORMAT = TIBLT_ERR_UNSUPPORTED_BRUSH_FORMAT,
  TIBLT_ERR_BAD_DST_RECT = -10,
  TIBLT_ERR_BAD_SRC_RECT = -11,
  TIBLT_ERR_BAD_MASK_RECT = -12,
  TIBLT_ERR_BAD_BRUSH_RECT = -13,
  TIBLT_ERR_BAD_SRC2_RECT = TIBLT_ERR_BAD_BRUSH_RECT,
  TIBLT_ERR_BAD_CLIP_RECT = -14,
  TIBLT_ERR_UNSUPPORTED_SCALE_FACTOR = -15,
  TIBLT_ERR_UNSUPPORTED_ROTATION = -16,
  TIBLT_ERR_UNSUPPORTED_CHROMA_KEY = -17,
  TIBLT_ERR_UNSUPPORTED_BLEND = -18,
  TIBLT_ERR_UNSUPPORTED_DITHER = -19,
  TIBLT_ERR_UNSUPPORTED_ROP = -20,
  TIBLT_ERR_UNSUPPORTED_SCALE_TYPE = -21,
  TIBLT_ERR_BAD_FLAGS = -22,
  TIBLT_ERR_UNSUPPORTED_FLAGS = -23,
  TIBLT_ERR_UNSUPPORTED_CONVERSION = -24,
  TIBLT_ERR_UNSUPPORTED_DST_RECT = -25,
  TIBLT_ERR_UNSUPPORTED_SRC_RECT = -26,
  TIBLT_ERR_UNSUPPORTED_MASK_RECT = -27,
  TIBLT_ERR_UNSUPPORTED_BRUSH_RECT = -28,
  TIBLT_ERR_UNSUPPORTED_SRC2_RECT = TIBLT_ERR_UNSUPPORTED_BRUSH_RECT,
  TIBLT_ERR_UNSUPPORTED_CLIP_RECT = -29,
  TIBLT_ERR_UNSUPPORTED_COMBINATION = -30,
  TIBLT_ERR_BAD_DST_POINTER = -31,    // pdst, pdst->virtptr, or pdst->physptr
  TIBLT_ERR_BAD_SRC_POINTER = -32,    // psrc, psrc->virtptr, or psrc->physptr
  TIBLT_ERR_BAD_MASK_POINTER = -33,   // pmask, pmask->virtptr, or pmask->physptr
  TIBLT_ERR_BAD_BRUSH_POINTER = -34,  // pbrush, pbrush->virtptr, or pbrush->physptr
  TIBLT_ERR_BAD_SRC2_POINTER = TIBLT_ERR_BAD_BRUSH_POINTER  // psrc2, psrc2->virtptr, or psrc2->physptr
};
typedef enum TIBLTERROR_ TIBLTERROR;

struct TIBLTRECT_
{
  int left;
  int top;
  unsigned int width;
  unsigned int height;
};
typedef struct TIBLTRECT_ TIBLTRECT;

/*
 * Bit field specifiers for TIBLTFMT
 */
#define TIBLTDEF_FMT_CS_SHIFT 14
#define TIBLTDEF_FMT_MONO (0 << TIBLTDEF_FMT_CS_SHIFT) // 0x0000
#define TIBLTDEF_FMT_LUT  (1 << TIBLTDEF_FMT_CS_SHIFT) // 0x4000
#define TIBLTDEF_FMT_RGB  (2 << TIBLTDEF_FMT_CS_SHIFT) // 0x8000
#define TIBLTDEF_FMT_YUV  (3 << TIBLTDEF_FMT_CS_SHIFT) // 0xC000

#define TIBLTDEF_FMT_REVERSE 0x2000

#define TIBLTDEF_FMT_LEFT_JUSTIFIED 0x1000

// RGB
#define TIBLTDEF_FMT_ALPHA      0x0800
#define TIBLTDEF_FMT_NONPREMULT 0x0400

// YUV
#define TIBLTDEF_FMT_YUV_STD_SHIFT 11
#define TIBLTDEF_FMT_601 (0 << TIBLTDEF_FMT_YUV_STD_SHIFT) // 0x0000
#define TIBLTDEF_FMT_709 (1 << TIBLTDEF_FMT_YUV_STD_SHIFT) // 0x0800

#define TIBLTDEF_YUV_LAYOUT_SHIFT 9
#define TIBLTDEF_FMT_444         (0 << TIBLTDEF_YUV_LAYOUT_SHIFT) // 0x0000
#define TIBLTDEF_FMT_422         (1 << TIBLTDEF_YUV_LAYOUT_SHIFT) // 0x0200
#define TIBLTDEF_FMT_420_2_PLANE (2 << TIBLTDEF_YUV_LAYOUT_SHIFT) // 0x0400
#define TIBLTDEF_FMT_420_3_PLANE (3 << TIBLTDEF_YUV_LAYOUT_SHIFT) // 0x0600

// YUV 4:2:0 3 Plane
#define TIBLTDEF_FMT_UV_SIDE_BY_SIDE 0x0100

enum TIBLTFMT_
{
  TIBLT_FMT_MONO1 = TIBLTDEF_FMT_MONO | 1,
  TIBLT_FMT_MONO2 = TIBLTDEF_FMT_MONO | 2,
  TIBLT_FMT_MONO4 = TIBLTDEF_FMT_MONO | 4,
  TIBLT_FMT_MONO8 = TIBLTDEF_FMT_MONO | 8,
  TIBLT_FMT_LUT1         = TIBLTDEF_FMT_LUT | 1,
  TIBLT_FMT_LUT1_SYMBIAN = TIBLT_FMT_LUT1 | TIBLTDEF_FMT_REVERSE,
  TIBLT_FMT_LUT2         = TIBLTDEF_FMT_LUT | 2,
  TIBLT_FMT_LUT2_SYMBIAN = TIBLT_FMT_LUT2 | TIBLTDEF_FMT_REVERSE,
  TIBLT_FMT_LUT4         = TIBLTDEF_FMT_LUT | 4,
  TIBLT_FMT_LUT4_SYMBIAN = TIBLT_FMT_LUT4 | TIBLTDEF_FMT_REVERSE,
  TIBLT_FMT_LUT8         = TIBLTDEF_FMT_LUT | 8,
  TIBLT_FMT_RGB12  = TIBLTDEF_FMT_RGB | 12,                          // x:4:4:4
  TIBLT_FMT_ARGB12 = TIBLT_FMT_RGB12 | TIBLTDEF_FMT_ALPHA,           // 4:4:4:4
  TIBLT_FMT_RGB15  = TIBLTDEF_FMT_RGB | 15,                          // x:5:5:5
  TIBLT_FMT_ARGB15 = TIBLT_FMT_RGB15 | TIBLTDEF_FMT_ALPHA,           // 1:5:5:5
  TIBLT_FMT_RGB16  = TIBLTDEF_FMT_RGB | 16,                          // 5:6:5
  TIBLT_FMT_ARGB24 = TIBLT_FMT_RGB16 | TIBLTDEF_FMT_ALPHA,           // 8:5:6:5
  TIBLT_FMT_RGBA24 = TIBLT_FMT_ARGB24 | TIBLTDEF_FMT_REVERSE,        // 5:6:5:8
  TIBLT_FMT_RGB24  = TIBLTDEF_FMT_RGB | 24,                          // 8:8:8
  TIBLT_FMT_BGR24  = TIBLT_FMT_RGB24 | TIBLTDEF_FMT_REVERSE,         // 8:8:8
  TIBLT_FMT_xRGB32 = TIBLTDEF_FMT_RGB | 32,                          // x:8:8:8
  TIBLT_FMT_xBGR32 = TIBLT_FMT_xRGB32 | TIBLTDEF_FMT_REVERSE,        // x:8:8:8
  TIBLT_FMT_RGBx32 = TIBLT_FMT_xRGB32 | TIBLTDEF_FMT_LEFT_JUSTIFIED, // 8:8:8:x
  TIBLT_FMT_BGRx32 = TIBLT_FMT_RGBx32 | TIBLTDEF_FMT_REVERSE,        // 8:8:8:x
  TIBLT_FMT_ARGB32 = TIBLT_FMT_xRGB32 | TIBLTDEF_FMT_ALPHA,          // 8:8:8:8
  TIBLT_FMT_ABGR32 = TIBLT_FMT_ARGB32 | TIBLTDEF_FMT_REVERSE,        // 8:8:8:8
  TIBLT_FMT_RGBA32 = TIBLT_FMT_ARGB32 | TIBLTDEF_FMT_LEFT_JUSTIFIED, // 8:8:8:8
  TIBLT_FMT_BGRA32 = TIBLT_FMT_RGBA32 | TIBLTDEF_FMT_REVERSE,        // 8:8:8:8
  TIBLT_FMT_BGRA32_NONPREMULT = TIBLT_FMT_BGRA32 | TIBLTDEF_FMT_NONPREMULT,
  TIBLT_FMT_UYVY = TIBLTDEF_FMT_YUV | TIBLTDEF_FMT_422,
  TIBLT_FMT_Y422 = TIBLT_FMT_UYVY,
  TIBLT_FMT_VYUY = TIBLT_FMT_UYVY | TIBLTDEF_FMT_REVERSE,
  TIBLT_FMT_YUYV = TIBLT_FMT_UYVY | TIBLTDEF_FMT_LEFT_JUSTIFIED,
  TIBLT_FMT_YUY2 = TIBLT_FMT_YUYV,
  TIBLT_FMT_YVYU = TIBLT_FMT_YUYV | TIBLTDEF_FMT_REVERSE,
  TIBLT_FMT_YV12 = TIBLTDEF_FMT_YUV | TIBLTDEF_FMT_420_3_PLANE,
  TIBLT_FMT_IYUV = TIBLT_FMT_YV12 | TIBLTDEF_FMT_REVERSE,
  TIBLT_FMT_I420 = TIBLT_FMT_IYUV,
  TIBLT_FMT_IMC1 = TIBLT_FMT_YV12 | TIBLTDEF_FMT_LEFT_JUSTIFIED,
  TIBLT_FMT_IMC2 = TIBLT_FMT_YV12 | TIBLTDEF_FMT_UV_SIDE_BY_SIDE,
  TIBLT_FMT_IMC3 = TIBLT_FMT_IMC1 | TIBLTDEF_FMT_REVERSE,
  TIBLT_FMT_IMC4 = TIBLT_FMT_IMC2 | TIBLTDEF_FMT_REVERSE,
  TIBLT_FMT_NV12 = TIBLTDEF_FMT_YUV | TIBLTDEF_FMT_420_2_PLANE,
  TIBLT_FMT_NV21 = TIBLT_FMT_NV12 | TIBLTDEF_FMT_REVERSE
};
typedef enum TIBLTFMT_ TIBLTFMT;

struct TIBLTSURF_
{
  unsigned int       structsize;
  TIBLTFMT           format;
  unsigned int       width;
  unsigned int       height;
  int                orientation;
  void*              virtptr;
  long               virtstride;
  unsigned long long physptr;
  long               physstride;
  unsigned long*     palette;     // Should match format, but be on 32-bit boundary
};
typedef struct TIBLTSURF_ TIBLTSURF;

typedef unsigned long TIBLTFLAGS;
#define TIBLT_FLAG_ROP             0x00000001 // mutually exclusive with BLEND
#define TIBLT_FLAG_BLEND           0x00000002 // mutually exclusive with ROP
#define TIBLT_FLAG_KEY_SRC         0x00000004
#define TIBLT_FLAG_KEY_DST         0x00000008
#define TIBLT_FLAG_DITHER          0x00000010
#define TIBLT_FLAG_FLIP_HORIZONTAL 0x00000020
#define TIBLT_FLAG_FLIP_VERTICAL   0x00000040
#define TIBLT_FLAG_CLIP            0x00000080

typedef unsigned long TIBLTBLENDTYPE;
#define TIBLT_BLEND_GLOBAL   0x00008000  // globalalpha used for blending
#define TIBLT_BLEND_REMOTE   0x00004000  // mask used for blending
#define TIBLT_BLEND_SRCLOCAL 0x00002000  // source alpha used for blending
#define TIBLT_BLEND_DSTLOCAL 0x00001000  // dest alpha used for blending
#define TIBLT_BLEND_TWOSRC   0x00000800  // two sources vs. blend into dest
#define TIBLT_BLEND_SRCOVER  0x00000001

enum TIBLTDITHERTYPE_
{
  DT_NONE = 0,
  DT_ORDERED_2x2 = 1
};
typedef enum TIBLTDITHERTYPE_ TIBLTDITHERTYPE;

enum TIBLTSCALETYPE_
{
  SC_POINT_SAMPLE = 0,
  SC_BILINEAR = 1,
  SC_MODIFIED_BILINEAR = 2
};
typedef enum TIBLTSCALETYPE_ TIBLTSCALETYPE;

struct TIBLTPARAMS_
{
  unsigned int    structsize;
  TIBLTFLAGS      flags;
  unsigned int    ROP;
  TIBLTSURF*      pdst;
  TIBLTRECT       dstrect;
  TIBLTSURF*      psrc;
  TIBLTRECT       srcrect;
  TIBLTDITHERTYPE dithertype;
  unsigned long   keylow;
  unsigned long   keyhigh;
  TIBLTBLENDTYPE  blendtype;
  unsigned int    globalalpha; // only 0 = 0.0 through 255 = 1.0
  TIBLTSCALETYPE  scaletype;
  TIBLTRECT       cliprect;
  TIBLTSURF*      pmask;
  TIBLTRECT       maskrect;
  union
  {
    TIBLTSURF*      pbrush;
    TIBLTSURF*      psrc2;
  };
  union
  {
    TIBLTRECT       brushrect;
    TIBLTRECT       src2rect;
  };
};
typedef struct TIBLTPARAMS_ TIBLTPARAMS;

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

TIBLTERROR TIBLT(TIBLTPARAMS*);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // TIBLT_H

