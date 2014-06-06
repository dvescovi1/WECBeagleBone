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


//
//  Includes
//
#include "TIBLT.h"


extern void BlockFill8(void*         dstptr,
                       long          dststride,
                       unsigned long dstleft,
                       unsigned long dsttop,
                       unsigned long width,
                       unsigned long height,
                       unsigned char color);

// NOTE: No overlap detection
extern void BlockCopy8(void*         dstptr,
                       long          dststride,
                       unsigned long dstleft,
                       unsigned long dsttop,
                       unsigned long width,
                       unsigned long height,
                       void const*   srcptr,
                       long          srcstride,
                       unsigned long srcleft,
                       unsigned long srctop);
extern void Rotate_8_90(void* dstptr,
                        long  dststride,
                        int   dstleft,
                        int   dsttop,
                        unsigned int dstwidth,
                        unsigned int dstheight,
                        void* srcptr,
                        long  srcstride,
                        int   srcleft,
                        int   srctop);

extern void BlockFill16(void*          dstptr,
                        long           dststride,
                        unsigned long  dstleft,
                        unsigned long  dsttop,
                        unsigned long  width,
                        unsigned long  height,
                        unsigned short color);
extern void BlockCopyLUT8to16(void*                dstptr,
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
// NOTE: No overlap detection
extern void BlockCopy16(void*         dstptr,
                        long          dststride,
                        unsigned long dstleft,
                        unsigned long dsttop,
                        unsigned long width,
                        unsigned long height,
                        void const*   srcptr,
                        long          srcstride,
                        unsigned long srcleft,
                        unsigned long srctop);
extern void Rotate_16_90(void* dstptr,
                         long  dststride,
                         int   dstleft,
                         int   dsttop,
                         unsigned int dstwidth,
                         unsigned int dstheight,
                         void* srcptr,
                         long  srcstride,
                         int   srcleft,
                         int   srctop);
extern void Rotate_16_180(void* dstptr,
                          long  dststride,
                          int   dstleft,
                          int   dsttop,
                          unsigned int dstwidth,
                          unsigned int dstheight,
                          void* srcptr,
                          long  srcstride,
                          int   srcleft,
                          int   srctop);
extern void Rotate_16_270(void* dstptr,
                          long  dststride,
                          int   dstleft,
                          int   dsttop,
                          unsigned int dstwidth,
                          unsigned int dstheight,
                          void* srcptr,
                          long  srcstride,
                          int   srcleft,
                          int   srctop);
extern void Rotate_UY_90(void* dstptr,
                         long  dststride,
                         int   dstleft,
                         int   dsttop,
                         unsigned int dstwidth,
                         unsigned int dstheight,
                         void* srcptr,
                         long  srcstride,
                         int   srcleft,
                         int   srctop);
extern void Rotate_UY_180(void* dstptr,
                          long  dststride,
                          int   dstleft,
                          int   dsttop,
                          unsigned int dstwidth,
                          unsigned int dstheight,
                          void* srcptr,
                          long  srcstride,
                          int   srcleft,
                          int   srctop);
extern void Rotate_UY_270(void* dstptr,
                          long  dststride,
                          int   dstleft,
                          int   dsttop,
                          unsigned int dstwidth,
                          unsigned int dstheight,
                          void* srcptr,
                          long  srcstride,
                          int   srcleft,
                          int   srctop);
extern void BlockCopyBGR24toRGB16(void*         dstptr,
                                  long          dststride,
                                  unsigned long dstleft,
                                  unsigned long dsttop,
                                  unsigned long width,
                                  unsigned long height,
                                  void const*   srcptr,
                                  long          srcstride,
                                  unsigned long srcleft,
                                  unsigned long srctop);
extern void BlockCopyBGRx32toRGB16(void*         dstptr,
                                   long          dststride,
                                   unsigned long dstleft,
                                   unsigned long dsttop,
                                   unsigned long width,
                                   unsigned long height,
                                   void const*   srcptr,
                                   long          srcstride,
                                   unsigned long srcleft,
                                   unsigned long srctop);
extern void BlockCopyUYVYtoRGB16(void*         dstptr,
                                 long          dststride,
                                 unsigned long dstleft,
                                 unsigned long dsttop,
                                 unsigned long width,
                                 unsigned long height,
                                 void const*   srcptr,
                                 long          srcstride,
                                 unsigned long srcleft,
                                 unsigned long srctop);
extern void BlockCopyIYUVtoRGB16(void*         dstptr,
                                 long          dststride,
                                 unsigned long dstleft,
                                 unsigned long dsttop,
                                 unsigned long width,
                                 unsigned long height,
                                 void const*   srcptr,
                                 long          srcstride,
                                 unsigned long srcleft,
                                 unsigned long srctop,
                                 unsigned long srcsurfheight);
extern void AlphaBlendpBGRA32toRGB16(void*         dstptr,
                                     long          dststride,
                                     unsigned long dstleft,
                                     unsigned long dsttop,
                                     unsigned long width,
                                     unsigned long height,
                                     void const*   srcptr,
                                     long          srcstride,
                                     unsigned long srcleft,
                                     unsigned long srctop);
extern void AlphaBlendRGB16toRGB16withConst(void*         dstptr,
                                            long          dststride,
                                            unsigned long dstleft,
                                            unsigned long dsttop,
                                            unsigned long width,
                                            unsigned long height,
                                            void const*   srcptr,
                                            long          srcstride,
                                            unsigned long srcleft,
                                            unsigned long srctop,
                                            unsigned char alpha);
extern void BlockFill24(void*         dstptr,
                        long          dststride,
                        unsigned long dstleft,
                        unsigned long dsttop,
                        unsigned long width,
                        unsigned long height,
                        unsigned long color);
extern void BlockCopyLUT8to24(void*                dstptr,
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
extern void BlockCopyRGB16toBGR24(void*         dstptr,
                                  long          dststride,
                                  unsigned long dstleft,
                                  unsigned long dsttop,
                                  unsigned long width,
                                  unsigned long height,
                                  void const*   srcptr,
                                  long          srcstride,
                                  unsigned long srcleft,
                                  unsigned long srctop);
extern void BlockCopy24(void*         dstptr,
                        long          dststride,
                        unsigned long dstleft,
                        unsigned long dsttop,
                        unsigned long width,
                        unsigned long height,
                        void const*   srcptr,
                        long          srcstride,
                        unsigned long srcleft,
                        unsigned long srctop);
extern void BlockCopyXYZx32toXYZ24(void*         dstptr,
                                   long          dststride,
                                   unsigned long dstleft,
                                   unsigned long dsttop,
                                   unsigned long width,
                                   unsigned long height,
                                   void const*   srcptr,
                                   long          srcstride,
                                   unsigned long srcleft,
                                   unsigned long srctop);
extern void BlockCopyUYVYtoBGR24(void*         dstptr,
                                 long          dststride,
                                 unsigned long dstleft,
                                 unsigned long dsttop,
                                 unsigned long width,
                                 unsigned long height,
                                 void const*   srcptr,
                                 long          srcstride,
                                 unsigned long srcleft,
                                 unsigned long srctop);
extern void BlockFill32(void*         dstptr,
                        long          dststride,
                        unsigned long dstleft,
                        unsigned long dsttop,
                        unsigned long width,
                        unsigned long height,
                        unsigned long color);
extern void BlockCopyLUT8to32(void*                dstptr,
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
extern void BlockCopyRGB16toBGRx32(void*         dstptr,
                                   long          dststride,
                                   unsigned long dstleft,
                                   unsigned long dsttop,
                                   unsigned long width,
                                   unsigned long height,
                                   void const*   srcptr,
                                   long          srcstride,
                                   unsigned long srcleft,
                                   unsigned long srctop);
extern void BlockCopyXYZ24toXYZx32(void*         dstptr,
                                   long          dststride,
                                   unsigned long dstleft,
                                   unsigned long dsttop,
                                   unsigned long width,
                                   unsigned long height,
                                   void const*   srcptr,
                                   long          srcstride,
                                   unsigned long srcleft,
                                   unsigned long srctop);
extern void BlockCopy32(void*         dstptr,
                        long          dststride,
                        unsigned long dstleft,
                        unsigned long dsttop,
                        unsigned long width,
                        unsigned long height,
                        void const*   srcptr,
                        long          srcstride,
                        unsigned long srcleft,
                        unsigned long srctop);
extern void AlphaBlendpBGRA32topBGRA32(void*         dstptr,
                                       long          dststride,
                                       unsigned long dstleft,
                                       unsigned long dsttop,
                                       unsigned long width,
                                       unsigned long height,
                                       void const*   srcptr,
                                       long          srcstride,
                                       unsigned long srcleft,
                                       unsigned long srctop);
extern void AlphaBlendBGRA32topBGRA32(void*         dstptr,
                                      long          dststride,
                                      unsigned long dstleft,
                                      unsigned long dsttop,
                                      unsigned long width,
                                      unsigned long height,
                                      void const*   srcptr,
                                      long          srcstride,
                                      unsigned long srcleft,
                                      unsigned long srctop);
extern void BlockCopyUYVYtoBGRx32(void*         dstptr,
                                  long          dststride,
                                  unsigned long dstleft,
                                  unsigned long dsttop,
                                  unsigned long width,
                                  unsigned long height,
                                  void const*   srcptr,
                                  long          srcstride,
                                  unsigned long srcleft,
                                  unsigned long srctop);
extern void MaskCopy32to32withA1(void*         dstptr,
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
extern void BlockCopyIYUVtoUYVY(void*         dstptr,
                                long          dststride,
                                unsigned long dstleft,
                                unsigned long dsttop,
                                unsigned long width,
                                unsigned long height,
                                void const*   srcptr,
                                long          srcstride,
                                unsigned long srcleft,
                                unsigned long srctop,
                                unsigned long srcsurfheight);
extern void BlockCopyBGR24toUYVY(void*         dstptr,
                                 long          dststride,
                                 unsigned long dstleft,
                                 unsigned long dsttop,
                                 unsigned long width,
                                 unsigned long height,
                                 void const*   srcptr,
                                 long          srcstride,
                                 unsigned long srcleft,
                                 unsigned long srctop);

int RectsIntersect(TIBLTRECT* prect1,
                   TIBLTRECT* prect2)
{
  int rect1right = prect1->left + prect1->width;
  int rect1bottom = prect1->top + prect1->height;
  int rect2right = prect2->left + prect2->width;
  int rect2bottom = prect2->top + prect2->height;

  return(!((prect1->left >= rect2right) ||
            (prect2->left >= rect1right) ||
            (prect1->top >= rect1bottom) ||
            (prect2->top >= rect2bottom)));
}

int NormalizeAngle(int angle)
{
  angle %= 360;
  if(angle < 0)
    angle = angle + 360;
  return(angle);
}

TIBLTERROR TIBLTFN_toLUT8(TIBLTPARAMS* pParms)
{
  TIBLTERROR error = TIBLT_ERR_NONE;

  // Any flag other than ROP not supported for LUT8
  if(pParms->flags & ~TIBLT_FLAG_ROP)
  {
    error = TIBLT_ERR_UNSUPPORTED_FLAGS;
    goto Error;
  }

  // ROP flag must be set
  if(!(pParms->flags & TIBLT_FLAG_ROP))
  {
    error = TIBLT_ERR_UNSUPPORTED_FLAGS;
    goto Error;
  }

  // Which ROP?
  switch(pParms->ROP)
  {
    case 0xF0F0:  // PATCOPY
      // Only fill with same format as destination supported
      if(pParms->pbrush->format != pParms->pdst->format)
      {
        error = TIBLT_ERR_UNSUPPORTED_CONVERSION;
        goto Error;
      }

      // only solid fill using 1x1 brush supported
      if((pParms->brushrect.width != 1) ||
         (pParms->brushrect.height != 1))
      {
        error = TIBLT_ERR_UNSUPPORTED_BRUSH_RECT;
        goto Error;
      }

      // Do the fill
      BlockFill8(pParms->pdst->virtptr,
                 pParms->pdst->virtstride,
                 pParms->dstrect.left,
                 pParms->dstrect.top,
                 pParms->dstrect.width,
                 pParms->dstrect.height,
                 *((unsigned char*)pParms->pbrush->virtptr +
                   pParms->brushrect.left +
                   (pParms->brushrect.top * pParms->pbrush->virtstride)));
      break;
      
    case 0xCCCC:  // SRCCOPY
      // No overlap support
      if(pParms->pdst == pParms->psrc)
      {
        if(RectsIntersect(&(pParms->dstrect), &(pParms->srcrect)))
        {
          error = TIBLT_ERR_UNSUPPORTED_SRC_RECT;
          goto Error;
        }
      }

      {
        int rotationangle = NormalizeAngle(pParms->pdst->orientation - pParms->psrc->orientation);
        
        // No scaling supported
        switch(rotationangle)
        {
          case 0:
          case 180:
            if((pParms->dstrect.width != pParms->srcrect.width) ||
               (pParms->dstrect.height != pParms->srcrect.height))
            {
              error = TIBLT_ERR_UNSUPPORTED_SCALE_FACTOR;
              goto Error;
            }
            break;

          case 90:
          case 270:
            if((pParms->dstrect.width != pParms->srcrect.height) ||
               (pParms->dstrect.height != pParms->srcrect.width))
            {
              error = TIBLT_ERR_UNSUPPORTED_SCALE_FACTOR;
              goto Error;
            }
            break;

          default:
            error = TIBLT_ERR_UNSUPPORTED_ROTATION;
            goto Error;
        }

        // Which angle?
        switch(rotationangle)
        {
          case 0:
            BlockCopy8(pParms->pdst->virtptr,
                       pParms->pdst->virtstride,
                       pParms->dstrect.left,
                       pParms->dstrect.top,
                       pParms->dstrect.width,
                       pParms->dstrect.height,
                       pParms->psrc->virtptr,
                       pParms->psrc->virtstride,
                       pParms->srcrect.left,
                       pParms->srcrect.top);
            break;
            
          case 90:
            Rotate_8_90(pParms->pdst->virtptr,
                        pParms->pdst->virtstride,
                        pParms->dstrect.left,
                        pParms->dstrect.top,
                        pParms->dstrect.width,
                        pParms->dstrect.height,
                        pParms->psrc->virtptr,
                        pParms->psrc->virtstride,
                        pParms->srcrect.left,
                        pParms->srcrect.top);
            break;
            
          case 180:
          case 270:
          default:
            error = TIBLT_ERR_UNSUPPORTED_ROTATION;
            goto Error;
        }
      }
      break;

    default:
      error = TIBLT_ERR_UNSUPPORTED_ROP;
      goto Error;
  }

Error:
  return(error);
}

TIBLTERROR TIBLTFN_toRGB16(TIBLTPARAMS* pParms)
{
  TIBLTERROR error = TIBLT_ERR_NONE;

  // Any flag other than ROP, BLEND not supported
  if(pParms->flags & ~(TIBLT_FLAG_ROP | TIBLT_FLAG_BLEND))
  {
    error = TIBLT_ERR_UNSUPPORTED_FLAGS;
    goto Error;
  }

  // ROP or BLEND must be set
  if(pParms->flags & TIBLT_FLAG_ROP)
  {
    // Which ROP?
    switch(pParms->ROP)
    {
      case 0xF0F0:
        // Only fill with same format as destination supported
        if(pParms->pbrush->format != pParms->pdst->format)
        {
          error = TIBLT_ERR_UNSUPPORTED_CONVERSION;
          goto Error;
        }

        // only solid fill using 1x1 brush supported
        if((pParms->brushrect.width != 1) ||
           (pParms->brushrect.height != 1))
        {
          error = TIBLT_ERR_UNSUPPORTED_BRUSH_RECT;
          goto Error;
        }

        // Do the fill
        BlockFill16(pParms->pdst->virtptr,
                    pParms->pdst->virtstride,
                    pParms->dstrect.left,
                    pParms->dstrect.top,
                    pParms->dstrect.width,
                    pParms->dstrect.height,
                    *((unsigned short*)pParms->pbrush->virtptr +
                      pParms->brushrect.left +
                      ((pParms->brushrect.top * pParms->pbrush->virtstride) / sizeof(unsigned short))));
        break;
        
      case 0xCCCC:  // SRCCOPY
        {
          int rotationangle = NormalizeAngle(pParms->pdst->orientation - pParms->psrc->orientation);
          
          // No overlap support
          if(pParms->pdst == pParms->psrc)
          {
            if(RectsIntersect(&(pParms->dstrect), &(pParms->srcrect)))
            {
              error = TIBLT_ERR_UNSUPPORTED_SRC_RECT;
              goto Error;
            }
          }

          // No scaling supported
          switch(rotationangle)
          {
            case 0:
            case 180:
              if((pParms->dstrect.width != pParms->srcrect.width) ||
                 (pParms->dstrect.height != pParms->srcrect.height))
              {
                error = TIBLT_ERR_UNSUPPORTED_SCALE_FACTOR;
                goto Error;
              }
              break;

            case 90:
            case 270:
              if((pParms->dstrect.width != pParms->srcrect.height) ||
                 (pParms->dstrect.height != pParms->srcrect.width))
              {
                error = TIBLT_ERR_UNSUPPORTED_SCALE_FACTOR;
                goto Error;
              }
              break;

            default:
              error = TIBLT_ERR_UNSUPPORTED_ROTATION;
              goto Error;
          }

          // No rotation while color space convering
          if(rotationangle != 0)
          {
            if(pParms->psrc->format != pParms->pdst->format)
            {
              error = TIBLT_ERR_UNSUPPORTED_COMBINATION;
              goto Error;
            }
          }

          // Which angle?
          switch(rotationangle)
          {
            case 0:
              switch(pParms->psrc->format)
              {
                case TIBLT_FMT_LUT8:
                  BlockCopyLUT8to16(pParms->pdst->virtptr,
                                    pParms->pdst->virtstride,
                                    pParms->dstrect.left,
                                    pParms->dstrect.top,
                                    pParms->dstrect.width,
                                    pParms->dstrect.height,
                                    pParms->psrc->virtptr,
                                    pParms->psrc->virtstride,
                                    pParms->srcrect.left,
                                    pParms->srcrect.top,
                                    pParms->psrc->palette);
                  break;

                case TIBLT_FMT_RGB16:
                  BlockCopy16(pParms->pdst->virtptr,
                              pParms->pdst->virtstride,
                              pParms->dstrect.left,
                              pParms->dstrect.top,
                              pParms->dstrect.width,
                              pParms->dstrect.height,
                              pParms->psrc->virtptr,
                              pParms->psrc->virtstride,
                              pParms->srcrect.left,
                              pParms->srcrect.top);
                  break;

                case TIBLT_FMT_BGR24:
                  BlockCopyBGR24toRGB16(pParms->pdst->virtptr,
                                        pParms->pdst->virtstride,
                                        pParms->dstrect.left,
                                        pParms->dstrect.top,
                                        pParms->dstrect.width,
                                        pParms->dstrect.height,
                                        pParms->psrc->virtptr,
                                        pParms->psrc->virtstride,
                                        pParms->srcrect.left,
                                        pParms->srcrect.top);
                  break;

                case TIBLT_FMT_BGRx32:
                case TIBLT_FMT_BGRA32:
                  BlockCopyBGRx32toRGB16(pParms->pdst->virtptr,
                                         pParms->pdst->virtstride,
                                         pParms->dstrect.left,
                                         pParms->dstrect.top,
                                         pParms->dstrect.width,
                                         pParms->dstrect.height,
                                         pParms->psrc->virtptr,
                                         pParms->psrc->virtstride,
                                         pParms->srcrect.left,
                                         pParms->srcrect.top);
                  break;

                case TIBLT_FMT_UYVY:
                  // Can't split subsampled pixel pairs
                  if(pParms->srcrect.left & 1)
                  {
                    error = TIBLT_ERR_UNSUPPORTED_SRC_RECT;
                    goto Error;
                  }
                  if(pParms->dstrect.width & 1)
                  {
                    error = TIBLT_ERR_UNSUPPORTED_DST_RECT;
                    goto Error;
                  }
                  
                  BlockCopyUYVYtoRGB16(pParms->pdst->virtptr,
                                       pParms->pdst->virtstride,
                                       pParms->dstrect.left,
                                       pParms->dstrect.top,
                                       pParms->dstrect.width,
                                       pParms->dstrect.height,
                                       pParms->psrc->virtptr,
                                       pParms->psrc->virtstride,
                                       pParms->srcrect.left,
                                       pParms->srcrect.top);
                  break;

                case TIBLT_FMT_IYUV:
                  // Can't split subsampled pixel quads
                  if((pParms->srcrect.left | pParms->srcrect.top) & 1)
                  {
                    error = TIBLT_ERR_UNSUPPORTED_SRC_RECT;
                    goto Error;
                  }
                  if((pParms->dstrect.width | pParms->dstrect.height) & 1)
                  {
                    error = TIBLT_ERR_UNSUPPORTED_DST_RECT;
                    goto Error;
                  }
                  
                  BlockCopyIYUVtoRGB16(pParms->pdst->virtptr,
                                       pParms->pdst->virtstride,
                                       pParms->dstrect.left,
                                       pParms->dstrect.top,
                                       pParms->dstrect.width,
                                       pParms->dstrect.height,
                                       pParms->psrc->virtptr,
                                       pParms->psrc->virtstride,
                                       pParms->srcrect.left,
                                       pParms->srcrect.top,
                                       pParms->psrc->height);
                  break;
                  
                default:
                  error = TIBLT_ERR_UNSUPPORTED_SRC_FORMAT;
                  goto Error;
              }
              break;
              
            case 90:
              Rotate_16_90(pParms->pdst->virtptr,
                           pParms->pdst->virtstride,
                           pParms->dstrect.left,
                           pParms->dstrect.top,
                           pParms->dstrect.width,
                           pParms->dstrect.height,
                           pParms->psrc->virtptr,
                           pParms->psrc->virtstride,
                           pParms->srcrect.left,
                           pParms->srcrect.top);
              break;
              
            case 180:
              Rotate_16_180(pParms->pdst->virtptr,
                            pParms->pdst->virtstride,
                            pParms->dstrect.left,
                            pParms->dstrect.top,
                            pParms->dstrect.width,
                            pParms->dstrect.height,
                            pParms->psrc->virtptr,
                            pParms->psrc->virtstride,
                            pParms->srcrect.left,
                            pParms->srcrect.top);
              break;
              
            case 270:
              Rotate_16_270(pParms->pdst->virtptr,
                            pParms->pdst->virtstride,
                            pParms->dstrect.left,
                            pParms->dstrect.top,
                            pParms->dstrect.width,
                            pParms->dstrect.height,
                            pParms->psrc->virtptr,
                            pParms->psrc->virtstride,
                            pParms->srcrect.left,
                            pParms->srcrect.top);
              break;
              
            default:
              error = TIBLT_ERR_UNSUPPORTED_ROTATION;
              goto Error;
          }
        }
        break;

      default:
        error = TIBLT_ERR_UNSUPPORTED_ROP;
        goto Error;
    }
  }
  else if(pParms->flags & TIBLT_FLAG_BLEND)
  {
    switch(pParms->psrc->format)
    {
      case TIBLT_FMT_RGB16:
        switch(pParms->blendtype)
        {
          case (TIBLT_BLEND_GLOBAL | TIBLT_BLEND_SRCOVER):
            AlphaBlendRGB16toRGB16withConst(pParms->pdst->virtptr,
                                            pParms->pdst->virtstride,
                                            pParms->dstrect.left,
                                            pParms->dstrect.top,
                                            pParms->dstrect.width,
                                            pParms->dstrect.height,
                                            pParms->psrc->virtptr,
                                            pParms->psrc->virtstride,
                                            pParms->srcrect.left,
                                            pParms->srcrect.top,
                                            (unsigned char)pParms->globalalpha);
            break;

          default:
            error = TIBLT_ERR_UNSUPPORTED_BLEND;
            goto Error;
        }
        break;
        
      case TIBLT_FMT_BGRA32:
        switch(pParms->blendtype)
        {
          case (TIBLT_BLEND_SRCLOCAL | TIBLT_BLEND_SRCOVER):
            AlphaBlendpBGRA32toRGB16(pParms->pdst->virtptr,
                                     pParms->pdst->virtstride,
                                     pParms->dstrect.left,
                                     pParms->dstrect.top,
                                     pParms->dstrect.width,
                                     pParms->dstrect.height,
                                     pParms->psrc->virtptr,
                                     pParms->psrc->virtstride,
                                     pParms->srcrect.left,
                                     pParms->srcrect.top);
            break;

          default:
            error = TIBLT_ERR_UNSUPPORTED_BLEND;
            goto Error;
        }
        break;

      default:
        error = TIBLT_ERR_UNSUPPORTED_COMBINATION;
        break;
    }
  }
  else
  {
    error = TIBLT_ERR_UNSUPPORTED_FLAGS;
    goto Error;
  }

Error:
  return(error);
}

TIBLTERROR TIBLTFN_toBGR24(TIBLTPARAMS* pParms)
{
  TIBLTERROR error = TIBLT_ERR_NONE;

  // Any flag other than ROP not supported
  if(pParms->flags & ~(TIBLT_FLAG_ROP))
  {
    error = TIBLT_ERR_UNSUPPORTED_FLAGS;
    goto Error;
  }

  // ROP or BLEND must be set
  if(pParms->flags & TIBLT_FLAG_ROP)
  {
    // Which ROP?
    switch(pParms->ROP)
    {
      case 0xF0F0:
        // Only fill with same format as destination supported
        if(pParms->pbrush->format != pParms->pdst->format)
        {
          error = TIBLT_ERR_UNSUPPORTED_CONVERSION;
          goto Error;
        }

        // only solid fill using 1x1 brush supported
        if((pParms->brushrect.width != 1) ||
           (pParms->brushrect.height != 1))
        {
          error = TIBLT_ERR_UNSUPPORTED_BRUSH_RECT;
          goto Error;
        }

        // Do the fill
        {
          unsigned char* p = (unsigned char*)pParms->pbrush->virtptr +
                             (pParms->brushrect.left * 3) +
                             (pParms->brushrect.top * pParms->pbrush->virtstride);
          unsigned long color = ((unsigned long)(*(p+2)) << 16) | ((unsigned long)(*(p+1)) << 8)  | *p;
          BlockFill24(pParms->pdst->virtptr,
                      pParms->pdst->virtstride,
                      pParms->dstrect.left,
                      pParms->dstrect.top,
                      pParms->dstrect.width,
                      pParms->dstrect.height,
                      color);
        }
        break;
        
      case 0xCCCC:  // SRCCOPY
        {
          int rotationangle = NormalizeAngle(pParms->pdst->orientation - pParms->psrc->orientation);
          
          // No overlap support
          if(pParms->pdst == pParms->psrc)
          {
            if(RectsIntersect(&(pParms->dstrect), &(pParms->srcrect)))
            {
              error = TIBLT_ERR_UNSUPPORTED_SRC_RECT;
              goto Error;
            }
          }

          // No scaling supported
          switch(rotationangle)
          {
            case 0:
            case 180:
              if((pParms->dstrect.width != pParms->srcrect.width) ||
                 (pParms->dstrect.height != pParms->srcrect.height))
              {
                error = TIBLT_ERR_UNSUPPORTED_SCALE_FACTOR;
                goto Error;
              }
              break;

            case 90:
            case 270:
              if((pParms->dstrect.width != pParms->srcrect.height) ||
                 (pParms->dstrect.height != pParms->srcrect.width))
              {
                error = TIBLT_ERR_UNSUPPORTED_SCALE_FACTOR;
                goto Error;
              }
              break;

            default:
              error = TIBLT_ERR_UNSUPPORTED_ROTATION;
              goto Error;
          }

          // No rotation while color space convering
          if(rotationangle != 0)
          {
            if(pParms->psrc->format != pParms->pdst->format)
            {
              error = TIBLT_ERR_UNSUPPORTED_COMBINATION;
              goto Error;
            }
          }

          // Which angle?
          switch(rotationangle)
          {
            case 0:
              switch(pParms->psrc->format)
              {
                case TIBLT_FMT_LUT8:
                  BlockCopyLUT8to24(pParms->pdst->virtptr,
                                    pParms->pdst->virtstride,
                                    pParms->dstrect.left,
                                    pParms->dstrect.top,
                                    pParms->dstrect.width,
                                    pParms->dstrect.height,
                                    pParms->psrc->virtptr,
                                    pParms->psrc->virtstride,
                                    pParms->srcrect.left,
                                    pParms->srcrect.top,
                                    pParms->psrc->palette);
                  break;

                case TIBLT_FMT_RGB16:
                  BlockCopyRGB16toBGR24(pParms->pdst->virtptr,
                                        pParms->pdst->virtstride,
                                        pParms->dstrect.left,
                                        pParms->dstrect.top,
                                        pParms->dstrect.width,
                                        pParms->dstrect.height,
                                        pParms->psrc->virtptr,
                                        pParms->psrc->virtstride,
                                        pParms->srcrect.left,
                                        pParms->srcrect.top);
                  break;

                case TIBLT_FMT_BGR24:
                  BlockCopy24(pParms->pdst->virtptr,
                              pParms->pdst->virtstride,
                              pParms->dstrect.left,
                              pParms->dstrect.top,
                              pParms->dstrect.width,
                              pParms->dstrect.height,
                              pParms->psrc->virtptr,
                              pParms->psrc->virtstride,
                              pParms->srcrect.left,
                              pParms->srcrect.top);
                  break;

                case TIBLT_FMT_BGRx32:
                case TIBLT_FMT_BGRA32:
                  BlockCopyXYZx32toXYZ24(pParms->pdst->virtptr,
                                         pParms->pdst->virtstride,
                                         pParms->dstrect.left,
                                         pParms->dstrect.top,
                                         pParms->dstrect.width,
                                         pParms->dstrect.height,
                                         pParms->psrc->virtptr,
                                         pParms->psrc->virtstride,
                                         pParms->srcrect.left,
                                         pParms->srcrect.top);
                  break;

                case TIBLT_FMT_UYVY:
                  // Can't split subsampled pixel pairs
                  if(pParms->srcrect.left & 1)
                  {
                    error = TIBLT_ERR_UNSUPPORTED_SRC_RECT;
                    goto Error;
                  }
                  if(pParms->dstrect.width & 1)
                  {
                    error = TIBLT_ERR_UNSUPPORTED_DST_RECT;
                    goto Error;
                  }
                  
                  BlockCopyUYVYtoBGR24(pParms->pdst->virtptr,
                                       pParms->pdst->virtstride,
                                       pParms->dstrect.left,
                                       pParms->dstrect.top,
                                       pParms->dstrect.width,
                                       pParms->dstrect.height,
                                       pParms->psrc->virtptr,
                                       pParms->psrc->virtstride,
                                       pParms->srcrect.left,
                                       pParms->srcrect.top);
                  break;

                default:
                  error = TIBLT_ERR_UNSUPPORTED_SRC_FORMAT;
                  goto Error;
              }
              break;
              
            case 90:
            case 180:
            case 270:
            default:
              error = TIBLT_ERR_UNSUPPORTED_ROTATION;
              goto Error;
          }
        }
        break;

      default:
        error = TIBLT_ERR_UNSUPPORTED_ROP;
        goto Error;
    }
  }
  else if(pParms->flags & TIBLT_FLAG_BLEND)
  {
    switch(pParms->psrc->format)
    {
      case TIBLT_FMT_RGB16:
        switch(pParms->blendtype)
        {
          case (TIBLT_BLEND_GLOBAL | TIBLT_BLEND_SRCOVER):
          default:
            error = TIBLT_ERR_UNSUPPORTED_BLEND;
            goto Error;
        }
        break;
        
      case TIBLT_FMT_BGRA32:
        switch(pParms->blendtype)
        {
          case (TIBLT_BLEND_SRCLOCAL | TIBLT_BLEND_SRCOVER):
          default:
            error = TIBLT_ERR_UNSUPPORTED_BLEND;
            goto Error;
        }
        break;

      default:
        error = TIBLT_ERR_UNSUPPORTED_COMBINATION;
        break;
    }
  }
  else
  {
    error = TIBLT_ERR_UNSUPPORTED_FLAGS;
    goto Error;
  }

Error:
  return(error);
}

TIBLTERROR TIBLTFN_toBGRx32(TIBLTPARAMS* pParms)
{
  TIBLTERROR error = TIBLT_ERR_NONE;

  // Any flag other than ROP not supported
  if(pParms->flags & ~(TIBLT_FLAG_ROP))
  {
    error = TIBLT_ERR_UNSUPPORTED_FLAGS;
    goto Error;
  }

  // ROP or BLEND must be set
  if(pParms->flags & TIBLT_FLAG_ROP)
  {
    // Which ROP?
    switch(pParms->ROP)
    {
      case 0xF0F0:
        // Only fill with same format as destination supported
        if(pParms->pbrush->format != pParms->pdst->format)
        {
          error = TIBLT_ERR_UNSUPPORTED_CONVERSION;
          goto Error;
        }

        // only solid fill using 1x1 brush supported
        if((pParms->brushrect.width != 1) ||
           (pParms->brushrect.height != 1))
        {
          error = TIBLT_ERR_UNSUPPORTED_BRUSH_RECT;
          goto Error;
        }

        // Do the fill
        BlockFill32(pParms->pdst->virtptr,
                    pParms->pdst->virtstride,
                    pParms->dstrect.left,
                    pParms->dstrect.top,
                    pParms->dstrect.width,
                    pParms->dstrect.height,
                    *((unsigned long*)pParms->pbrush->virtptr +
                      pParms->brushrect.left +
                      ((pParms->brushrect.top * pParms->pbrush->virtstride) / sizeof(unsigned long))));
        break;
        
      case 0xCCCC:  // SRCCOPY
        {
          int rotationangle = NormalizeAngle(pParms->pdst->orientation - pParms->psrc->orientation);
          
          // No overlap support
          if(pParms->pdst == pParms->psrc)
          {
            if(RectsIntersect(&(pParms->dstrect), &(pParms->srcrect)))
            {
              error = TIBLT_ERR_UNSUPPORTED_SRC_RECT;
              goto Error;
            }
          }

          // No scaling supported
          switch(rotationangle)
          {
            case 0:
            case 180:
              if((pParms->dstrect.width != pParms->srcrect.width) ||
                 (pParms->dstrect.height != pParms->srcrect.height))
              {
                error = TIBLT_ERR_UNSUPPORTED_SCALE_FACTOR;
                goto Error;
              }
              break;

            case 90:
            case 270:
              if((pParms->dstrect.width != pParms->srcrect.height) ||
                 (pParms->dstrect.height != pParms->srcrect.width))
              {
                error = TIBLT_ERR_UNSUPPORTED_SCALE_FACTOR;
                goto Error;
              }
              break;

            default:
              error = TIBLT_ERR_UNSUPPORTED_ROTATION;
              goto Error;
          }

          // No rotation while color space convering
          if(rotationangle != 0)
          {
            if(pParms->psrc->format != pParms->pdst->format)
            {
              error = TIBLT_ERR_UNSUPPORTED_COMBINATION;
              goto Error;
            }
          }

          // Which angle?
          switch(rotationangle)
          {
            case 0:
              switch(pParms->psrc->format)
              {
                case TIBLT_FMT_LUT8:
                  BlockCopyLUT8to32(pParms->pdst->virtptr,
                                    pParms->pdst->virtstride,
                                    pParms->dstrect.left,
                                    pParms->dstrect.top,
                                    pParms->dstrect.width,
                                    pParms->dstrect.height,
                                    pParms->psrc->virtptr,
                                    pParms->psrc->virtstride,
                                    pParms->srcrect.left,
                                    pParms->srcrect.top,
                                    pParms->psrc->palette);
                  break;

                case TIBLT_FMT_RGB16:
                  BlockCopyRGB16toBGRx32(pParms->pdst->virtptr,
                                         pParms->pdst->virtstride,
                                         pParms->dstrect.left,
                                         pParms->dstrect.top,
                                         pParms->dstrect.width,
                                         pParms->dstrect.height,
                                         pParms->psrc->virtptr,
                                         pParms->psrc->virtstride,
                                         pParms->srcrect.left,
                                         pParms->srcrect.top);
                  break;

                case TIBLT_FMT_BGR24:
                  BlockCopyXYZ24toXYZx32(pParms->pdst->virtptr,
                                         pParms->pdst->virtstride,
                                         pParms->dstrect.left,
                                         pParms->dstrect.top,
                                         pParms->dstrect.width,
                                         pParms->dstrect.height,
                                         pParms->psrc->virtptr,
                                         pParms->psrc->virtstride,
                                         pParms->srcrect.left,
                                         pParms->srcrect.top);
                  break;

                case TIBLT_FMT_BGRx32:
                case TIBLT_FMT_BGRA32:
                  BlockCopy32(pParms->pdst->virtptr,
                              pParms->pdst->virtstride,
                              pParms->dstrect.left,
                              pParms->dstrect.top,
                              pParms->dstrect.width,
                              pParms->dstrect.height,
                              pParms->psrc->virtptr,
                              pParms->psrc->virtstride,
                              pParms->srcrect.left,
                              pParms->srcrect.top);
                  break;

                case TIBLT_FMT_UYVY:
                  // Can't split subsampled pixel pairs
                  if(pParms->srcrect.left & 1)
                  {
                    error = TIBLT_ERR_UNSUPPORTED_SRC_RECT;
                    goto Error;
                  }
                  if(pParms->dstrect.width & 1)
                  {
                    error = TIBLT_ERR_UNSUPPORTED_DST_RECT;
                    goto Error;
                  }
                  
                  BlockCopyUYVYtoBGRx32(pParms->pdst->virtptr,
                                        pParms->pdst->virtstride,
                                        pParms->dstrect.left,
                                        pParms->dstrect.top,
                                        pParms->dstrect.width,
                                        pParms->dstrect.height,
                                        pParms->psrc->virtptr,
                                        pParms->psrc->virtstride,
                                        pParms->srcrect.left,
                                        pParms->srcrect.top);
                  break;

                default:
                  error = TIBLT_ERR_UNSUPPORTED_SRC_FORMAT;
                  goto Error;
              }
              break;
              
            case 90:
            case 180:
            case 270:
            default:
              error = TIBLT_ERR_UNSUPPORTED_ROTATION;
              goto Error;
          }
        }
        break;

      default:
        error = TIBLT_ERR_UNSUPPORTED_ROP;
        goto Error;
    }
  }
  else if(pParms->flags & TIBLT_FLAG_BLEND)
  {
    switch(pParms->psrc->format)
    {
      case TIBLT_FMT_RGB16:
        switch(pParms->blendtype)
        {
          case (TIBLT_BLEND_GLOBAL | TIBLT_BLEND_SRCOVER):
          default:
            error = TIBLT_ERR_UNSUPPORTED_BLEND;
            goto Error;
        }
        break;
        
      case TIBLT_FMT_BGRA32:
        switch(pParms->blendtype)
        {
          case (TIBLT_BLEND_SRCLOCAL | TIBLT_BLEND_SRCOVER):
          default:
            error = TIBLT_ERR_UNSUPPORTED_BLEND;
            goto Error;
        }
        break;

      default:
        error = TIBLT_ERR_UNSUPPORTED_COMBINATION;
        break;
    }
  }
  else
  {
    error = TIBLT_ERR_UNSUPPORTED_FLAGS;
    goto Error;
  }

Error:
  return(error);
}

TIBLTERROR TIBLTFN_toBGRA32(TIBLTPARAMS* pParms)
{
  TIBLTERROR error = TIBLT_ERR_NONE;

  // ROP or BLEND must be set
  if(pParms->flags & TIBLT_FLAG_ROP)
  {
    // Which ROP?
    switch(pParms->ROP)
    {
      case 0xF0F0:
        // Only fill with same format as destination supported
        if(pParms->pbrush->format != pParms->pdst->format)
        {
          error = TIBLT_ERR_UNSUPPORTED_CONVERSION;
          goto Error;
        }

        // only solid fill using 1x1 brush supported
        if((pParms->brushrect.width != 1) ||
           (pParms->brushrect.height != 1))
        {
          error = TIBLT_ERR_UNSUPPORTED_BRUSH_RECT;
          goto Error;
        }

        // Do the fill
        BlockFill32(pParms->pdst->virtptr,
                    pParms->pdst->virtstride,
                    pParms->dstrect.left,
                    pParms->dstrect.top,
                    pParms->dstrect.width,
                    pParms->dstrect.height,
                    *((unsigned long*)pParms->pbrush->virtptr +
                      pParms->brushrect.left +
                      ((pParms->brushrect.top * pParms->pbrush->virtstride) / sizeof(unsigned long))));
        break;
        
      case 0xCCCC:  // SRCCOPY
        {
          int rotationangle = NormalizeAngle(pParms->pdst->orientation - pParms->psrc->orientation);
          
          // No overlap support
          if(pParms->pdst == pParms->psrc)
          {
            if(RectsIntersect(&(pParms->dstrect), &(pParms->srcrect)))
            {
              error = TIBLT_ERR_UNSUPPORTED_SRC_RECT;
              goto Error;
            }
          }

          // No scaling supported
          switch(rotationangle)
          {
            case 0:
            case 180:
              if((pParms->dstrect.width != pParms->srcrect.width) ||
                 (pParms->dstrect.height != pParms->srcrect.height))
              {
                error = TIBLT_ERR_UNSUPPORTED_SCALE_FACTOR;
                goto Error;
              }
              break;

            case 90:
            case 270:
              if((pParms->dstrect.width != pParms->srcrect.height) ||
                 (pParms->dstrect.height != pParms->srcrect.width))
              {
                error = TIBLT_ERR_UNSUPPORTED_SCALE_FACTOR;
                goto Error;
              }
              break;

            default:
              error = TIBLT_ERR_UNSUPPORTED_ROTATION;
              goto Error;
          }

          // No rotation while color space convering
          if(rotationangle != 0)
          {
            if(pParms->psrc->format != pParms->pdst->format)
            {
              error = TIBLT_ERR_UNSUPPORTED_COMBINATION;
              goto Error;
            }
          }

          // Which angle?
          switch(rotationangle)
          {
            case 0:
              switch(pParms->psrc->format)
              {
                case TIBLT_FMT_LUT8:
                  BlockCopyLUT8to32(pParms->pdst->virtptr,
                                    pParms->pdst->virtstride,
                                    pParms->dstrect.left,
                                    pParms->dstrect.top,
                                    pParms->dstrect.width,
                                    pParms->dstrect.height,
                                    pParms->psrc->virtptr,
                                    pParms->psrc->virtstride,
                                    pParms->srcrect.left,
                                    pParms->srcrect.top,
                                    pParms->psrc->palette);
                  break;

                case TIBLT_FMT_RGB16:
                  BlockCopyRGB16toBGRx32(pParms->pdst->virtptr,
                                         pParms->pdst->virtstride,
                                         pParms->dstrect.left,
                                         pParms->dstrect.top,
                                         pParms->dstrect.width,
                                         pParms->dstrect.height,
                                         pParms->psrc->virtptr,
                                         pParms->psrc->virtstride,
                                         pParms->srcrect.left,
                                         pParms->srcrect.top);
                  break;

                case TIBLT_FMT_BGR24:
                  BlockCopyXYZ24toXYZx32(pParms->pdst->virtptr,
                                         pParms->pdst->virtstride,
                                         pParms->dstrect.left,
                                         pParms->dstrect.top,
                                         pParms->dstrect.width,
                                         pParms->dstrect.height,
                                         pParms->psrc->virtptr,
                                         pParms->psrc->virtstride,
                                         pParms->srcrect.left,
                                         pParms->srcrect.top);
                  break;

                case TIBLT_FMT_BGRA32:
                  BlockCopy32(pParms->pdst->virtptr,
                              pParms->pdst->virtstride,
                              pParms->dstrect.left,
                              pParms->dstrect.top,
                              pParms->dstrect.width,
                              pParms->dstrect.height,
                              pParms->psrc->virtptr,
                              pParms->psrc->virtstride,
                              pParms->srcrect.left,
                              pParms->srcrect.top);
                  break;

                case TIBLT_FMT_UYVY:
                  // Can't split subsampled pixel pairs
                  if(pParms->srcrect.left & 1)
                  {
                    error = TIBLT_ERR_UNSUPPORTED_SRC_RECT;
                    goto Error;
                  }
                  if(pParms->dstrect.width & 1)
                  {
                    error = TIBLT_ERR_UNSUPPORTED_DST_RECT;
                    goto Error;
                  }
                  
                  BlockCopyUYVYtoBGRx32(pParms->pdst->virtptr,
                                        pParms->pdst->virtstride,
                                        pParms->dstrect.left,
                                        pParms->dstrect.top,
                                        pParms->dstrect.width,
                                        pParms->dstrect.height,
                                        pParms->psrc->virtptr,
                                        pParms->psrc->virtstride,
                                        pParms->srcrect.left,
                                        pParms->srcrect.top);
                  break;

                case TIBLT_FMT_BGRx32:
                  // premultiplied stripped of alpha not supported
                default:
                  error = TIBLT_ERR_UNSUPPORTED_SRC_FORMAT;
                  goto Error;
              }
              break;
              
            case 90:
            case 180:
            case 270:
            default:
              error = TIBLT_ERR_UNSUPPORTED_ROTATION;
              goto Error;
          }
        }
        break;

      default:
        error = TIBLT_ERR_UNSUPPORTED_ROP;
        goto Error;
    }
  }
  else if(pParms->flags & TIBLT_FLAG_BLEND)
  {
    switch(pParms->psrc->format)
    {
      case TIBLT_FMT_RGB16:
        switch(pParms->blendtype)
        {
          case (TIBLT_BLEND_GLOBAL | TIBLT_BLEND_SRCOVER):
          default:
            error = TIBLT_ERR_UNSUPPORTED_BLEND;
            goto Error;
        }
        break;
        
      case TIBLT_FMT_BGRA32:
        switch(pParms->blendtype)
        {
          case (TIBLT_BLEND_SRCLOCAL | TIBLT_BLEND_DSTLOCAL | TIBLT_BLEND_SRCOVER):
            AlphaBlendpBGRA32topBGRA32(pParms->pdst->virtptr,
                                       pParms->pdst->virtstride,
                                       pParms->dstrect.left,
                                       pParms->dstrect.top,
                                       pParms->dstrect.width,
                                       pParms->dstrect.height,
                                       pParms->psrc->virtptr,
                                       pParms->psrc->virtstride,
                                       pParms->srcrect.left,
                                       pParms->srcrect.top);
            break;
            
          default:
            error = TIBLT_ERR_UNSUPPORTED_BLEND;
            goto Error;
        }
        break;
 
      case TIBLT_FMT_BGRA32_NONPREMULT:
        switch(pParms->blendtype)
        {
          case (TIBLT_BLEND_SRCLOCAL | TIBLT_BLEND_DSTLOCAL | TIBLT_BLEND_SRCOVER):
            AlphaBlendBGRA32topBGRA32(pParms->pdst->virtptr,
                                      pParms->pdst->virtstride,
                                      pParms->dstrect.left,
                                      pParms->dstrect.top,
                                      pParms->dstrect.width,
                                      pParms->dstrect.height,
                                      pParms->psrc->virtptr,
                                      pParms->psrc->virtstride,
                                      pParms->srcrect.left,
                                      pParms->srcrect.top);
            break;
            
          default:
            error = TIBLT_ERR_UNSUPPORTED_BLEND;
            goto Error;
        }
        break;

      default:
        error = TIBLT_ERR_UNSUPPORTED_COMBINATION;
        break;
    }
  }
  else
  {
    error = TIBLT_ERR_UNSUPPORTED_FLAGS;
    goto Error;
  }

Error:
  return(error);
}


void UnpremultXYZA32toXYZA32(void*          dstptr,
                             long           dststride,
                             void*          srcptr,
                             long           srcstride,
                             unsigned long  width,
                             unsigned long  height,
                             unsigned long* lut);
 
void UnpremultXYZA32(void*          dstptr,
                     long           dststride,
                     unsigned long  width,
                     unsigned long  height,
                     unsigned long* lut);
 
unsigned long recip16[256] =
{
  0, 65280, 32640, 21760, 16320, 13056, 10880, 9326, 8160, 7253, 6528, 5935, 5440, 5022, 4663, 4352,
  4080, 3840, 3627, 3436, 3264, 3109, 2967, 2838, 2720, 2611, 2511, 2418, 2331, 2251, 2176, 2106,
  2040, 1978, 1920, 1865, 1813, 1764, 1718, 1674, 1632, 1592, 1554, 1518, 1484, 1451, 1419, 1389,
  1360, 1332, 1306, 1280, 1255, 1232, 1209, 1187, 1166, 1145, 1126, 1106, 1088, 1070, 1053, 1036,
  1020, 1004, 989, 974, 960, 946, 933, 919, 907, 894, 882, 870, 859, 848, 837, 826,
  816, 806, 796, 787, 777, 768, 759, 750, 742, 733, 725, 717, 710, 702, 694, 687,
  680, 673, 666, 659, 653, 646, 640, 634, 628, 622, 616, 610, 604, 599, 593, 588,
  583, 578, 573, 568, 563, 558, 553, 549, 544, 540, 535, 531, 526, 522, 518, 514,
  510, 506, 502, 498, 495, 491, 487, 484, 480, 476, 473, 470, 466, 463, 460, 457,
  453, 450, 447, 444, 441, 438, 435, 432, 429, 427, 424, 421, 418, 416, 413, 411,
  408, 405, 403, 400, 398, 396, 393, 391, 389, 386, 384, 382, 380, 377, 375, 373,
  371, 369, 367, 365, 363, 361, 359, 357, 355, 353, 351, 349, 347, 345, 344, 342,
  340, 338, 336, 335, 333, 331, 330, 328, 326, 325, 323, 322, 320, 318, 317, 315,
  314, 312, 311, 309, 308, 306, 305, 304, 302, 301, 299, 298, 297, 295, 294, 293,
  291, 290, 289, 288, 286, 285, 284, 283, 281, 280, 279, 278, 277, 275, 274, 273,
  272, 271, 270, 269, 268, 266, 265, 264, 263, 262, 261, 260, 259, 258, 257, 256
};

TIBLTERROR TIBLTFN_tonBGRA32(TIBLTPARAMS* pParms)
{
  TIBLTERROR error = TIBLT_ERR_NONE;

  // Any flag other than ROP not supported
  if(pParms->flags & ~(TIBLT_FLAG_ROP))
  {
    error = TIBLT_ERR_UNSUPPORTED_FLAGS;
    goto Error;
  }

  // ROP or BLEND must be set
  if(pParms->flags & TIBLT_FLAG_ROP)
  {
    // Which ROP?
    switch(pParms->ROP)
    {
      case 0xF0F0:
        // Only fill with same format as destination supported
        if(pParms->pbrush->format != pParms->pdst->format)
        {
          error = TIBLT_ERR_UNSUPPORTED_CONVERSION;
          goto Error;
        }

        // only solid fill using 1x1 brush supported
        if((pParms->brushrect.width != 1) ||
           (pParms->brushrect.height != 1))
        {
          error = TIBLT_ERR_UNSUPPORTED_BRUSH_RECT;
          goto Error;
        }

        // Do the fill
        BlockFill32(pParms->pdst->virtptr,
                    pParms->pdst->virtstride,
                    pParms->dstrect.left,
                    pParms->dstrect.top,
                    pParms->dstrect.width,
                    pParms->dstrect.height,
                    *((unsigned long*)pParms->pbrush->virtptr +
                      pParms->brushrect.left +
                      ((pParms->brushrect.top * pParms->pbrush->virtstride) / sizeof(unsigned long))));
        break;
        
      case 0xCCCC:  // SRCCOPY
        {
          int rotationangle = NormalizeAngle(pParms->pdst->orientation - pParms->psrc->orientation);
          
          // No overlap support
          if(pParms->pdst == pParms->psrc)
          {
            if(RectsIntersect(&(pParms->dstrect), &(pParms->srcrect)))
            {
              error = TIBLT_ERR_UNSUPPORTED_SRC_RECT;
              goto Error;
            }
          }

          // No scaling supported
          switch(rotationangle)
          {
            case 0:
            case 180:
              if((pParms->dstrect.width != pParms->srcrect.width) ||
                 (pParms->dstrect.height != pParms->srcrect.height))
              {
                error = TIBLT_ERR_UNSUPPORTED_SCALE_FACTOR;
                goto Error;
              }
              break;

            case 90:
            case 270:
              if((pParms->dstrect.width != pParms->srcrect.height) ||
                 (pParms->dstrect.height != pParms->srcrect.width))
              {
                error = TIBLT_ERR_UNSUPPORTED_SCALE_FACTOR;
                goto Error;
              }
              break;

            default:
              error = TIBLT_ERR_UNSUPPORTED_ROTATION;
              goto Error;
          }

          // No rotation while color space convering
          if(rotationangle != 0)
          {
            if(pParms->psrc->format != pParms->pdst->format)
            {
              error = TIBLT_ERR_UNSUPPORTED_COMBINATION;
              goto Error;
            }
          }

          // Which angle?
          switch(rotationangle)
          {
            case 0:
              switch(pParms->psrc->format)
              {
                case TIBLT_FMT_LUT8:
                  BlockCopyLUT8to32(pParms->pdst->virtptr,
                                    pParms->pdst->virtstride,
                                    pParms->dstrect.left,
                                    pParms->dstrect.top,
                                    pParms->dstrect.width,
                                    pParms->dstrect.height,
                                    pParms->psrc->virtptr,
                                    pParms->psrc->virtstride,
                                    pParms->srcrect.left,
                                    pParms->srcrect.top,
                                    pParms->psrc->palette);
                  break;

                case TIBLT_FMT_RGB16:
                  BlockCopyRGB16toBGRx32(pParms->pdst->virtptr,
                                         pParms->pdst->virtstride,
                                         pParms->dstrect.left,
                                         pParms->dstrect.top,
                                         pParms->dstrect.width,
                                         pParms->dstrect.height,
                                         pParms->psrc->virtptr,
                                         pParms->psrc->virtstride,
                                         pParms->srcrect.left,
                                         pParms->srcrect.top);
                  break;

                case TIBLT_FMT_BGR24:
                  BlockCopyXYZ24toXYZx32(pParms->pdst->virtptr,
                                         pParms->pdst->virtstride,
                                         pParms->dstrect.left,
                                         pParms->dstrect.top,
                                         pParms->dstrect.width,
                                         pParms->dstrect.height,
                                         pParms->psrc->virtptr,
                                         pParms->psrc->virtstride,
                                         pParms->srcrect.left,
                                         pParms->srcrect.top);
                  break;

                case TIBLT_FMT_BGRA32:
                  BlockCopy32(pParms->pdst->virtptr,
                              pParms->pdst->virtstride,
                              pParms->dstrect.left,
                              pParms->dstrect.top,
                              pParms->dstrect.width,
                              pParms->dstrect.height,
                              pParms->psrc->virtptr,
                              pParms->psrc->virtstride,
                              pParms->srcrect.left,
                              pParms->srcrect.top);
                  break;

                case TIBLT_FMT_BGRA32_NONPREMULT:
                  {
                    void* pdstbits = (void*)((long)pParms->pdst->virtptr +
                                             (pParms->dstrect.left * 4) +
                                             (pParms->dstrect.top * pParms->pdst->virtstride));
                    void* psrcbits = (void*)((long)pParms->psrc->virtptr +
                                             (pParms->srcrect.left * 4) +
                                             (pParms->srcrect.top * pParms->psrc->virtstride));
                    if(pParms->pdst->virtptr == pParms->psrc->virtptr)
                      UnpremultXYZA32(pdstbits,
                                      pParms->pdst->virtstride,
                                      pParms->dstrect.width,
                                      pParms->dstrect.height,
                                      recip16);
                    else
                      UnpremultXYZA32toXYZA32(pdstbits,
                                              pParms->pdst->virtstride,
                                              pParms->psrc->virtptr,
                                              pParms->psrc->virtstride,
                                              pParms->dstrect.width,
                                              pParms->dstrect.height,
                                              recip16);
                  }
                  break;

                case TIBLT_FMT_UYVY:
                  // Can't split subsampled pixel pairs
                  if(pParms->srcrect.left & 1)
                  {
                    error = TIBLT_ERR_UNSUPPORTED_SRC_RECT;
                    goto Error;
                  }
                  if(pParms->dstrect.width & 1)
                  {
                    error = TIBLT_ERR_UNSUPPORTED_DST_RECT;
                    goto Error;
                  }
                  
                  BlockCopyUYVYtoBGRx32(pParms->pdst->virtptr,
                                        pParms->pdst->virtstride,
                                        pParms->dstrect.left,
                                        pParms->dstrect.top,
                                        pParms->dstrect.width,
                                        pParms->dstrect.height,
                                        pParms->psrc->virtptr,
                                        pParms->psrc->virtstride,
                                        pParms->srcrect.left,
                                        pParms->srcrect.top);
                  break;

                case TIBLT_FMT_BGRx32:
                default:
                  error = TIBLT_ERR_UNSUPPORTED_SRC_FORMAT;
                  goto Error;
              }
              break;
              
            case 90:
            case 180:
            case 270:
            default:
              error = TIBLT_ERR_UNSUPPORTED_ROTATION;
              goto Error;
          }
        }
        break;

      default:
        error = TIBLT_ERR_UNSUPPORTED_ROP;
        goto Error;
    }
  }
  else if(pParms->flags & TIBLT_FLAG_BLEND)
  {
    switch(pParms->psrc->format)
    {
      case TIBLT_FMT_RGB16:
        switch(pParms->blendtype)
        {
          case (TIBLT_BLEND_GLOBAL | TIBLT_BLEND_SRCOVER):
          default:
            error = TIBLT_ERR_UNSUPPORTED_BLEND;
            goto Error;
        }
        break;
        
      case TIBLT_FMT_BGRA32:
        switch(pParms->blendtype)
        {
          case (TIBLT_BLEND_SRCLOCAL | TIBLT_BLEND_DSTLOCAL | TIBLT_BLEND_SRCOVER):
          default:
            error = TIBLT_ERR_UNSUPPORTED_BLEND;
            goto Error;
        }
        break;

      default:
        error = TIBLT_ERR_UNSUPPORTED_COMBINATION;
        break;
    }
  }
  else
  {
    error = TIBLT_ERR_UNSUPPORTED_FLAGS;
    goto Error;
  }

Error:
  return(error);
}

TIBLTERROR TIBLTFN_toUYVY(TIBLTPARAMS* pParms)
{
  TIBLTERROR error = TIBLT_ERR_NONE;

  // Any flag other than ROP not supported
  if(pParms->flags & ~(TIBLT_FLAG_ROP))
  {
    error = TIBLT_ERR_UNSUPPORTED_FLAGS;
    goto Error;
  }

  // ROP or BLEND must be set
  if(pParms->flags & TIBLT_FLAG_ROP)
  {
    // Which ROP?
    switch(pParms->ROP)
    {
      case 0xCCCC:  // SRCCOPY
        {
          int rotationangle = NormalizeAngle(pParms->pdst->orientation - pParms->psrc->orientation);
          
          // No overlap support
          if(pParms->pdst == pParms->psrc)
          {
            if(RectsIntersect(&(pParms->dstrect), &(pParms->srcrect)))
            {
              error = TIBLT_ERR_UNSUPPORTED_SRC_RECT;
              goto Error;
            }
          }

          // No scaling supported
          switch(rotationangle)
          {
            case 0:
            case 180:
              if((pParms->dstrect.width != pParms->srcrect.width) ||
                 (pParms->dstrect.height != pParms->srcrect.height))
              {
                error = TIBLT_ERR_UNSUPPORTED_SCALE_FACTOR;
                goto Error;
              }
              break;

            case 90:
            case 270:
              if((pParms->dstrect.width != pParms->srcrect.height) ||
                 (pParms->dstrect.height != pParms->srcrect.width))
              {
                error = TIBLT_ERR_UNSUPPORTED_SCALE_FACTOR;
                goto Error;
              }
              break;

            default:
              error = TIBLT_ERR_UNSUPPORTED_ROTATION;
              goto Error;
          }

          // No rotation while color space convering
          if(rotationangle != 0)
          {
            if(pParms->psrc->format != pParms->pdst->format)
            {
              error = TIBLT_ERR_UNSUPPORTED_COMBINATION;
              goto Error;
            }
          }

          // Which angle?
          switch(rotationangle)
          {
            case 0:
              switch(pParms->psrc->format)
              {
                case TIBLT_FMT_UYVY:
                  // Can't split subsampled pixel pairs
                  if(pParms->srcrect.left & 1)
                  {
                    error = TIBLT_ERR_UNSUPPORTED_SRC_RECT;
                    goto Error;
                  }
                  if(pParms->dstrect.width & 1)
                  {
                    error = TIBLT_ERR_UNSUPPORTED_DST_RECT;
                    goto Error;
                  }
                  
                  BlockCopy16(pParms->pdst->virtptr,
                              pParms->pdst->virtstride,
                              pParms->dstrect.left,
                              pParms->dstrect.top,
                              pParms->dstrect.width,
                              pParms->dstrect.height,
                              pParms->psrc->virtptr,
                              pParms->psrc->virtstride,
                              pParms->srcrect.left,
                              pParms->srcrect.top);
                  break;

                case TIBLT_FMT_IYUV:
                  // Can't split subsampled pixel pairs
                  if((pParms->srcrect.left |
                      pParms->dstrect.left) & 1)
                  {
                    error = TIBLT_ERR_UNSUPPORTED_SRC_RECT;
                    goto Error;
                  }
                  if(pParms->dstrect.width & 1)
                  {
                    error = TIBLT_ERR_UNSUPPORTED_DST_RECT;
                    goto Error;
                  }
                  
                  BlockCopyIYUVtoUYVY(pParms->pdst->virtptr,
                                      pParms->pdst->virtstride,
                                      pParms->dstrect.left,
                                      pParms->dstrect.top,
                                      pParms->dstrect.width,
                                      pParms->dstrect.height,
                                      pParms->psrc->virtptr,
                                      pParms->psrc->virtstride,
                                      pParms->srcrect.left,
                                      pParms->srcrect.top,
                                      pParms->psrc->height);
                  break;

                case TIBLT_FMT_BGR24:
                  // Can't split subsampled pixel pairs
                  if(pParms->dstrect.left & 1)
                  {
                    error = TIBLT_ERR_UNSUPPORTED_DST_RECT;
                    goto Error;
                  }
                  if(pParms->dstrect.width & 1)
                  {
                    error = TIBLT_ERR_UNSUPPORTED_DST_RECT;
                    goto Error;
                  }
                  
                  BlockCopyBGR24toUYVY(pParms->pdst->virtptr,
                                       pParms->pdst->virtstride,
                                       pParms->dstrect.left,
                                       pParms->dstrect.top,
                                       pParms->dstrect.width,
                                       pParms->dstrect.height,
                                       pParms->psrc->virtptr,
                                       pParms->psrc->virtstride,
                                       pParms->srcrect.left,
                                       pParms->srcrect.top);
                  break;
                  
                case TIBLT_FMT_LUT8:
                case TIBLT_FMT_RGB16:
                case TIBLT_FMT_BGRx32:
                case TIBLT_FMT_BGRA32:
                default:
                  error = TIBLT_ERR_UNSUPPORTED_SRC_FORMAT;
                  goto Error;
              }
              break;
              
            case 90:
              switch(pParms->psrc->format)
              {
                case TIBLT_FMT_UYVY:
                  // Can't split subsampled pixel pairs
                  if((pParms->srcrect.left | pParms->dstrect.left) & 1)
                  {
                    error = TIBLT_ERR_UNSUPPORTED_SRC_RECT;
                    goto Error;
                  }
                  if(pParms->dstrect.width & 1)
                  {
                    error = TIBLT_ERR_UNSUPPORTED_DST_RECT;
                    goto Error;
                  }
                  
	              Rotate_UY_90(pParms->pdst->virtptr,
	                           pParms->pdst->virtstride,
	                           pParms->dstrect.left,
	                           pParms->dstrect.top,
	                           pParms->dstrect.width,
	                           pParms->dstrect.height,
	                           pParms->psrc->virtptr,
	                           pParms->psrc->virtstride,
	                           pParms->srcrect.left,
	                           pParms->srcrect.top);
                  break;

				default:
	              error = TIBLT_ERR_UNSUPPORTED_ROTATION;
	              goto Error;
              	}
			  break;
			  
            case 270:
              switch(pParms->psrc->format)
              {
                case TIBLT_FMT_UYVY:
                  // Can't split subsampled pixel pairs
                  if((pParms->srcrect.left | pParms->dstrect.left) & 1)
                  {
                    error = TIBLT_ERR_UNSUPPORTED_SRC_RECT;
                    goto Error;
                  }
                  if(pParms->dstrect.width & 1)
                  {
                    error = TIBLT_ERR_UNSUPPORTED_DST_RECT;
                    goto Error;
                  }
                  
	              Rotate_UY_270(pParms->pdst->virtptr,
	                            pParms->pdst->virtstride,
	                            pParms->dstrect.left,
	                            pParms->dstrect.top,
	                            pParms->dstrect.width,
	                            pParms->dstrect.height,
	                            pParms->psrc->virtptr,
	                            pParms->psrc->virtstride,
	                            pParms->srcrect.left,
	                            pParms->srcrect.top);
                  break;

				default:
	              error = TIBLT_ERR_UNSUPPORTED_ROTATION;
	              goto Error;
              	}
			  break;
				  
            case 180:
              switch(pParms->psrc->format)
              {
                case TIBLT_FMT_UYVY:
                  // Can't split subsampled pixel pairs
                  if((pParms->srcrect.left | pParms->dstrect.left) & 1)
                  {
                    error = TIBLT_ERR_UNSUPPORTED_SRC_RECT;
                    goto Error;
                  }
                  if(pParms->dstrect.width & 1)
                  {
                    error = TIBLT_ERR_UNSUPPORTED_DST_RECT;
                    goto Error;
                  }
                  
	              Rotate_UY_180(pParms->pdst->virtptr,
	                            pParms->pdst->virtstride,
	                            pParms->dstrect.left,
	                            pParms->dstrect.top,
	                            pParms->dstrect.width,
	                            pParms->dstrect.height,
	                            pParms->psrc->virtptr,
	                            pParms->psrc->virtstride,
	                            pParms->srcrect.left,
	                            pParms->srcrect.top);
                  break;

				default:
	              error = TIBLT_ERR_UNSUPPORTED_ROTATION;
	              goto Error;
              	}
			  break;
				  
            default:
              error = TIBLT_ERR_UNSUPPORTED_ROTATION;
              goto Error;
          }
        }
        break;

      default:
        error = TIBLT_ERR_UNSUPPORTED_ROP;
        goto Error;
    }
  }
  else if(pParms->flags & TIBLT_FLAG_BLEND)
  {
    switch(pParms->psrc->format)
    {
      case TIBLT_FMT_RGB16:
        switch(pParms->blendtype)
        {
          case (TIBLT_BLEND_GLOBAL | TIBLT_BLEND_SRCOVER):
          default:
            error = TIBLT_ERR_UNSUPPORTED_BLEND;
            goto Error;
        }
        break;
        
      case TIBLT_FMT_BGRA32:
        switch(pParms->blendtype)
        {
          case (TIBLT_BLEND_SRCLOCAL | TIBLT_BLEND_SRCOVER):
          default:
            error = TIBLT_ERR_UNSUPPORTED_BLEND;
            goto Error;
        }
        break;

      default:
        error = TIBLT_ERR_UNSUPPORTED_COMBINATION;
        break;
    }
  }
  else
  {
    error = TIBLT_ERR_UNSUPPORTED_FLAGS;
    goto Error;
  }

Error:
  return(error);
}

TIBLTERROR TIBLT(TIBLTPARAMS* pParms)
{
  TIBLTERROR error = TIBLT_ERR_UNKNOWN;

  // Check the version of the TIBLTPARAMS structure
  if(pParms->structsize != sizeof(TIBLTPARAMS))
  {
    error = TIBLT_ERR_UNSUPPORTED_BLTPARAMS_VERSION;
    goto Error;
  }

  // Check size of destination surface structure
  if(pParms->pdst->structsize != sizeof(TIBLTSURF))
  {
    error = TIBLT_ERR_UNSUPPORTED_SURF_VERSION;
    goto Error;
  }
  
  // The flags must have either ROP or BLEND set
  if(!(pParms->flags & (TIBLT_FLAG_ROP | TIBLT_FLAG_BLEND)))
  {
    error = TIBLT_ERR_BAD_FLAGS;
    goto Error;
  }

  switch(pParms->pdst->format)
  {
    case TIBLT_FMT_LUT8:
      error = TIBLTFN_toLUT8(pParms);
      break;
      
    case TIBLT_FMT_RGB16:
      error = TIBLTFN_toRGB16(pParms);
      break;
      
    case TIBLT_FMT_BGR24:
      error = TIBLTFN_toBGR24(pParms);
      break;

    case TIBLT_FMT_BGRx32:
      error = TIBLTFN_toBGRx32(pParms);
      break;
      
    case TIBLT_FMT_BGRA32:
      error = TIBLTFN_toBGRA32(pParms);
      break;

    case TIBLT_FMT_BGRA32_NONPREMULT:
      error = TIBLTFN_tonBGRA32(pParms);
      break;
      
    case TIBLT_FMT_UYVY:
      error = TIBLTFN_toUYVY(pParms);
      break;

    case TIBLT_FMT_IYUV:
//      break;
      
    default:
      error = TIBLT_ERR_UNSUPPORTED_DST_FORMAT;
      goto Error;
  }

Error:
  return(error);
}


void UnpremultiplyBGRA32(void*        dstptr,
                      long          dststride,
                      unsigned long dstleft,
                      unsigned long dsttop,
                      unsigned long width,
                      unsigned long height,
                      void const*   srcptr,
                      long          srcstride,
                      unsigned long srcleft,
                      unsigned long srctop)
{
    void* pdstbits = (void*)((long)dstptr +
                             (dstleft * 4) +
                             (dsttop * dststride));
    void* psrcbits = (void*)((long)srcptr +
                             (srcleft * 4) +
                             (srctop * srcstride));

    UnpremultXYZA32toXYZA32(pdstbits,
                            dststride,
                            psrcbits,
                            srcstride,
                            width,
                            height,
                            recip16);
}

