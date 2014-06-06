// -----------------------------------------------------------------------------
//
//  Module Name:    lcdgpe.h
//
// -----------------------------------------------------------------------------

/* lcdgpe.h
 *
 * Class for the primary surface using the LCD Controller
 *
 *
 */

#ifndef __LCDGPE_H__
#define __LCDGPE_H__

#include <lcdc.h>
#include <display.h>

#define GPE_ZONE_PM         DEBUGZONE(15)

#define MAKERGB565(r, g, b) \
    ((WORD)(((BYTE)(r) >> 3) << 11) | (((BYTE)(g) >> 2) << 5) | ((BYTE)(b) >> 3))

#define RGB565_BLACK    MAKERGB565(0x00, 0x00, 0x00)
#define RGB565_WHITE    MAKERGB565(0xFF, 0xFF, 0xFF)
#define RGB565_RED      MAKERGB565(0xFF, 0x00, 0xFF)
#define RGB565_GREEN    MAKERGB565(0x00, 0xFF, 0xFF)
#define RGB565_BLUE     MAKERGB565(0x00, 0x00, 0xFF)


// Every line in the display layer framebuffer must start on a 32 byte boundary
#define VRAM_GRAIN      32

// Maximum valeus for AC bias HSYNC counting registers.
#define ACB_I_MAX       0xf
#define ACB_MAX         0xff

// Delay to wait for framebuffer access mutex when flipping
#define SET_FRAME_BUFFER_TIMEOUT    1000    

// Timeout while waiting for the previous flip to complete
#define WAIT_FOR_FLIP_TIMEOUT       1000

// Timeout while waiting for vsync
#define WAIT_FOR_VSYNC_TIMEOUT      1000

// Timeout while waiting for the LCD to be disabled
#define WAIT_FOR_LCD_DISABLE_TIMEOUT  100

// Timeout while waiting for the palette to load
#define WAIT_FOR_PAL_LOAD_TIMEOUT   100


// This is the main class for controlling the video system
class LCDDDGPE : public DDGPE
{
protected:

	lcdc			m_Lcdc;
    GPEMode         m_ModeInfo;
    DWORD           m_colorDepth;

    // Software Cursor
    BOOL            m_CursorDisabled;
    BOOL            m_CursorVisible;
    BOOL            m_CursorForcedOff;
    RECTL           m_CursorRect;
    POINTL          m_CursorSize;
    POINTL          m_CursorHotspot;

    // allocate enough backing store for a 64x64 cursor on a 32bpp (4 bytes per pixel) screen
    UCHAR           m_CursorBackingStore[64 * 64 * 4];
    UCHAR           m_CursorXorShape[64 * 64];
    UCHAR           m_CursorAndShape[64 * 64];

    SurfaceHeap		*m_pVideoMemoryHeap;     // VRAM heap

    HANDLE          m_hDisplay;

	CEDEVICE_POWER_STATE    m_currentDX;

    // VBlank detection
    HANDLE          m_VSyncEvent;        // Pulsed on every Vsync.
    HANDLE          m_VSyncBufferMutex;     // Protection around next framebuffer address.
    HANDLE          m_VSyncFlipFinished;    // Cleared when waiting for flip, set when finished.

    // Queued framebuffer details
    UINT32          m_Queued_FrameBuffer;
    UINT32          m_Queued_FrameBufferSize;

public:
    // lcdgpe.cpp
                    LCDDDGPE();
    virtual         ~LCDDDGPE();
    virtual SCODE   SetMode(int modeId, HPALETTE * palette);
    virtual SCODE   GetModeInfo(GPEMode * pMode, int modeNumber);
    virtual int     NumModes(void);
    virtual void    GetPhysicalVideoMemory(unsigned long * physicalMemoryBase, unsigned long * videoMemorySize);
    void            GetVirtualVideoMemory(unsigned long * virtualMemoryBase, unsigned long * videoMemorySize);
    BOOL            Init();
    BOOL            LcdMode2GpeMode(INT nModeId, GPEMode *pGpeInfo); 
    BOOL            SetPower(CEDEVICE_POWER_STATE dx);
    DWORD           VSyncISTImpl(LPVOID pParams);
    DWORD WINAPI    Flip(LPDDHAL_FLIPDATA pd);

    UINT32          LcdPlatformSetup( void );
    void            PaletteSetup( void );
    void            LcdSetMode( void );
    void            LcdSetFrameBufferOnVsync( UINT32 FrameBuffer, UINT32 Size );
    void            LcdSetFrameBufferImmediate( UINT32 FrameBuffer, UINT32 Size );
    void            LcdEnable( void );
    void            LcdDisable( void );
    
    // misc.cpp
    virtual ULONG   DrvEscape(SURFOBJ * pso, ULONG iEsc, ULONG cjIn, void * pvIn, ULONG cjOut, void * pvOut);
    virtual void    WaitForNotBusy(void);
    virtual void    WaitForVBlank(void);
    virtual int     IsBusy(void);
    virtual int     InVBlank(void);
    virtual SCODE   SetPalette(const PALETTEENTRY * source, USHORT firstEntry, USHORT numEntries);
    BOOL            AdvertisePowerInterface(void);

    // cursor.cpp
    virtual SCODE   SetPointerShape(GPESurf *pMask, GPESurf *pColorSurface, int xHot, int yHot, int cx, int cy);
    virtual SCODE   MovePointer(int x, int y);
    virtual void    EnableCursor(void);
    virtual void    DisableCursor(void);

    // surf.cpp
    virtual SCODE   AllocSurface(GPESurf ** surface, int width, int height, EGPEFormat format, int surfaceFlags);
    virtual SCODE   AllocSurface(DDGPESurf ** ppSurf, int width, int height, EGPEFormat format, EDDGPEPixelFormat pixelFormat, int surfaceFlags);
    virtual void    SetVisibleSurface(GPESurf * pSurf,  BOOL bWaitForVBlank = FALSE);

    // line.cpp
    virtual SCODE   Line(GPELineParms * lineParameters, EGPEPhase phase);
    SCODE           WrappedEmulatedLine(GPELineParms * lineParameters);

    // blt.cpp
    virtual SCODE   BltPrepare(GPEBltParms * blitParameters);
    virtual SCODE   BltComplete(GPEBltParms * blitParameters);

    // rotate.cpp
    int             GetRotateModeFromReg(void);
    void            SetRotateParams(void);
    long            DynRotate(int angle);

    friend void     buildDDHALInfo(LPDDHALINFO lpddhi, DWORD modeidx);
};



// This class acts as a wrapper around the DDGPESurf class and stores pointers 
// to the VRAM heap and LCDDDGPE class.  The destructor releases the heap VRAM
// allocated.  It is called when DirectX destroys the surface.
class LCDDDGPESurf : public DDGPESurf
{
private:
    SurfaceHeap * m_pHeap;
    int           m_nAlignedWidth;
    LCDDDGPE      * m_pLCDDDGPE;

public:
    LCDDDGPESurf(int width, int alignedWidth, int height, ULONG offset, PVOID pBits, int stride,
            EGPEFormat format, EDDGPEPixelFormat pixelFormat,
            SurfaceHeap *pHeap, LCDDDGPE *pLCDDDGPE);

    LCDDDGPESurf(int width, int alignedWidth, int height, int stride,
            EGPEFormat format, EDDGPEPixelFormat pixelFormat);

    virtual DWORD    AlignedWidth (void) { return (DWORD) m_nAlignedWidth; }
    virtual         ~LCDDDGPESurf(void);
};


#endif __LCDGPE_H__

// <end of file>

