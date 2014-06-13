// All rights reserved ADENEO EMBEDDED 2010
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

#include "precomp.h"
#include "dispperf.h"

//------------------------------------------------------------------------------
//  Defines

// If CALLOUT is not (void), it prints out the name of the Emulated*() function being executed

//#define CALLOUT OutputDebugString
#define CALLOUT (void)

// CHECKBLT performs each operation with the optimized function and
// then compares it to EmulatedBlt() using the CRC checks below.

//#define CHECKBLT
#define NOCHECKALPHA
#define NOCHECK565

//------------------------------------------------------------------------------
//  Structures

typedef SCODE (GPE::*BLTFN)(GPEBltParms *);


//------------------------------------------------------------------------------
//  Globals


//------------------------------------------------------------------------------
//  Assembly Prototypes

extern "C"
{
  // Preload the cache with the contents of the specified line
  void PreloadLine(void const*   ptr,
                   unsigned long width_in_bytes);

  // Fill a rectangle with a constant color
  void BlockFill8(void*         dstptr,
                  long          dststride,
                  unsigned long dstleft,
                  unsigned long dsttop,
                  unsigned long width,
                  unsigned long height,
                  unsigned char color);
  void BlockFill16(void*          dstptr,
                   long           dststride,
                   unsigned long  dstleft,
                   unsigned long  dsttop,
                   unsigned long  width,
                   unsigned long  height,
                   unsigned short color);
  void BlockFill24(void*         dstptr,
                   long          dststride,
                   unsigned long dstleft,
                   unsigned long dsttop,
                   unsigned long width,
                   unsigned long height,
                   unsigned long color);
  void BlockFill32(void*         dstptr,
                   long          dststride,
                   unsigned long dstleft,
                   unsigned long dsttop,
                   unsigned long width,
                   unsigned long height,
                   unsigned long color);

  // Copy a rectangle of bytes
  // NOTE: No overlap detection
  void BlockCopy8(void*         dstptr,
                  long          dststride,
                  unsigned long dstleft,
                  unsigned long dsttop,
                  unsigned long width,
                  unsigned long height,
                  void const*   srcptr,
                  long          srcstride,
                  unsigned long srcleft,
                  unsigned long srctop);
  void BlockCopy16(void*         dstptr,
                   long          dststride,
                   unsigned long dstleft,
                   unsigned long dsttop,
                   unsigned long width,
                   unsigned long height,
                   void const*   srcptr,
                   long          srcstride,
                   unsigned long srcleft,
                   unsigned long srctop);
  void BlockCopy24(void*         dstptr,
                   long          dststride,
                   unsigned long dstleft,
                   unsigned long dsttop,
                   unsigned long width,
                   unsigned long height,
                   void const*   srcptr,
                   long          srcstride,
                   unsigned long srcleft,
                   unsigned long srctop);
  void BlockCopy32(void*         dstptr,
                   long          dststride,
                   unsigned long dstleft,
                   unsigned long dsttop,
                   unsigned long width,
                   unsigned long height,
                   void const*   srcptr,
                   long          srcstride,
                   unsigned long srcleft,
                   unsigned long srctop);

  // Copy and convert 8-bit palettized to 16 bit format
  void BlockCopyLUT8to16(void*                dstptr,
                         long                 dststride,
                         unsigned long        dstleft,
                         unsigned long        dsttop,
                         unsigned long        width,
                         unsigned long        height,
                         void const*          srcptr,
                         long                 srcstride,
                         unsigned long        srcleft,
                         unsigned long        srctop,
                         unsigned long const* lut);
  // Copy and convert 8-bit palettized to 24 bit format
  void BlockCopyLUT8to24(void*                dstptr,
                         long                 dststride,
                         unsigned long        dstleft,
                         unsigned long        dsttop,
                         unsigned long        width,
                         unsigned long        height,
                         void const*          srcptr,
                         long                 srcstride,
                         unsigned long        srcleft,
                         unsigned long        srctop,
                         unsigned long const* lut);
  // Copy and convert 8-bit palettized to 32 bit format
  void BlockCopyLUT8to32(void*                dstptr,
                         long                 dststride,
                         unsigned long        dstleft,
                         unsigned long        dsttop,
                         unsigned long        width,
                         unsigned long        height,
                         void const*          srcptr,
                         long                 srcstride,
                         unsigned long        srcleft,
                         unsigned long        srctop,
                         unsigned long const* lut);

  void BlockCopyRGB16toBGR24(void*         dstptr,
                             long          dststride,
                             unsigned long dstleft,
                             unsigned long dsttop,
                             unsigned long width,
                             unsigned long height,
                             void const*   srcptr,
                             long          srcstride,
                             unsigned long srcleft,
                             unsigned long srctop);
  // Copy and convert 16-bit RGB (5:6:5) to BGRx 32
  void BlockCopyRGB16toBGRx32(void*         dstptr,
                              long          dststride,
                              unsigned long dstleft,
                              unsigned long dsttop,
                              unsigned long width,
                              unsigned long height,
                              void const*   srcptr,
                              long          srcstride,
                              unsigned long srcleft,
                              unsigned long srctop);
  // Copy and convert packed 24bpp BGR to RGB16 (5:6:5)
  void BlockCopyBGR24toRGB16(void*         dstptr,
                             long          dststride,
                             unsigned long dstleft,
                             unsigned long dsttop,
                             unsigned long width,
                             unsigned long height,
                             void const*   srcptr,
                             long          srcstride,
                             unsigned long srcleft,
                             unsigned long srctop);
  void BlockCopyXYZ24toXYZx32(void*         dstptr,
                              long          dststride,
                              unsigned long dstleft,
                              unsigned long dsttop,
                              unsigned long width,
                              unsigned long height,
                              void const*   srcptr,
                              long          srcstride,
                              unsigned long srcleft,
                              unsigned long srctop);

  // Copy and convert 32-bit BGRx to 16-bit RGB (5:6:5)
  void BlockCopyBGRx32toRGB16(void*         dstptr,
                              long          dststride,
                              unsigned long dstleft,
                              unsigned long dsttop,
                              unsigned long width,
                              unsigned long height,
                              void const*   srcptr,
                              long          srcstride,
                              unsigned long srcleft,
                              unsigned long srctop);
  void BlockCopyXYZx32toXYZ24(void*         dstptr,
                              long          dststride,
                              unsigned long dstleft,
                              unsigned long dsttop,
                              unsigned long width,
                              unsigned long height,
                              void const*   srcptr,
                              long          srcstride,
                              unsigned long srcleft,
                              unsigned long srctop);

  // Alpha blend BGRx32 surface into RGB16 (5:6:5) using source alpha
  void AlphaBlendpBGRA32toRGB16(void*         dstptr,
                                long          dststride,
                                unsigned long dstleft,
                                unsigned long dsttop,
                                unsigned long width,
                                unsigned long height,
                                void const*   srcptr,
                                long          srcstride,
                                unsigned long srcleft,
                                unsigned long srctop);

  // Alpha blend premult BGRA32 surface into into premult BRGA32 using source alpha
  void AlphaBlendpBGRA32topBGRA32( void*         dstptr,
                                   long          dststride,
                                   unsigned long dstleft,
                                   unsigned long dsttop,
                                   unsigned long width,
                                   unsigned long height,
                                   void const*   srcptr,
                                   long          srcstride,
                                   unsigned long srcleft,
                                   unsigned long srctop);

  // Alpha blend non-premult BGRA32 surface into into premult BRGA32 using source alpha
  void AlphaBlendBGRA32topBGRA32( void*         dstptr,
                                  long          dststride,
                                  unsigned long dstleft,
                                  unsigned long dsttop,
                                  unsigned long width,
                                  unsigned long height,
                                  void const*   srcptr,
                                  long          srcstride,
                                  unsigned long srcleft,
                                  unsigned long srctop);

  // Un-premulitply pre-BGRA32
  void UnpremultiplyBGRA32(void*        dstptr,
                          long          dststride,
                          unsigned long dstleft,
                          unsigned long dsttop,
                          unsigned long width,
                          unsigned long height,
                          void const*   srcptr,
                          long          srcstride,
                          unsigned long srcleft,
                          unsigned long srctop);


  // Transparent copy 16 bpp surface to 16 bpp surface using 1 bit mask
  void MaskCopy16to16withA1(void*         dstptr,
                            long          dststride,
                            unsigned long dstleft,
                            unsigned long dsttop,
                            unsigned long width,
                            unsigned long height,
                            void const*   srcptr,
                            long          srcstride,
                            unsigned long srcleft,
                            unsigned long srctop,
                            void const*   mskptr,
                            long          mskstride,
                            unsigned long mskleft,
                            unsigned long msktop);
  // Transparent copy 32 bpp surface to 32 bpp surface using 1 bit mask
  void MaskCopy32to32withA1(void*         dstptr,
                            long          dststride,
                            unsigned long dstleft,
                            unsigned long dsttop,
                            unsigned long width,
                            unsigned long height,
                            void const*   srcptr,
                            long          srcstride,
                            unsigned long srcleft,
                            unsigned long srctop,
                            void const*   mskptr,
                            long          mskstride,
                            unsigned long mskleft,
                            unsigned long msktop);
}

void DumpParms(GPEBltParms* pParms)
{
  DEBUGMSG(GPE_ZONE_BLT, (TEXT("Dst(%d (%d bytes) x %d)=(%d, %d) - (%d, %d), fmt=%d (%d bpp)\n"),
            pParms->pDst->Width(),
            pParms->pDst->Stride(),
            pParms->pDst->Height(),
            pParms->prclDst->left,
            pParms->prclDst->top,
            pParms->prclDst->right,
            pParms->prclDst->bottom,
            pParms->pDst->Format(),
            pParms->pDst->BytesPerPixel() * 8));
  if(pParms->pSrc && pParms->prclSrc)
    DEBUGMSG(GPE_ZONE_BLT, (TEXT("Src(%d (%d bytes) x %d)=(%d, %d) - (%d, %d), fmt=%d (%d bpp)\n"),
              pParms->pSrc->Width(),
              pParms->pSrc->Stride(),
              pParms->pSrc->Height(),
              pParms->prclSrc->left,
              pParms->prclSrc->top,
              pParms->prclSrc->right,
              pParms->prclSrc->bottom,
              pParms->pSrc->Format(),
              pParms->pSrc->BytesPerPixel() * 8));
  if(pParms->pMask && pParms->prclMask)
    DEBUGMSG(GPE_ZONE_BLT, (TEXT("Mask(%d (%d bytes) x %d)=(%d, %d) - (%d, %d), fmt=%d (%d bpp)\n"),
              pParms->pMask->Width(),
              pParms->pMask->Stride(),
              pParms->pMask->Height(),
              pParms->prclMask->left,
              pParms->prclMask->top,
              pParms->prclMask->right,
              pParms->prclMask->bottom,
              pParms->pMask->Format(),
              pParms->pMask->BytesPerPixel() * 8));
  if(pParms->pBrush && pParms->pptlBrush)
    DEBUGMSG(GPE_ZONE_BLT, (TEXT("Brush(%d (%d bytes) x %d)=(%d, %d), fmt=%d (%d bpp)\n"),
              pParms->pBrush->Width(),
              pParms->pBrush->Stride(),
              pParms->pBrush->Height(),
              pParms->pptlBrush->x,
              pParms->pptlBrush->y,
              pParms->pBrush->Format(),
              pParms->pBrush->BytesPerPixel() * 8));
  if(pParms-> prclClip)
    DEBUGMSG(GPE_ZONE_BLT, (TEXT("Clip=(%d, %d) - (%d, %d) \n"),
              pParms->prclClip->left,
              pParms->prclClip->top,
              pParms->prclClip->right,
              pParms->prclClip->bottom));
  DEBUGMSG(GPE_ZONE_BLT, (TEXT("solidColor=0x%08X\n"), pParms->solidColor));
  DEBUGMSG(GPE_ZONE_BLT, (TEXT("bltFlags=0x%08X\n"), pParms->bltFlags));
  DEBUGMSG(GPE_ZONE_BLT, (TEXT("rop4=0x%08X\n"), pParms->rop4));
  DEBUGMSG(GPE_ZONE_BLT, (TEXT("xPositive=%d, yPositive=%d\n"),
            pParms->xPositive,
            pParms->yPositive));
  DEBUGMSG(GPE_ZONE_BLT, (TEXT("pLookup=0x%08X\n"), pParms->pLookup));
  DEBUGMSG(GPE_ZONE_BLT, (TEXT("pConvert=0x%08X\n"), pParms->pConvert));
  DEBUGMSG(GPE_ZONE_BLT, (TEXT("iMode=0x%08X\n"), pParms->iMode));
  DEBUGMSG(GPE_ZONE_BLT, (TEXT("blendFunction=0x%08X\n"), pParms->blendFunction));
}

//------------------------------------------------------------------------------
// From www.cl.cam.ac.uk - CRC Algorithm Three

#ifdef CHECKBLT
#define QUOTIENT 0x04C11DB7;

// Table used by algorithm below
unsigned int crctab[256] = {0};

// Fill in the table above
void crc32_init(void)
{
  int i, j;
  unsigned int crc;

  for(i = 0; i < 256; i++)
  {
    crc = i << 24;
    for(j = 0; j < 8; j++)
    {
      if(crc & 0x80000000)
      {
        crc = (crc << 1) ^ QUOTIENT;
      }
      else
      {
        crc = crc << 1;
      }
    }
    crctab[i] = crc;
  }
}

// Not used - reference code for 1-D CRC
unsigned int wombat(unsigned char* data,
                    int len)
{
  unsigned int result;
  int i;

  if(!crctab[0])
    crc32_init();

  result = *data++ << 24;
  result |= *data++ << 16;
  result |= *data++ << 8;
  result |= *data++;
  result = ~result;
  len -= 4;

  for(i = 0; i < len; i++)
  {
    result = (result << 8 | *data++) ^ crctab[result >> 24];
  }

  return(~result);
}

// Get CRC on 2-D array of bytes
unsigned int wombat2d(unsigned char* data,
                      long           stride,
                      unsigned long  width,
                      unsigned long  height)
{
  unsigned int result;
  unsigned long x, y;

  long step = stride - width;

  if(!crctab[0])
    crc32_init();

  result = *data++ << 24;
  result |= *data++ << 16;
  result |= *data++ << 8;
  result |= *data++;
  result = ~result;

  // Do first line without first four bytes
  for(x = 4; x < width; x++)
  {
    result = (result << 8 | *data++) ^ crctab[result >> 24];
  }
  data += step;

  // Do remaining lines
  for(y = 1; y < height; y++)
  {
    for(x = 0; x < width; x++)
    {
      result = (result << 8 | *data++) ^ crctab[result >> 24];
    }
    data += step;
  }

  return(~result);
}

bool CheckBlt(GPEBltParms* pParms)
{
  unsigned int mybltcrc;


  // Can't check BLTs when src and dst are the same surface
  if( pParms->pSrc == pParms->pDst )
    return TRUE;

  mybltcrc = wombat2d((unsigned char*)pParms->pDst->Buffer(),
                                   pParms->pDst->Stride(),
                                   pParms->pDst->Width(),
                                   pParms->pDst->Height());
  GPE* pGPE = GetGPE();
  pGPE->EmulatedBlt(pParms);
  return(mybltcrc == wombat2d((unsigned char*)pParms->pDst->Buffer(),
                          pParms->pDst->Stride(),
                          pParms->pDst->Width(),
                          pParms->pDst->Height()));
}
#endif // CHECKBLT

#if 0
// Optimized C version of packed BGR24 to RGB16 (5:6:5) for reference
void BlockCopyRGB24toRGB16_v1(unsigned short* dst,
                              long            dststride,
                              unsigned long   dstx,
                              unsigned long   dsty,
                              unsigned char*  src,
                              long            srcstride,
                              unsigned long   srcx,
                              unsigned long   srcy,
                              unsigned long   width,
                              unsigned long   height)
{
  unsigned long widthbytes = width + width;
  long dststep = dststride - widthbytes;
  widthbytes += width;
  long srcstep = srcstride - widthbytes;
  unsigned long pels = width;
  do
  {
    PreloadLine(src, width * 3);
    pels = width;
    do
    {
      *dst = ((((unsigned short)(*(src + 2))) << 8) & 0xF800) |
             ((((unsigned short)(*(src + 1))) << 3) & 0x07E0) |
             (((unsigned short)(*src)) >> 3);
      dst++;
      src += 3;
    } while(--pels);
    dst = (unsigned short*)(((long)dst) + dststep);
    src += srcstep;
  } while(--height);
}
#endif // 0

// Adjust source and destination for the clipping rectangle.
void ClipNoScale(RECTL*        prclDst,
                 RECTL const * prclClip)
{
  if(prclDst && prclClip)
  {
    if(prclClip->left > prclDst->left)
    {
      prclDst->left = prclClip->left;
    }
    if(prclClip->top > prclDst->top)
    {
      prclDst->top = prclClip->top;
    }
    if(prclClip->right < prclDst->right)
    {
      prclDst->right = prclClip->right;
    }
    if(prclClip->bottom < prclDst->bottom)
    {
      prclDst->bottom = prclClip->bottom;
    }
  }
}

// NOTE: Do not use if image is being scaled
void ClipNoScale(RECTL*       prclDst,
                 RECTL*       prclSrc,
                 RECTL const* prclClip)
{
  if(prclDst && prclClip)
  {
    if(prclClip->left > prclDst->left)
    {
      if(prclSrc)
        prclSrc->left += prclClip->left - prclDst->left;
      prclDst->left = prclClip->left;
    }
    if(prclClip->top > prclDst->top)
    {
      if(prclSrc)
        prclSrc->top += prclClip->top - prclDst->top;
      prclDst->top = prclClip->top;
    }
    if(prclClip->right < prclDst->right)
    {
      if(prclSrc)
      prclSrc->right -= prclDst->right - prclClip->right;
      prclDst->right = prclClip->right;
    }
    if(prclClip->bottom < prclDst->bottom)
    {
      if(prclSrc)
        prclSrc->bottom -= prclDst->bottom - prclClip->bottom;
      prclDst->bottom = prclClip->bottom;
    }
  }
}

void ClipNoScale(RECTL*       prclDst,
                 RECTL*       prclSrc,
                 POINTL*      pptlBrush,
                 RECTL const* prclClip)
{
  if(prclDst && prclClip)
  {
    if(prclClip->left > prclDst->left)
    {
      long leftclip = prclClip->left - prclDst->left;
      if(prclSrc)
        prclSrc->left += leftclip;
      if(pptlBrush)
        pptlBrush->x += leftclip;
      prclDst->left = prclClip->left;
    }
    if(prclClip->top > prclDst->top)
    {
      long topclip = prclClip->top - prclDst->top;
      if(prclSrc)
        prclSrc->top += topclip;
      if(pptlBrush)
        pptlBrush->y += topclip;
      prclDst->top = prclClip->top;
    }
    if(prclClip->right < prclDst->right)
    {
      long rightclip = prclDst->right - prclClip->right;
      if(prclSrc)
        prclSrc->right -= rightclip;
      prclDst->right = prclClip->right;
    }
    if(prclClip->bottom < prclDst->bottom)
    {
      long bottomclip = prclDst->bottom - prclClip->bottom;
      if(prclSrc)
        prclSrc->bottom -= bottomclip;
      prclDst->bottom = prclClip->bottom;
    }
  }
}

void ClipNoScale(RECTL*       prclDst,
                 RECTL*       prclSrc,
                 POINTL*      pptlBrush,
                 RECTL*       prclMask,
                 RECTL const* prclClip)
{
  if(prclDst && prclClip)
  {
    if(prclClip->left > prclDst->left)
    {
      long leftclip = prclClip->left - prclDst->left;
      if(prclSrc)
        prclSrc->left += leftclip;
      if(pptlBrush)
        pptlBrush->x += leftclip;
      if(prclMask)
        prclMask->left += leftclip;
      prclDst->left = prclClip->left;
    }
    if(prclClip->top > prclDst->top)
    {
      long topclip = prclClip->top - prclDst->top;
      if(prclSrc)
        prclSrc->top += topclip;
      if(pptlBrush)
        pptlBrush->y += topclip;
      if(prclMask)
        prclMask->top += topclip;
      prclDst->top = prclClip->top;
    }
    if(prclClip->right < prclDst->right)
    {
      long rightclip = prclDst->right - prclClip->right;
      if(prclSrc)
        prclSrc->right -= rightclip;
      if(prclMask)
        prclMask->right -= rightclip;
      prclDst->right = prclClip->right;
    }
    if(prclClip->bottom < prclDst->bottom)
    {
      long bottomclip = prclDst->bottom - prclClip->bottom;
      if(prclSrc)
        prclSrc->bottom -= bottomclip;
      if(prclMask)
        prclMask->bottom -= bottomclip;
      prclDst->bottom = prclClip->bottom;
    }
  }
}

// Flip Blt params if needed
void AdjustForBottomUp(void*&        pDst,
                       long&         dststride,
                       unsigned long dstheight,
                       RECTL&        rclDst,
                       void*&        pSrc,
                       long&         srcstride,
                       unsigned long srcheight,
                       RECTL&        rclSrc)
{
  long tmp = rclDst.bottom;
  rclDst.bottom = dstheight - rclDst.top;
  rclDst.top = dstheight - tmp;
  tmp = rclSrc.bottom;
  rclSrc.bottom = srcheight - rclSrc.top;
  rclSrc.top = srcheight - tmp;
  dststride = -dststride;
  srcstride = -srcstride;
  pDst = (void*)((long)pDst - ((dstheight - 1) * dststride));
  pSrc = (void*)((long)pSrc - ((srcheight - 1) * srcstride));
}

// Start the Blt delegation tree by determining type of ROP
SCODE OMAPDDGPE::DesignateBlt(GPEBltParms* pParms)
{
#if 0
  DEBUGMSG(GPE_ZONE_BLT, (TEXT("****** BLT ******\r\n")));
  DumpParms(pParms);
#endif

  BLTFN pfnSavedBlt = pParms->pBlt;

  // Default to base emulated routine
  pParms->pBlt = &GPE::EmulatedBlt;

  ROP4 rop4 = pParms->rop4;

  ROP4 rop3 = rop4 & 0xFF;
  if((rop4 >> 8) != rop3)
  {
    DesignateBltROP4(pParms, rop4);
  }
  else
  {
    ROP4 rop2 = rop3 & 0xF;
    if((rop3 >> 4) != rop2)
    {
      DesignateBltROP3(pParms, rop4);
    }
    else
    {
      ROP4 rop1 = rop2 & 0x3;
      if((rop2 >> 2) != rop1)
      {
        DesignateBltROP2(pParms, rop4);
      }
      else
      {
        DesignateBltROP1(pParms, rop4);
      }
    }
  }

#if DEBUG
	if (pParms->pBlt == &GPE::EmulatedBlt)
		DEBUGMSG(GPE_ZONE_BLT, (TEXT("set BLT to Emulated BLT\r\n")));
	else
		DEBUGMSG(GPE_ZONE_BLT, (TEXT("set BLT to NEON BLT\r\n")));
#endif

#if DEBUG_NEON_MEMORY_LEAK
  BLTFN pNeonBlt;

  if (pParms->pBlt != &GPE::EmulatedBlt)
  {
    m_MemoryStatusBefore.dwLength = sizeof(MEMORYSTATUS);
    GlobalMemoryStatus(&m_MemoryStatusBefore);
    m_bNeonBlt = TRUE;
    pNeonBlt = pParms->pBlt;
  }
#endif

  // Do the BLT here
  SCODE scode = (this->*(pParms->pBlt))(pParms);

#if DEBUG_NEON_MEMORY_LEAK
  if (m_bNeonBlt)
  {
    m_bNeonBlt = FALSE;

    m_MemoryStatusAfter.dwLength = sizeof(MEMORYSTATUS);
    GlobalMemoryStatus(&m_MemoryStatusAfter);

    m_dwAvailPhysDelta = m_MemoryStatusAfter.dwAvailPhys - m_MemoryStatusBefore.dwAvailPhys;
	m_dwAvailPageFileDelta = m_MemoryStatusAfter.dwAvailPageFile - m_MemoryStatusBefore.dwAvailPageFile;

    if (m_dwAvailPhysDelta || m_dwAvailPageFileDelta)
	{
      TCHAR *pszBltName;

      if (pNeonBlt == &OMAPDDGPE::EmulatedBlockFill8)
          pszBltName = L"EmulatedBlockFill8";
      else if (pNeonBlt == &OMAPDDGPE::EmulatedBlockCopy8to8)
          pszBltName = L"EmulatedBlockCopy8to8";
      else if (pNeonBlt == &OMAPDDGPE::EmulatedBlockFill16)
          pszBltName = L"EmulatedBlockFill16";
      else if (pNeonBlt == &OMAPDDGPE::EmulatedBlockCopyLUT8to16)
          pszBltName = L"EmulatedBlockCopyLUT8to16";
      else if (pNeonBlt == &OMAPDDGPE::EmulatedBlockCopy16to16)
          pszBltName = L"EmulatedBlockCopy16to16";
      else if (pNeonBlt == &OMAPDDGPE::EmulatedBlockCopyBGR24toRGB16)
          pszBltName = L"EmulatedBlockCopyBGR24toRGB16";
      else if (pNeonBlt == &OMAPDDGPE::EmulatedBlockCopyBGRx32toRGB16)
          pszBltName = L"EmulatedBlockCopyBGRx32toRGB16";
      else if (pNeonBlt == &OMAPDDGPE::EmulatedBlockFill24)
          pszBltName = L"EmulatedBlockFill24";
      else if (pNeonBlt == &OMAPDDGPE::EmulatedBlockCopyLUT8to24)
          pszBltName = L"EmulatedBlockCopyLUT8to24";
      else if (pNeonBlt == &OMAPDDGPE::EmulatedBlockCopyRGB16toBGR24)
          pszBltName = L"EmulatedBlockCopyRGB16toBGR24";
      else if (pNeonBlt == &OMAPDDGPE::EmulatedBlockCopy24to24)
          pszBltName = L"EmulatedBlockCopy24to24";
      else if (pNeonBlt == &OMAPDDGPE::EmulatedBlockCopyXYZx32toXYZ24)
          pszBltName = L"EmulatedBlockCopyXYZx32toXYZ24";
      else if (pNeonBlt == &OMAPDDGPE::EmulatedBlockFill32)
          pszBltName = L"EmulatedBlockFill32";
      else if (pNeonBlt == &OMAPDDGPE::EmulatedBlockCopyLUT8to32)
          pszBltName = L"EmulatedBlockCopyLUT8to32";
      else if (pNeonBlt == &OMAPDDGPE::EmulatedBlockCopyRGB16toBGRx32)
          pszBltName = L"EmulatedBlockCopyRGB16toBGRx32";
      else if (pNeonBlt == &OMAPDDGPE::EmulatedBlockCopyXYZ24toXYZx32)
          pszBltName = L"EmulatedBlockCopyXYZ24toXYZx32";
      else if (pNeonBlt == &OMAPDDGPE::EmulatedBlockCopy32to32)
          pszBltName = L"EmulatedBlockCopy32to32";
      else if (pNeonBlt == &OMAPDDGPE::EmulatedMaskCopy16to16withA1)
          pszBltName = L"EmulatedMaskCopy16to16withA1";
      else if (pNeonBlt == &OMAPDDGPE::EmulatedMaskCopy32to32withA1)
          pszBltName = L"EmulatedMaskCopy32to32withA1";
      else if (pNeonBlt == &OMAPDDGPE::EmulatedPerPixelAlphaBlendBGRA32toRGB16)
          pszBltName = L"EmulatedPerPixelAlphaBlendBGRA32toRGB16";
      else
          pszBltName = L"Unknown";
			
      RETAILMSG(1, (L"NEON BLT:%s AvailPhysDelta = %d, AvailPageFileDelta = %d\r\n", pszBltName, (long) m_dwAvailPhysDelta, (long) m_dwAvailPageFileDelta));
	}
  }
#endif

  pParms->pBlt = pfnSavedBlt;

  return(scode);
}

// ROP1s
void OMAPDDGPE::DesignateBltROP1(GPEBltParms* pParms,
                                 ROP4         rop1)
{
//  GPEBltParms parms = *pParms;
//  if(!(parms.bltFlags & BLT_STRETCH))
//    ClipNoScale(parms.prclDst, parms.prclClip);
//  if(((parms.prclDst->right - parms.prclDst->left) > 0) &&
//     ((parms.prclDst->bottom - parms.prclDst->top) > 0))
//  {
//    switch(rop1)
//    {
//    }
//  }
//  pParms->pBlt = parms.pBlt;
    UNREFERENCED_PARAMETER(rop1);
    UNREFERENCED_PARAMETER(pParms);
}

// ROP2s
void OMAPDDGPE::DesignateBltROP2(GPEBltParms* pParms,
                                 ROP4         rop2)
{
  GPEBltParms parms = *pParms;
  if(!(parms.bltFlags & BLT_STRETCH))
    ClipNoScale(parms.prclDst, parms.prclSrc, parms.prclClip);

  if(((parms.prclDst->right - parms.prclDst->left) > 0) &&
     ((parms.prclDst->bottom - parms.prclDst->top) > 0))
  {
    switch(rop2)
    {
    case 0xCCCC:  // SRCCOPY
      DesignateBltSRCCOPY(&parms);
      break;
    }
  }
  pParms->pBlt = parms.pBlt;
}

void OMAPDDGPE::DesignateBltSRCCOPY(GPEBltParms* pParms)
{
  switch(pParms->pDst->Format())
  {
  case gpe8Bpp:
    DesignateBltSRCCOPY_toLUT8(pParms);
    break;

  case gpe16Bpp:
    DesignateBltSRCCOPY_toRGB16(pParms);
    break;

  case gpe24Bpp:
    // Check for RGB24 vs BGR24
    if( pParms->pDst->FormatPtr()->m_pPalette != NULL &&
        pParms->pDst->FormatPtr()->m_pPalette[0] == 0x00ff0000 )
        {
        DesignateBltSRCCOPY_toBGR24(pParms);
        }
    break;

  case gpe32Bpp:
    if( pParms->pDst->FormatPtr()->m_pPalette != NULL &&
        pParms->pDst->FormatPtr()->m_pPalette[0] == 0x00ff0000 )
        {
        DesignateBltSRCCOPY_toBGRA32(pParms);
        }
    break;
  }
}

void OMAPDDGPE::DesignateBltSRCCOPY_toLUT8(GPEBltParms* pParms)
{
  if(pParms->pSrc)
  {
    switch(pParms->pSrc->Format())
    {
    case gpe8Bpp:
      DesignateBltSRCCOPY_LUT8toLUT8(pParms);
      break;
    }
  }
}

void OMAPDDGPE::DesignateBltSRCCOPY_LUT8toLUT8(GPEBltParms* pParms)
{
  RECTL*  prclDst =  pParms->prclDst;
  DWORD   dwWidth  = prclDst->right - prclDst->left;

  // Can't handle transparency, alpha, or stretch (yet)
  if( (pParms->bltFlags & ~(BLT_WAITVSYNC|BLT_WAITNOTBUSY)) == 0 )
  {
    // EmulatedBlockCopy only when width is multiple of 8.
    if (( dwWidth % 8 ) == 0)
        pParms->pBlt = (BLTFN)&OMAPDDGPE::EmulatedBlockCopy8to8;
  }
}

void OMAPDDGPE::DesignateBltSRCCOPY_toRGB16(GPEBltParms* pParms)
{
  if(pParms->pSrc)
  {
    switch(pParms->pSrc->Format())
    {
    case gpe8Bpp:
      DesignateBltSRCCOPY_LUT8toRGB16(pParms);
      break;

    case gpe16Bpp:      
      DesignateBltSRCCOPY_RGB16toRGB16(pParms);
      break;

    case gpe24Bpp:      
      DesignateBltSRCCOPY_BGR24toRGB16(pParms);
      break;

    case gpe32Bpp:
      DesignateBltSRCCOPY_BGRA32toRGB16(pParms);
      break;
    }
  }
}

void OMAPDDGPE::DesignateBltSRCCOPY_LUT8toRGB16(GPEBltParms* pParms)
{
  // Can't handle transparency, alpha, or stretch (yet)
  if( (pParms->bltFlags & ~(BLT_WAITVSYNC|BLT_WAITNOTBUSY)) == 0 )
  {
    pParms->pBlt = (BLTFN)&OMAPDDGPE::EmulatedBlockCopyLUT8to16;
  }
}

void OMAPDDGPE::DesignateBltSRCCOPY_RGB16toRGB16(GPEBltParms* pParms)
{
  // Can't handle transparency, alpha, or stretch (yet)
  if( (pParms->bltFlags & ~(BLT_WAITVSYNC|BLT_WAITNOTBUSY)) == 0 )
  {
    pParms->pBlt = (BLTFN)&OMAPDDGPE::EmulatedBlockCopy16to16;
  }
}

void OMAPDDGPE::DesignateBltSRCCOPY_BGR24toRGB16(GPEBltParms* pParms)
{
  // Can't handle transparency, alpha, or stretch (yet)
  if( (pParms->bltFlags & ~(BLT_WAITVSYNC|BLT_WAITNOTBUSY)) == 0 )
  {
    if (pParms->pDst->FormatPtr()->m_pPalette)
    {
      //Can't handle other format than RGB 565
      if (pParms->pDst->FormatPtr()->m_pPalette[0] == 0x0000f800 &&
      pParms->pDst->FormatPtr()->m_pPalette[1] == 0x000007e0 &&
      pParms->pDst->FormatPtr()->m_pPalette[2] == 0x0000001f)
      {
        pParms->pBlt = (BLTFN)&OMAPDDGPE::EmulatedBlockCopyBGR24toRGB16;
      }
    }
  }
}

void OMAPDDGPE::DesignateBltSRCCOPY_BGRA32toRGB16(GPEBltParms* pParms)
{
  // Not transparency, alpha, or stretch
  if( (pParms->bltFlags & ~(BLT_WAITVSYNC|BLT_WAITNOTBUSY)) == 0 )
  {
    pParms->pBlt = (BLTFN)&OMAPDDGPE::EmulatedBlockCopyBGRx32toRGB16;
  }
  else if((pParms->bltFlags == BLT_ALPHABLEND) && // Only handle transparency
          (*((unsigned long*)(&(pParms->blendFunction))) == 0x01FF0000))  // Only handle per-pixel, premultiplied
  {
    pParms->pBlt = (BLTFN)&OMAPDDGPE::EmulatedPerPixelAlphaBlendBGRA32toRGB16;
  }
}

void OMAPDDGPE::DesignateBltSRCCOPY_toBGR24(GPEBltParms* pParms)
{
  if(pParms->pSrc)
  {
    switch(pParms->pSrc->Format())
    {
    case gpe8Bpp:
      DesignateBltSRCCOPY_LUT8toBGR24(pParms);
      break;

    case gpe16Bpp:
      DesignateBltSRCCOPY_RGB16toBGR24(pParms);
      break;

    case gpe24Bpp:
      DesignateBltSRCCOPY_BGR24toBGR24(pParms);
      break;

    case gpe32Bpp:
      DesignateBltSRCCOPY_BGRA32toBGR24(pParms);
      break;
    }
  }
}

void OMAPDDGPE::DesignateBltSRCCOPY_LUT8toBGR24(GPEBltParms* pParms)
{
  // Can't handle transparency, alpha, or stretch (yet)
  if( (pParms->bltFlags & ~(BLT_WAITVSYNC|BLT_WAITNOTBUSY)) == 0 )
  {
    pParms->pBlt = (BLTFN)&OMAPDDGPE::EmulatedBlockCopyLUT8to24;
  }
}

void OMAPDDGPE::DesignateBltSRCCOPY_RGB16toBGR24(GPEBltParms* pParms)
{
  // Can't handle transparency, alpha, or stretch (yet)
  if( (pParms->bltFlags & ~(BLT_WAITVSYNC|BLT_WAITNOTBUSY)) == 0 )
  {
    if (pParms->pSrc->FormatPtr()->m_pPalette)
    {
      //Can't handle other format than RGB 565
      if (pParms->pSrc->FormatPtr()->m_pPalette[0] == 0x0000f800 &&
      pParms->pSrc->FormatPtr()->m_pPalette[1] == 0x000007e0 &&
      pParms->pSrc->FormatPtr()->m_pPalette[2] == 0x0000001f)
      {
        pParms->pBlt = (BLTFN)&OMAPDDGPE::EmulatedBlockCopyRGB16toBGR24;
      }    
    }
  }
}

void OMAPDDGPE::DesignateBltSRCCOPY_BGR24toBGR24(GPEBltParms* pParms)
{
  // Can't handle transparency, alpha, or stretch (yet)
  if( (pParms->bltFlags & ~(BLT_WAITVSYNC|BLT_WAITNOTBUSY)) == 0 )
  {
    pParms->pBlt = (BLTFN)&OMAPDDGPE::EmulatedBlockCopy24to24;
  }
}

void OMAPDDGPE::DesignateBltSRCCOPY_BGRA32toBGR24(GPEBltParms* pParms)
{
  // Can't handle transparency, alpha, or stretch (yet)
  if( (pParms->bltFlags & ~(BLT_WAITVSYNC|BLT_WAITNOTBUSY)) == 0 )
  {
    pParms->pBlt = (BLTFN)&OMAPDDGPE::EmulatedBlockCopyXYZx32toXYZ24;
  }
}

void OMAPDDGPE::DesignateBltSRCCOPY_toBGRA32(GPEBltParms* pParms)
{
  if(pParms->pSrc)
  {
    switch(pParms->pSrc->Format())
    {
    case gpe8Bpp:
      DesignateBltSRCCOPY_LUT8toBGRA32(pParms);
      break;

    case gpe16Bpp:
      DesignateBltSRCCOPY_RGB16toBGRA32(pParms);
      break;

    case gpe24Bpp:
      DesignateBltSRCCOPY_BGR24toBGRA32(pParms);
      break;

    case gpe32Bpp:
      DesignateBltSRCCOPY_BGRA32toBGRA32(pParms);
      break;
    }
  }
}

void OMAPDDGPE::DesignateBltSRCCOPY_LUT8toBGRA32(GPEBltParms* pParms)
{
  // Can't handle transparency, alpha, or stretch (yet)
  if( (pParms->bltFlags & ~(BLT_WAITVSYNC|BLT_WAITNOTBUSY)) == 0 )
  {
    pParms->pBlt = (BLTFN)&OMAPDDGPE::EmulatedBlockCopyLUT8to32;
  }
}

void OMAPDDGPE::DesignateBltSRCCOPY_RGB16toBGRA32(GPEBltParms* pParms)
{
  // Can't handle transparency, alpha, or stretch (yet)
  if( (pParms->bltFlags & ~(BLT_WAITVSYNC|BLT_WAITNOTBUSY)) == 0 )
  {
    //Can't handle other format than RGB 565
    if (pParms->pSrc->FormatPtr()->m_pPalette[0] == 0x0000f800 &&
    pParms->pSrc->FormatPtr()->m_pPalette[1] == 0x000007e0 &&
    pParms->pSrc->FormatPtr()->m_pPalette[2] == 0x0000001f)
    {
      pParms->pBlt = (BLTFN)&OMAPDDGPE::EmulatedBlockCopyRGB16toBGRx32;
    }
  }
}

void OMAPDDGPE::DesignateBltSRCCOPY_BGR24toBGRA32(GPEBltParms* pParms)
{
  // Can't handle transparency, alpha, or stretch (yet)
  if( (pParms->bltFlags & ~(BLT_WAITVSYNC|BLT_WAITNOTBUSY)) == 0 )
  {
    pParms->pBlt = (BLTFN)&OMAPDDGPE::EmulatedBlockCopyXYZ24toXYZx32;
  }
}

void OMAPDDGPE::DesignateBltSRCCOPY_BGRA32toBGRA32(GPEBltParms* pParms)
{
  // Can't handle transparency, alpha, or stretch (yet)
  if( (pParms->bltFlags & ~(BLT_WAITVSYNC|BLT_WAITNOTBUSY)) == 0 )
  {
    pParms->pBlt = (BLTFN)&OMAPDDGPE::EmulatedBlockCopy32to32;
  }
}

// ROP3s
void OMAPDDGPE::DesignateBltROP3(GPEBltParms* pParms,
                                 ROP4         rop3)
{
  GPEBltParms parms = *pParms;
  if(!(parms.bltFlags & BLT_STRETCH))
    ClipNoScale(parms.prclDst, parms.prclSrc, parms.pptlBrush, parms.prclClip);

  if(((parms.prclDst->right - parms.prclDst->left) > 0) &&
     ((parms.prclDst->bottom - parms.prclDst->top) > 0))
  {
    switch(rop3)
    {
    case 0xF0F0:  // PATCOPY
      DesignateBltPATCOPY(&parms);
      break;
    }
  }
  pParms->pBlt = parms.pBlt;
}

void OMAPDDGPE::DesignateBltPATCOPY(GPEBltParms* pParms)
{
  switch(pParms->pDst->Format())
  {
  case gpe8Bpp:
    DesignateBltPATCOPY_toLUT8(pParms);
    break;

  case gpe16Bpp:
    DesignateBltPATCOPY_toRGB16(pParms);
    break;

  case gpe24Bpp:
    DesignateBltPATCOPY_toBGR24(pParms);
    break;

  case gpe32Bpp:
    DesignateBltPATCOPY_toBGRA32(pParms);
    break;
  }
}

void OMAPDDGPE::DesignateBltPATCOPY_toLUT8(GPEBltParms* pParms)
{
  if(pParms->solidColor != -1)
    DesignateBltPATCOPY_solidtoLUT8(pParms);
}

void OMAPDDGPE::DesignateBltPATCOPY_solidtoLUT8(GPEBltParms* pParms)
{
  pParms->pBlt = (BLTFN)&OMAPDDGPE::EmulatedBlockFill8;
}

void OMAPDDGPE::DesignateBltPATCOPY_toRGB16(GPEBltParms* pParms)
{
  if(pParms->solidColor != -1)
    DesignateBltPATCOPY_solidtoRGB16(pParms);
}

void OMAPDDGPE::DesignateBltPATCOPY_solidtoRGB16(GPEBltParms* pParms)
{
  pParms->pBlt = (BLTFN)&OMAPDDGPE::EmulatedBlockFill16;
}

void OMAPDDGPE::DesignateBltPATCOPY_toBGR24(GPEBltParms* pParms)
{
  if(pParms->solidColor != -1)
    DesignateBltPATCOPY_solidtoBGR24(pParms);
}

void OMAPDDGPE::DesignateBltPATCOPY_solidtoBGR24(GPEBltParms* pParms)
{
  pParms->pBlt = (BLTFN)&OMAPDDGPE::EmulatedBlockFill24;
}

void OMAPDDGPE::DesignateBltPATCOPY_toBGRA32(GPEBltParms* pParms)
{
  if(pParms->solidColor != -1)
    DesignateBltPATCOPY_solidtoBGRA32(pParms);
}

void OMAPDDGPE::DesignateBltPATCOPY_solidtoBGRA32(GPEBltParms* pParms)
{
  pParms->pBlt = (BLTFN)&OMAPDDGPE::EmulatedBlockFill32;
}

// ROP4s
void OMAPDDGPE::DesignateBltROP4(GPEBltParms* pParms,
                                 ROP4         rop4)
{
#if 0
  RETAILMSG(1, (TEXT("****** BLT ******\n")));
  DumpParms(pParms);
#endif
  GPEBltParms parms = *pParms;
  if(!(parms.bltFlags & BLT_STRETCH))
    ClipNoScale(parms.prclDst, parms.prclSrc, parms.pptlBrush, parms.prclMask, parms.prclClip);

  if(((parms.prclDst->right - parms.prclDst->left) > 0) &&
     ((parms.prclDst->bottom - parms.prclDst->top) > 0))
  {
    switch(rop4)
    {
    case 0xAACC:  // Masked copy
      DesignateBltAACC(&parms);
      break;
    }
  }
  pParms->pBlt = parms.pBlt;
}

void OMAPDDGPE::DesignateBltAACC(GPEBltParms* pParms)
{
  switch(pParms->pDst->Format())
  {
  case gpe16Bpp:
     DesignateBltAACC_toRGB16(pParms);
     break;

  case gpe32Bpp:
     DesignateBltAACC_toBGRA32(pParms);
     break;
  }
}

void OMAPDDGPE::DesignateBltAACC_toRGB16(GPEBltParms* pParms)
{
  if(pParms->pSrc)
  {
    switch(pParms->pSrc->Format())
    {
    case gpe16Bpp:
       DesignateBltAACC_RGB16toRGB16(pParms);
       break;
    }
  }
}

void OMAPDDGPE::DesignateBltAACC_RGB16toRGB16(GPEBltParms* pParms)
{
  // Can't handle transparency, alpha, or stretch (yet)
  if( (pParms->bltFlags & ~(BLT_WAITVSYNC|BLT_WAITNOTBUSY)) == 0 )
  {
    //Avoid negative mask stride
    if(pParms->pMask->Stride() > 0)
      pParms->pBlt = (BLTFN)&OMAPDDGPE::EmulatedMaskCopy16to16withA1;
  }
}

void OMAPDDGPE::DesignateBltAACC_toBGRA32(GPEBltParms* pParms)
{
  if(pParms->pSrc)
  {
    switch(pParms->pSrc->Format())
    {
    case gpe32Bpp:
       DesignateBltAACC_BGRA32toBGRA32(pParms);
       break;
    }
  }
}

void OMAPDDGPE::DesignateBltAACC_BGRA32toBGRA32(GPEBltParms* pParms)
{
  // Can't handle transparency, alpha, or stretch (yet)
  if( (pParms->bltFlags & ~(BLT_WAITVSYNC|BLT_WAITNOTBUSY)) == 0 )
  {
//    pParms->pBlt = (BLTFN)&OMAPDDGPE::EmulatedMaskCopy32to32withA1;
  }
}

//
// Final level Blts
//
SCODE OMAPDDGPE::EmulatedBlockFill8(GPEBltParms* pParms)
{
CALLOUT(L"EmulatedBlockFill8()");

  void* dstptr = pParms->pDst->Buffer();
  long dststride = pParms->pDst->Stride();
  RECTL rclDst = *(pParms->prclDst);

  WaitForNotBusy();

  BlockFill8(dstptr,
             dststride,
             rclDst.left,
             rclDst.top,
             rclDst.right - rclDst.left,
             rclDst.bottom - rclDst.top,
             (unsigned char)pParms->solidColor);

#ifdef CHECKBLT
  if(!CheckBlt(pParms))
  {
    OutputDebugString(TEXT("** BAD BLT **: ") TEXT(__FUNCTION__) TEXT("()"));
    DebugBreak();
  }
#endif

  return(S_OK);
}

SCODE OMAPDDGPE::EmulatedBlockFill16(GPEBltParms* pParms)
{
CALLOUT(L"EmulatedBlockFill16()");

  void* dstptr = pParms->pDst->Buffer();
  long dststride = pParms->pDst->Stride();
  RECTL rclDst = *(pParms->prclDst);

  WaitForNotBusy();
  BlockFill16(dstptr,
              dststride,
              rclDst.left,
              rclDst.top,
              rclDst.right - rclDst.left,
              rclDst.bottom - rclDst.top,
              (unsigned short)pParms->solidColor);

#ifdef CHECKBLT
  if(!CheckBlt(pParms))
  {
    OutputDebugString(TEXT("** BAD BLT **: ") TEXT(__FUNCTION__) TEXT("()"));
    DebugBreak();
  }
#endif

  return(S_OK);
}

SCODE OMAPDDGPE::EmulatedBlockFill24(GPEBltParms* pParms)
{
CALLOUT(L"EmulatedBlockFill24()");

  void* dstptr = pParms->pDst->Buffer();
  long dststride = pParms->pDst->Stride();
  RECTL rclDst = *(pParms->prclDst);

  WaitForNotBusy();
  BlockFill24(dstptr,
              dststride,
              rclDst.left,
              rclDst.top,
              rclDst.right - rclDst.left,
              rclDst.bottom - rclDst.top,
              (unsigned long)pParms->solidColor);

#ifdef CHECKBLT
  if(!CheckBlt(pParms))
  {
    OutputDebugString(TEXT("** BAD BLT **: ") TEXT(__FUNCTION__) TEXT("()"));
    DebugBreak();
  }
#endif

  return(S_OK);
}

SCODE OMAPDDGPE::EmulatedBlockFill32(GPEBltParms* pParms)
{
CALLOUT(L"EmulatedBlockFill32()");

  void* dstptr = pParms->pDst->Buffer();
  long dststride = pParms->pDst->Stride();
  RECTL rclDst = *(pParms->prclDst);

  WaitForNotBusy();
  BlockFill32(dstptr,
              dststride,
              rclDst.left,
              rclDst.top,
              rclDst.right - rclDst.left,
              rclDst.bottom - rclDst.top,
              (unsigned long)pParms->solidColor);

#ifdef CHECKBLT
  if(!CheckBlt(pParms))
  {
    OutputDebugString(TEXT("** BAD BLT **: ") TEXT(__FUNCTION__) TEXT("()"));
    DebugBreak();
  }
#endif

  return(S_OK);
}

SCODE OMAPDDGPE::EmulatedBlockCopy8to8(GPEBltParms* pParms)
{
  // Can't handle right to left yet
  if(pParms->xPositive != 1)
    return(GPE::EmulatedBlt(pParms));

  // Can't handle conversion routines
  if(pParms->pConvert)
    return(GPE::EmulatedBlt(pParms));

CALLOUT(L"EmulatedBlockCopy8to8()");

  void* dstptr = pParms->pDst->Buffer();
  long dststride = pParms->pDst->Stride();
  void* srcptr = pParms->pSrc->Buffer();
  long srcstride = pParms->pSrc->Stride();
  RECTL rclDst = *(pParms->prclDst);
  RECTL rclSrc = *(pParms->prclSrc);

  if(pParms->yPositive != 1)
    AdjustForBottomUp(dstptr, dststride, pParms->pDst->Height(), rclDst,
                      srcptr, srcstride, pParms->pSrc->Height(), rclSrc);

  WaitForNotBusy();
  BlockCopy8(dstptr,
             dststride,
             rclDst.left,
             rclDst.top,
             rclDst.right - rclDst.left,
             rclDst.bottom - rclDst.top,
             srcptr,
             srcstride,
             rclSrc.left,
             rclSrc.top);

#ifdef CHECKBLT
  if(!CheckBlt(pParms))
  {
    OutputDebugString(TEXT("** BAD BLT **: ") TEXT(__FUNCTION__) TEXT("()"));
    DebugBreak();
  }
#endif

  return(S_OK);
}

SCODE OMAPDDGPE::EmulatedBlockCopyLUT8to16(GPEBltParms* pParms)
{
CALLOUT(L"EmulatedBlockCopyLUT8to16()");

  void* dstptr = pParms->pDst->Buffer();
  long dststride = pParms->pDst->Stride();
  void* srcptr = pParms->pSrc->Buffer();
  long srcstride = pParms->pSrc->Stride();
  RECTL rclDst = *(pParms->prclDst);
  RECTL rclSrc = *(pParms->prclSrc);

  WaitForNotBusy();
  BlockCopyLUT8to16(dstptr,
                    dststride,
                    rclDst.left,
                    rclDst.top,
                    rclDst.right - rclDst.left,
                    rclDst.bottom - rclDst.top,
                    srcptr,
                    srcstride,
                    rclSrc.left,
                    rclSrc.top,
                    pParms->pLookup);

#ifdef CHECKBLT
  if(!CheckBlt(pParms))
  {
    OutputDebugString(TEXT("** BAD BLT **: ") TEXT(__FUNCTION__) TEXT("()"));
    DebugBreak();
  }
#endif

  return(S_OK);
}

SCODE OMAPDDGPE::EmulatedBlockCopy16to16(GPEBltParms* pParms)
{
  // Can't handle right to left yet
  if(pParms->xPositive != 1)
    return(GPE::EmulatedBlt(pParms));

  // Can't handle lookup tables or conversion routines
  if(pParms->pLookup ||
     pParms->pConvert)
    return(GPE::EmulatedBlt(pParms));

CALLOUT(L"EmulatedBlockCopy16to16()");

  void* dstptr = pParms->pDst->Buffer();
  long dststride = pParms->pDst->Stride();
  void* srcptr = pParms->pSrc->Buffer();
  long srcstride = pParms->pSrc->Stride();
  RECTL rclDst = *(pParms->prclDst);
  RECTL rclSrc = *(pParms->prclSrc);

  if(pParms->yPositive != 1)
    AdjustForBottomUp(dstptr, dststride, pParms->pDst->Height(), rclDst,
                      srcptr, srcstride, pParms->pSrc->Height(), rclSrc);

  WaitForNotBusy();
  BlockCopy16(dstptr,
              dststride,
              rclDst.left,
              rclDst.top,
              rclDst.right - rclDst.left,
              rclDst.bottom - rclDst.top,
              srcptr,
              srcstride,
              rclSrc.left,
              rclSrc.top);

#ifdef CHECKBLT
  if(!CheckBlt(pParms))
  {
    OutputDebugString(TEXT("** BAD BLT **: ") TEXT(__FUNCTION__) TEXT("()"));
    DebugBreak();
  }
#endif

  return(S_OK);
}

SCODE OMAPDDGPE::EmulatedMaskCopy16to16withA1(GPEBltParms* pParms)
{
CALLOUT(L"EmulatedMaskCopy16to16withA1()");

#ifdef CHECKBLT
//DebugBreak();
GPEBltParms SavedParms = *pParms;
ULONG size = abs(SavedParms.pDst->Stride()) * SavedParms.pDst->Height();
void* buffolddst = malloc(size);
// Save original destination
if(SavedParms.pDst->Stride() < 0)
  memcpy(buffolddst, (void*)((long)SavedParms.pDst->Buffer() - ((SavedParms.pDst->Height() - 1) * SavedParms.pDst->Stride())), size);
else
  memcpy(buffolddst, SavedParms.pDst->Buffer(), size);
#endif
  void* dstptr = pParms->pDst->Buffer();
  long dststride = pParms->pDst->Stride();
  void const* srcptr = pParms->pSrc->Buffer();
  long srcstride = pParms->pSrc->Stride();
  void const* mskptr = pParms->pMask->Buffer();
  long mskstride = pParms->pMask->Stride();
  RECTL rclDst = *(pParms->prclDst);
  RECTL rclSrc = *(pParms->prclSrc);
  RECTL rclMask = *(pParms->prclMask);

  WaitForNotBusy();
  MaskCopy16to16withA1(dstptr,
                       dststride,
                       rclDst.left,
                       rclDst.top,
                       rclDst.right - rclDst.left,
                       rclDst.bottom - rclDst.top,
                       srcptr,
                       srcstride,
                       rclSrc.left,
                       rclSrc.top,
                       mskptr,
                       mskstride,
                       rclMask.left,
                       rclMask.top);

#ifdef CHECKBLT
// Save my BLT results
void* buffmydst = malloc(size);
if(SavedParms.pDst->Stride() < 0)
  memcpy(buffmydst, (void*)((long)SavedParms.pDst->Buffer() - ((SavedParms.pDst->Height() - 1) * SavedParms.pDst->Stride())), size);
else
  memcpy(buffmydst, SavedParms.pDst->Buffer(), size);

// Check for an error
  unsigned int mybltcrc = wombat2d((unsigned char*)SavedParms.pDst->Buffer(),
                                   SavedParms.pDst->Stride(),
                                   SavedParms.pDst->Width(),
                                   SavedParms.pDst->Height());
// Restore original destination
if(SavedParms.pDst->Stride() < 0)
  memcpy((void*)((long)SavedParms.pDst->Buffer() - ((SavedParms.pDst->Height() - 1) * SavedParms.pDst->Stride())), buffolddst, size);
else
  memcpy(SavedParms.pDst->Buffer(), buffolddst, size);

  GPE* pGPE = GetGPE();
  pGPE->EmulatedBlt(pParms);
  if(mybltcrc != wombat2d((unsigned char*)SavedParms.pDst->Buffer(),
                          SavedParms.pDst->Stride(),
                          SavedParms.pDst->Width(),
                          SavedParms.pDst->Height()))
  {
    RETAILMSG(1, (TEXT("****** BAD BLT ******\n")));
    DumpParms(&SavedParms);
    DebugBreak();
  }
  free(buffmydst);
  free(buffolddst);
#endif

  return(S_OK);
}

SCODE OMAPDDGPE::EmulatedBlockCopyBGR24toRGB16(GPEBltParms* pParms)
{
CALLOUT(L"EmulatedBlockCopyBGR24toRGB16()");

  void* dstptr = pParms->pDst->Buffer();
  long dststride = pParms->pDst->Stride();
  void* srcptr = pParms->pSrc->Buffer();
  long srcstride = pParms->pSrc->Stride();
  RECTL rclDst = *(pParms->prclDst);
  RECTL rclSrc = *(pParms->prclSrc);

  WaitForNotBusy();
#if 1
  BlockCopyBGR24toRGB16(dstptr,
                        dststride,
                        rclDst.left,
                        rclDst.top,
                        rclDst.right - rclDst.left,
                        rclDst.bottom - rclDst.top,
                        srcptr,
                        srcstride,
                        rclSrc.left,
                        rclSrc.top);
#else
  BlockCopyRGB24toRGB16_v1(dstptr,
                           dststride,
                           rclDst.left,
                           rclDst.top,
                           rclDst.right - rclDst.left,
                           rclDst.bottom - rclDst.top,
                           srcptr,
                           srcstride,
                           rclSrc.left,
                           rclSrc.top);
#endif

#ifdef CHECKBLT
  if(!CheckBlt(pParms))
  {
    OutputDebugString(TEXT("** BAD BLT **: ") TEXT(__FUNCTION__) TEXT("()"));
    DebugBreak();
  }
#endif

  return(S_OK);
}

SCODE OMAPDDGPE::EmulatedBlockCopyBGRx32toRGB16(GPEBltParms* pParms)
{
CALLOUT(L"EmulatedBlockCopyBGRx32toRGB16()");

  void* dstptr = pParms->pDst->Buffer();
  long dststride = pParms->pDst->Stride();
  void* srcptr = pParms->pSrc->Buffer();
  long srcstride = pParms->pSrc->Stride();
  RECTL rclDst = *(pParms->prclDst);
  RECTL rclSrc = *(pParms->prclSrc);

  WaitForNotBusy();
  BlockCopyBGRx32toRGB16(dstptr,
                         dststride,
                         rclDst.left,
                         rclDst.top,
                         rclDst.right - rclDst.left,
                         rclDst.bottom - rclDst.top,
                         srcptr,
                         srcstride,
                         rclSrc.left,
                         rclSrc.top);

#ifdef CHECKBLT
  if(!CheckBlt(pParms))
  {
    OutputDebugString(TEXT("** BAD BLT **: ") TEXT(__FUNCTION__) TEXT("()"));
    DebugBreak();
  }
#endif

  return(S_OK);
}

SCODE OMAPDDGPE::EmulatedPerPixelAlphaBlendBGRA32toRGB16(GPEBltParms* pParms)
{
CALLOUT(L"EmulatedPerPixelAlphaBlendBGRA32toRGB16()");

#if defined(CHECKBLT) && !defined(NOCHECKALPHA)
//DebugBreak();
GPEBltParms SavedParms = *pParms;
ULONG size = abs(SavedParms.pDst->Stride()) * SavedParms.pDst->Height();
void* buffolddst = malloc(size);
// Save original destination
if(SavedParms.pDst->Stride() < 0)
  memcpy(buffolddst, (void*)((long)SavedParms.pDst->Buffer() - ((SavedParms.pDst->Height() - 1) * SavedParms.pDst->Stride())), size);
else
  memcpy(buffolddst, SavedParms.pDst->Buffer(), size);
#endif
  void* dstptr = pParms->pDst->Buffer();
  long dststride = pParms->pDst->Stride();
  void* srcptr = pParms->pSrc->Buffer();
  long srcstride = pParms->pSrc->Stride();
  RECTL rclDst = *(pParms->prclDst);
  RECTL rclSrc = *(pParms->prclSrc);

  WaitForNotBusy();
  AlphaBlendpBGRA32toRGB16(dstptr,
                           dststride,
                           rclDst.left,
                           rclDst.top,
                           rclDst.right - rclDst.left,
                           rclDst.bottom - rclDst.top,
                           srcptr,
                           srcstride,
                           rclSrc.left,
                           rclSrc.top);

#if defined(CHECKBLT) && !defined(NOCHECKALPHA)
// Save my BLT results
void* buffmydst = malloc(size);
if(SavedParms.pDst->Stride() < 0)
  memcpy(buffmydst, (void*)((long)SavedParms.pDst->Buffer() - ((SavedParms.pDst->Height() - 1) * SavedParms.pDst->Stride())), size);
else
  memcpy(buffmydst, SavedParms.pDst->Buffer(), size);

// Check for an error
  unsigned int mybltcrc = wombat2d((unsigned char*)SavedParms.pDst->Buffer(),
                                   SavedParms.pDst->Stride(),
                                   SavedParms.pDst->Width(),
                                   SavedParms.pDst->Height());
// Restore original destination
if(SavedParms.pDst->Stride() < 0)
  memcpy((void*)((long)SavedParms.pDst->Buffer() - ((SavedParms.pDst->Height() - 1) * SavedParms.pDst->Stride())), buffolddst, size);
else
  memcpy(SavedParms.pDst->Buffer(), buffolddst, size);

  GPE* pGPE = GetGPE();
  pGPE->EmulatedBlt(pParms);
  if(mybltcrc != wombat2d((unsigned char*)SavedParms.pDst->Buffer(),
                          SavedParms.pDst->Stride(),
                          SavedParms.pDst->Width(),
                          SavedParms.pDst->Height()))
  {
    RETAILMSG(1, (TEXT("****** BAD BLT ******\n")));
    DumpParms(&SavedParms);

    // Print out specific errors
    long sx, sy, dx, dy;
    for(sy = SavedParms.prclSrc->top, dy = SavedParms.prclDst->top;
        sy < SavedParms.prclSrc->bottom;
        sy++, dy++)
    {
      for(sx = SavedParms.prclSrc->left, dx = SavedParms.prclDst->left;
          sx < SavedParms.prclSrc->right;
          sx++, dx++)
      {
        WORD* mydst  = (WORD*)((long)buffmydst + (dx * sizeof(WORD)) + (dy * abs(SavedParms.pDst->Stride())));
        WORD* refdst = (WORD*)((long)SavedParms.pDst->Buffer()  + (dx * sizeof(WORD)) + (dy * SavedParms.pDst->Stride()));
        if(*mydst != *refdst)
        {
          RETAILMSG(1, (TEXT("Bad pixel: (%d, %d) -> (%d, %d), Src: 0x%08X, Dst Before: 0x%04X, My Dst After: 0x%04X, Ref Dst AFter: 0x%04X\n"),
            sx, sy,
            dx, dy,
            *((ULONG*)((long)SavedParms.pSrc->Buffer() + (sx * sizeof(ULONG)) + (sy * SavedParms.pSrc->Stride()))),
            *((WORD*)((long)buffolddst + (dx * sizeof(WORD)) + (dy * abs(SavedParms.pDst->Stride())))),
            *mydst,
            *refdst));
        }
      }
    }
    DebugBreak();
  }
  free(buffmydst);
  free(buffolddst);
#endif

  return(S_OK);
}

SCODE OMAPDDGPE::EmulatedPerPixelAlphaBlendBGRA32toPreBGRA32(GPEBltParms* pParms)
{
  void* dstptr = pParms->pDst->Buffer();
  long dststride = pParms->pDst->Stride();
  void* srcptr = pParms->pSrc->Buffer();
  long srcstride = pParms->pSrc->Stride();
  RECTL rclDst = *(pParms->prclDst);
  RECTL rclSrc = *(pParms->prclSrc);

  WaitForNotBusy();
  AlphaBlendBGRA32topBGRA32(dstptr,
                           dststride,
                           rclDst.left,
                           rclDst.top,
                           rclDst.right - rclDst.left,
                           rclDst.bottom - rclDst.top,
                           srcptr,
                           srcstride,
                           rclSrc.left,
                           rclSrc.top);

  return(S_OK);
}

SCODE OMAPDDGPE::EmulatedPerPixelAlphaBlendPreBGRA32toPreBGRA32(GPEBltParms* pParms)
{
  void* dstptr = pParms->pDst->Buffer();
  long dststride = pParms->pDst->Stride();
  void* srcptr = pParms->pSrc->Buffer();
  long srcstride = pParms->pSrc->Stride();
  RECTL rclDst = *(pParms->prclDst);
  RECTL rclSrc = *(pParms->prclSrc);

  WaitForNotBusy();
  AlphaBlendpBGRA32topBGRA32(dstptr,
                           dststride,
                           rclDst.left,
                           rclDst.top,
                           rclDst.right - rclDst.left,
                           rclDst.bottom - rclDst.top,
                           srcptr,
                           srcstride,
                           rclSrc.left,
                           rclSrc.top);

  return(S_OK);
}

SCODE OMAPDDGPE::EmulatedPerPixelUnpremultiplyBGRA32(GPEBltParms* pParms)
{
  void* dstptr = pParms->pDst->Buffer();
  long dststride = pParms->pDst->Stride();
  void* srcptr = pParms->pSrc->Buffer();
  long srcstride = pParms->pSrc->Stride();
  RECTL rclDst = *(pParms->prclDst);
  RECTL rclSrc = *(pParms->prclSrc);

  WaitForNotBusy();
  UnpremultiplyBGRA32(dstptr,
                   dststride,
                   rclDst.left,
                   rclDst.top,
                   rclDst.right - rclDst.left,
                   rclDst.bottom - rclDst.top,
                   srcptr,
                   srcstride,
                   rclSrc.left,
                   rclSrc.top);

  return(S_OK);
}

SCODE OMAPDDGPE::EmulatedBlockCopyLUT8to24(GPEBltParms* pParms)
{
CALLOUT(L"EmulatedBlockCopyLUT8to24()");

  void* dstptr = pParms->pDst->Buffer();
  long dststride = pParms->pDst->Stride();
  void* srcptr = pParms->pSrc->Buffer();
  long srcstride = pParms->pSrc->Stride();
  RECTL rclDst = *(pParms->prclDst);
  RECTL rclSrc = *(pParms->prclSrc);

  WaitForNotBusy();
  BlockCopyLUT8to24(dstptr,
                    dststride,
                    rclDst.left,
                    rclDst.top,
                    rclDst.right - rclDst.left,
                    rclDst.bottom - rclDst.top,
                    srcptr,
                    srcstride,
                    rclSrc.left,
                    rclSrc.top,
                    pParms->pLookup);

#ifdef CHECKBLT
  if(!CheckBlt(pParms))
  {
    OutputDebugString(TEXT("** BAD BLT **: ") TEXT(__FUNCTION__) TEXT("()"));
    DebugBreak();
  }
#endif

  return(S_OK);
}

SCODE OMAPDDGPE::EmulatedBlockCopyRGB16toBGR24(GPEBltParms* pParms)
{
CALLOUT(L"EmulatedBlockCopyRGB16toBGR24()");

  void* dstptr = pParms->pDst->Buffer();
  long dststride = pParms->pDst->Stride();
  void* srcptr = pParms->pSrc->Buffer();
  long srcstride = pParms->pSrc->Stride();
  RECTL rclDst = *(pParms->prclDst);
  RECTL rclSrc = *(pParms->prclSrc);

  WaitForNotBusy();
  BlockCopyRGB16toBGR24(dstptr,
                        dststride,
                        rclDst.left,
                        rclDst.top,
                        rclDst.right - rclDst.left,
                        rclDst.bottom - rclDst.top,
                        srcptr,
                        srcstride,
                        rclSrc.left,
                        rclSrc.top);

#ifdef CHECKBLT
  if(!CheckBlt(pParms))
  {
    OutputDebugString(TEXT("** BAD BLT **: ") TEXT(__FUNCTION__) TEXT("()"));
    DebugBreak();
  }
#endif

  return(S_OK);
}

SCODE OMAPDDGPE::EmulatedBlockCopy24to24(GPEBltParms* pParms)
{
  // Can't handle right to left yet
  if(pParms->xPositive != 1)
    return(GPE::EmulatedBlt(pParms));

  // Can't handle lookup tables or conversion routines
  if(pParms->pLookup ||
     pParms->pConvert)
    return(GPE::EmulatedBlt(pParms));

CALLOUT(L"EmulatedBlockCopy24to24()");

  void* dstptr = pParms->pDst->Buffer();
  long dststride = pParms->pDst->Stride();
  void* srcptr = pParms->pSrc->Buffer();
  long srcstride = pParms->pSrc->Stride();
  RECTL rclDst = *(pParms->prclDst);
  RECTL rclSrc = *(pParms->prclSrc);

  if(pParms->yPositive != 1)
    AdjustForBottomUp(dstptr, dststride, pParms->pDst->Height(), rclDst,
                      srcptr, srcstride, pParms->pSrc->Height(), rclSrc);

  WaitForNotBusy();
  BlockCopy24(dstptr,
              dststride,
              rclDst.left,
              rclDst.top,
              rclDst.right - rclDst.left,
              rclDst.bottom - rclDst.top,
              srcptr,
              srcstride,
              rclSrc.left,
              rclSrc.top);

#ifdef CHECKBLT
  if(!CheckBlt(pParms))
  {
    OutputDebugString(TEXT("** BAD BLT **: ") TEXT(__FUNCTION__) TEXT("()"));
    DebugBreak();
  }
#endif

  return(S_OK);
}

SCODE OMAPDDGPE::EmulatedBlockCopyXYZx32toXYZ24(GPEBltParms* pParms)
{
CALLOUT(L"EmulatedBlockCopyXYZx32toXYZ24()");

  void* dstptr = pParms->pDst->Buffer();
  long dststride = pParms->pDst->Stride();
  void* srcptr = pParms->pSrc->Buffer();
  long srcstride = pParms->pSrc->Stride();
  RECTL rclDst = *(pParms->prclDst);
  RECTL rclSrc = *(pParms->prclSrc);

  WaitForNotBusy();
  BlockCopyXYZx32toXYZ24(dstptr,
                         dststride,
                         rclDst.left,
                         rclDst.top,
                         rclDst.right - rclDst.left,
                         rclDst.bottom - rclDst.top,
                         srcptr,
                         srcstride,
                         rclSrc.left,
                         rclSrc.top);

#ifdef CHECKBLT
  if(!CheckBlt(pParms))
  {
    OutputDebugString(TEXT("** BAD BLT **: ") TEXT(__FUNCTION__) TEXT("()"));
    DebugBreak();
  }
#endif

  return(S_OK);
}

SCODE OMAPDDGPE::EmulatedBlockCopyLUT8to32(GPEBltParms* pParms)
{
CALLOUT(L"EmulatedBlockCopyLUT8to32()");

  void* dstptr = pParms->pDst->Buffer();
  long dststride = pParms->pDst->Stride();
  void* srcptr = pParms->pSrc->Buffer();
  long srcstride = pParms->pSrc->Stride();
  RECTL rclDst = *(pParms->prclDst);
  RECTL rclSrc = *(pParms->prclSrc);

  WaitForNotBusy();
  BlockCopyLUT8to32(dstptr,
                    dststride,
                    rclDst.left,
                    rclDst.top,
                    rclDst.right - rclDst.left,
                    rclDst.bottom - rclDst.top,
                    srcptr,
                    srcstride,
                    rclSrc.left,
                    rclSrc.top,
                    pParms->pLookup);

#ifdef CHECKBLT
  if(!CheckBlt(pParms))
  {
    OutputDebugString(TEXT("** BAD BLT **: ") TEXT(__FUNCTION__) TEXT("()"));
    DebugBreak();
  }
#endif

  return(S_OK);
}

SCODE OMAPDDGPE::EmulatedBlockCopyRGB16toBGRx32(GPEBltParms* pParms)
{
CALLOUT(L"EmulatedBlockCopyRGB16toBGRx32()");

#if defined(CHECKBLT) && !defined(NOCHECK565)
//DebugBreak();
GPEBltParms SavedParms = *pParms;
ULONG size = abs(SavedParms.pDst->Stride()) * SavedParms.pDst->Height();
#endif
  void* dstptr = pParms->pDst->Buffer();
  long dststride = pParms->pDst->Stride();
  void* srcptr = pParms->pSrc->Buffer();
  long srcstride = pParms->pSrc->Stride();
  RECTL rclDst = *(pParms->prclDst);
  RECTL rclSrc = *(pParms->prclSrc);

  WaitForNotBusy();
  BlockCopyRGB16toBGRx32(dstptr,
                         dststride,
                         rclDst.left,
                         rclDst.top,
                         rclDst.right - rclDst.left,
                         rclDst.bottom - rclDst.top,
                         srcptr,
                         srcstride,
                         rclSrc.left,
                         rclSrc.top);

#if defined(CHECKBLT) && !defined(NOCHECK565)
// Save my BLT results
void* buffmydst = malloc(size);
if(SavedParms.pDst->Stride() < 0)
  memcpy(buffmydst, (void*)((long)SavedParms.pDst->Buffer() - ((SavedParms.pDst->Height() - 1) * SavedParms.pDst->Stride())), size);
else
  memcpy(buffmydst, SavedParms.pDst->Buffer(), size);

// Check for an error
  unsigned int mybltcrc = wombat2d((unsigned char*)SavedParms.pDst->Buffer(),
                                   SavedParms.pDst->Stride(),
                                   SavedParms.pDst->Width(),
                                   SavedParms.pDst->Height());
  GPE* pGPE = GetGPE();
  pGPE->EmulatedBlt(pParms);
  if(mybltcrc != wombat2d((unsigned char*)SavedParms.pDst->Buffer(),
                          SavedParms.pDst->Stride(),
                          SavedParms.pDst->Width(),
                          SavedParms.pDst->Height()))
  {
    RETAILMSG(1, (TEXT("****** BAD BLT ******\n")));
    DumpParms(&SavedParms);

    // Print out specific errors
    long sx, sy, dx, dy;
    for(sy = SavedParms.prclSrc->top, dy = SavedParms.prclDst->top;
        sy < SavedParms.prclSrc->bottom;
        sy++, dy++)
    {
      for(sx = SavedParms.prclSrc->left, dx = SavedParms.prclDst->left;
          sx < SavedParms.prclSrc->right;
          sx++, dx++)
      {
        ULONG* mydst  = (ULONG*)((long)buffmydst + (dx * sizeof(ULONG)) + (dy * abs(SavedParms.pDst->Stride())));
        ULONG* refdst = (ULONG*)((long)SavedParms.pDst->Buffer()  + (dx * sizeof(ULONG)) + (dy * SavedParms.pDst->Stride()));
        if(*mydst != *refdst)
        {
          RETAILMSG(1, (TEXT("Bad pixel: (%d, %d) -> (%d, %d), Src: 0x%08X, My Dst After: 0x%04X, Ref Dst AFter: 0x%04X\n"),
            sx, sy,
            dx, dy,
            *((WORD*)((long)SavedParms.pSrc->Buffer() + (sx * sizeof(WORD)) + (sy * SavedParms.pSrc->Stride()))),
            *mydst,
            *refdst));
        }
      }
    }
    DebugBreak();
  }
  free(buffmydst);
#endif

  return(S_OK);
}

SCODE OMAPDDGPE::EmulatedBlockCopyXYZ24toXYZx32(GPEBltParms* pParms)
{
CALLOUT(L"EmulatedBlockCopyXYZ24toXYZx32()");

  void* dstptr = pParms->pDst->Buffer();
  long dststride = pParms->pDst->Stride();
  void* srcptr = pParms->pSrc->Buffer();
  long srcstride = pParms->pSrc->Stride();
  RECTL rclDst = *(pParms->prclDst);
  RECTL rclSrc = *(pParms->prclSrc);

  WaitForNotBusy();
  BlockCopyXYZ24toXYZx32(dstptr,
                         dststride,
                         rclDst.left,
                         rclDst.top,
                         rclDst.right - rclDst.left,
                         rclDst.bottom - rclDst.top,
                         srcptr,
                         srcstride,
                         rclSrc.left,
                         rclSrc.top);

#ifdef CHECKBLT
  if(!CheckBlt(pParms))
  {
    OutputDebugString(TEXT("** BAD BLT **: ") TEXT(__FUNCTION__) TEXT("()"));
    DebugBreak();
  }
#endif

  return(S_OK);
}

SCODE OMAPDDGPE::EmulatedBlockCopy32to32(GPEBltParms* pParms)
{
  // Can't handle right to left yet
  if(pParms->xPositive != 1)
    return(GPE::EmulatedBlt(pParms));

  // Can't handle lookup tables or conversion routines
  if(pParms->pLookup )
    return(GPE::EmulatedBlt(pParms));

CALLOUT(L"EmulatedBlockCopy32to32()");

  void* dstptr = pParms->pDst->Buffer();
  long dststride = pParms->pDst->Stride();
  void* srcptr = pParms->pSrc->Buffer();
  long srcstride = pParms->pSrc->Stride();
  RECTL rclDst = *(pParms->prclDst);
  RECTL rclSrc = *(pParms->prclSrc);

  if(pParms->yPositive != 1)
    AdjustForBottomUp(dstptr, dststride, pParms->pDst->Height(), rclDst,
                      srcptr, srcstride, pParms->pSrc->Height(), rclSrc);

  WaitForNotBusy();
  BlockCopy32(dstptr,
              dststride,
              rclDst.left,
              rclDst.top,
              rclDst.right - rclDst.left,
              rclDst.bottom - rclDst.top,
              srcptr,
              srcstride,
              rclSrc.left,
              rclSrc.top);

#ifdef CHECKBLT
  if(!CheckBlt(pParms))
  {
    OutputDebugString(TEXT("** BAD BLT **: ") TEXT(__FUNCTION__) TEXT("()"));
    DebugBreak();
  }
#endif

  return(S_OK);
}

SCODE OMAPDDGPE::EmulatedMaskCopy32to32withA1(GPEBltParms* pParms)
{
CALLOUT(L"EmulatedMaskCopy32to32withA1()");

#ifdef CHECKBLT
//DebugBreak();
GPEBltParms SavedParms = *pParms;
ULONG size = abs(SavedParms.pDst->Stride()) * SavedParms.pDst->Height();
void* buffolddst = malloc(size);
// Save original destination
if(SavedParms.pDst->Stride() < 0)
  memcpy(buffolddst, (void*)((long)SavedParms.pDst->Buffer() - ((SavedParms.pDst->Height() - 1) * SavedParms.pDst->Stride())), size);
else
  memcpy(buffolddst, SavedParms.pDst->Buffer(), size);
#endif
  void* dstptr = pParms->pDst->Buffer();
  long dststride = pParms->pDst->Stride();
  void const* srcptr = pParms->pSrc->Buffer();
  long srcstride = pParms->pSrc->Stride();
  void const* mskptr = pParms->pMask->Buffer();
  long mskstride = pParms->pMask->Stride();
  RECTL rclDst = *(pParms->prclDst);
  RECTL rclSrc = *(pParms->prclSrc);
  RECTL rclMask = *(pParms->prclMask);

  WaitForNotBusy();
  MaskCopy32to32withA1(dstptr,
                       dststride,
                       rclDst.left,
                       rclDst.top,
                       rclDst.right - rclDst.left,
                       rclDst.bottom - rclDst.top,
                       srcptr,
                       srcstride,
                       rclSrc.left,
                       rclSrc.top,
                       mskptr,
                       mskstride,
                       rclMask.left,
                       rclMask.top);

#ifdef CHECKBLT
// Save my BLT results
void* buffmydst = malloc(size);
if(SavedParms.pDst->Stride() < 0)
  memcpy(buffmydst, (void*)((long)SavedParms.pDst->Buffer() - ((SavedParms.pDst->Height() - 1) * SavedParms.pDst->Stride())), size);
else
  memcpy(buffmydst, SavedParms.pDst->Buffer(), size);

// Check for an error
  unsigned int mybltcrc = wombat2d((unsigned char*)SavedParms.pDst->Buffer(),
                                   SavedParms.pDst->Stride(),
                                   SavedParms.pDst->Width(),
                                   SavedParms.pDst->Height());
// Restore original destination
if(SavedParms.pDst->Stride() < 0)
  memcpy((void*)((long)SavedParms.pDst->Buffer() - ((SavedParms.pDst->Height() - 1) * SavedParms.pDst->Stride())), buffolddst, size);
else
  memcpy(SavedParms.pDst->Buffer(), buffolddst, size);

  GPE* pGPE = GetGPE();
  pGPE->EmulatedBlt(pParms);
  if(mybltcrc != wombat2d((unsigned char*)SavedParms.pDst->Buffer(),
                          SavedParms.pDst->Stride(),
                          SavedParms.pDst->Width(),
                          SavedParms.pDst->Height()))
  {
    RETAILMSG(1, (TEXT("****** BAD BLT ******\n")));
    DumpParms(&SavedParms);
    DebugBreak();
  }
  free(buffmydst);
  free(buffolddst);
#endif

  return(S_OK);
}

