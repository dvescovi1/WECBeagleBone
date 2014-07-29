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

#ifndef __DSSAI_H__
#define __DSSAI_H__

#include "soc_cfg.h"
#include <isp_resizer.h>

//------------------------------------------------------------------------------
//  Defines/Enums

#define NUM_DSS_PIPELINES       3

typedef enum OMAP_DSS_PIPELINE {
    OMAP_DSS_PIPELINE_GFX,
    OMAP_DSS_PIPELINE_VIDEO1,
    OMAP_DSS_PIPELINE_VIDEO2,
    OMAP_DSS_PIPELINE_MAX
} OMAP_DSS_PIPELINE;


#define NUM_DSS_DESTINATION     1

typedef enum OMAP_DSS_DESTINATION {
    OMAP_DSS_DESTINATION_LCD
} OMAP_DSS_DESTINATION;


typedef enum OMAP_DSS_PIXELFORMAT {
    OMAP_DSS_PIXELFORMAT_UNKNOWN    = -1,
    OMAP_DSS_PIXELFORMAT_RGB16      = DISPC_PIXELFORMAT_RGB16,
    OMAP_DSS_PIXELFORMAT_RGB32      = DISPC_PIXELFORMAT_RGB32,
    OMAP_DSS_PIXELFORMAT_ARGB16     = DISPC_PIXELFORMAT_ARGB16,
    OMAP_DSS_PIXELFORMAT_RGB24   	= DISPC_PIXELFORMAT_RGB24,
    OMAP_DSS_PIXELFORMAT_ARGB32     = DISPC_PIXELFORMAT_ARGB32,
    OMAP_DSS_PIXELFORMAT_RGBA32     = DISPC_PIXELFORMAT_RGBA32,
    OMAP_DSS_PIXELFORMAT_YUV2       = DISPC_PIXELFORMAT_YUV2,
    OMAP_DSS_PIXELFORMAT_UYVY       = DISPC_PIXELFORMAT_UYVY
} OMAP_DSS_PIXELFORMAT;


typedef enum OMAP_DSS_ROTATION {
    OMAP_DSS_ROTATION_0             = 0,
    OMAP_DSS_ROTATION_90            = 90,
    OMAP_DSS_ROTATION_180           = 180,
    OMAP_DSS_ROTATION_270           = 270
} OMAP_DSS_ROTATION;


typedef enum OMAP_DSS_COLORKEY {
    OMAP_DSS_COLORKEY_TRANS_SOURCE,         // Video source transparent color key
    OMAP_DSS_COLORKEY_TRANS_DEST,           // Graphics destination transparent color key
    OMAP_DSS_COLORKEY_GLOBAL_ALPHA_GFX,     // Global alpha value for graphics pipeline        
    OMAP_DSS_COLORKEY_GLOBAL_ALPHA_VIDEO2   // Global alpha value for video2 pipeline        
} OMAP_DSS_COLORKEY;



//
//  Video Encoder programming values for NTSC 601 and PAL 601 (Table 15-67 OMAP35XX TRM)
//  The VENC values for NTSC and PAL are held in an array of DWORDs indexed by the
//  OMAP_VENC_REGISTERS enum
//

typedef enum OMAP_VENC_REGISTERS {
    VENC_F_CONTROL,
    VENC_VIDOUT_CTRL,
    VENC_SYNC_CTRL,
    VENC_LLEN,
    VENC_FLENS,
    VENC_HFLTR_CTRL,
    VENC_CC_CARR_WSS_CARR,
    VENC_C_PHASE,
    VENC_GAIN_U,
    VENC_GAIN_V,
    VENC_GAIN_Y,
    VENC_BLACK_LEVEL,
    VENC_BLANK_LEVEL,
    VENC_X_COLOR,
    VENC_M_CONTROL,
    VENC_BSTAMP_WSS_DATA,
    VENC_S_CARR,
    VENC_LINE21,
    VENC_LN_SEL,
    VENC_L21__WC_CTL,
    VENC_HTRIGGER_VTRIGGER,
    VENC_SAVID_EAVID,
    VENC_FLEN_FAL,
    VENC_LAL_PHASE_RESET,
    VENC_HS_INT_START_STOP_X,
    VENC_HS_EXT_START_STOP_X,
    VENC_VS_INT_START_X,
    VENC_VS_INT_STOP_X__VS_INT_START_Y,
    VENC_VS_INT_STOP_Y__VS_EXT_START_X,
    VENC_VS_EXT_STOP_X__VS_EXT_START_Y,
    VENC_VS_EXT_STOP_Y,
    VENC_AVID_START_STOP_X,
    VENC_AVID_START_STOP_Y,
    VENC_FID_INT_START_X__FID_INT_START_Y,
    VENC_FID_INT_OFFSET_Y__FID_EXT_START_X,
    VENC_FID_EXT_START_Y__FID_EXT_OFFSET_Y,
    VENC_TVDETGP_INT_START_STOP_X,
    VENC_TVDETGP_INT_START_STOP_Y,
    VENC_GEN_CTRL,
    VENC_OUTPUT_CONTROL,
    VENC_OUTPUT_TEST,
    NUM_VENC_REGISTERS
} OMAP_VENC_REGISTERS;


//
//  Surface orientation is relative to the LCD position
//

typedef enum OMAP_SURF_ORIENTATION {
    OMAP_SURF_ORIENTATION_STANDARD,
    OMAP_SURF_ORIENTATION_ROTATED
} OMAP_SURF_ORIENTATION;


//
// Surface Type
//
typedef enum OMAP_SURFACE_TYPE {
    OMAP_SURFACE_NORMAL,        // one that is sent to app and input to resizer if needed
    OMAP_SURFACE_RESIZER        // one that is output of resizer
} OMAP_SURFACE_TYPE;

//
// Associated Surface Behavior Setting
//
typedef enum OMAP_ASSOC_SURF_USAGE {
    OMAP_ASSOC_SURF_DEFAULT,        // default action is based on flag ex: useResizer
    OMAP_ASSOC_SURF_FORCE_OFF,      // Dont use Assoc surface even if available
    OMAP_ASSOC_SURF_FORCE_ON        // use Assoc Surface if available
} OMAP_ASSOC_SURF_USAGE;



//
//  DSS Clock sources
//
typedef enum OMAP_DSS_FCLK {
    OMAP_DSS_FCLK_DSS1ALWON = 0,
    OMAP_DSS_FCLK_DSS2ALWON
} OMAP_DSS_FCLK;

//
//  DSI FCLK values
//
typedef enum OMAP_DSS_FCLKVALUE {
    OMAP_DSS_FCLKVALUE_NORMAL = 172000000,
    OMAP_DSS_FCLKVALUE_HDMI = 148500000,
    OMAP_DSS_FCLKVALUE_LPR = 96000000
} OMAP_DSS_FCLKVALUE;


//
//  Globals
//

//  YUV-RGB Color Conversion Tables

#define NUM_COLORSPACE_COEFFS   5

extern DWORD    g_dwColorSpaceCoeff_BT601_Limited[NUM_COLORSPACE_COEFFS];
extern DWORD    g_dwColorSpaceCoeff_BT601_Full[NUM_COLORSPACE_COEFFS];
extern DWORD    g_dwColorSpaceCoeff_BT709_Limited[NUM_COLORSPACE_COEFFS];
extern DWORD    g_dwColorSpaceCoeff_BT709_Full[NUM_COLORSPACE_COEFFS];

extern DWORD    g_dwVencValues_NTSC[NUM_VENC_REGISTERS];
extern DWORD    g_dwVencValues_PAL[NUM_VENC_REGISTERS];


//  Up/Down Scaling Coefficients

#define NUM_SCALING_COEFFS      24
#define NUM_SCALING_PHASES      8

extern DWORD    g_dwScalingCoeff_Horiz_Up[NUM_SCALING_COEFFS];
extern DWORD    g_dwScalingCoeff_Horiz_Down[NUM_SCALING_COEFFS];
extern DWORD    g_dwScalingCoeff_Vert_Up_3_Taps[NUM_SCALING_COEFFS];
extern DWORD    g_dwScalingCoeff_Vert_Up_5_Taps[NUM_SCALING_COEFFS];
extern DWORD    g_dwScalingCoeff_Vert_Down_3_Taps[NUM_SCALING_COEFFS];
extern DWORD    g_dwScalingCoeff_Vert_Down_5_Taps[NUM_SCALING_COEFFS];

//***** 3 taps *****
extern BYTE g_coef3_M8[5][8];
extern BYTE g_coef3_M16[5][8];
extern BYTE g_coef3_M11[5][8];

//***** 5 taps *****
extern BYTE g_coef_M8[5][8];
extern BYTE g_coef_M9[5][8];
extern BYTE g_coef_M10[5][8];
extern BYTE g_coef_M11[5][8];
extern BYTE g_coef_M12[5][8];
extern BYTE g_coef_M13[5][8];
extern BYTE g_coef_M14[5][8];
extern BYTE g_coef_M16[5][8];
extern BYTE g_coef_M19[5][8];
extern BYTE g_coef_M22[5][8];
extern BYTE g_coef_M26[5][8];
extern BYTE g_coef_M32[5][8];

//  Contrast Gamma Tables

#define NUM_CONTRAST_LEVELS     7
#define DEFAULT_CONTRAST_LEVEL  3
#define NUM_GAMMA_VALS          256

extern DWORD    g_dwGammaTable[NUM_CONTRAST_LEVELS][NUM_GAMMA_VALS];


//  Display Timeout

#define DISPLAY_TIMEOUT         30

#define VSYNC_EVENT_NAME    TEXT("DSSAI_VSYNC_EVENT")
#define VSYNC_EVENT_WSEGL    	TEXT("DSSAI_VSYNC_EVENT_WSEGL")
//
//  Class declarations
//

class OMAPDisplayController;
class OMAPSurfaceManager;
class OMAPSurface;
class OMAPFlatSurface;


//------------------------------------------------------------------------------
//
//  Class:  OMAPDisplayController
//
//  Class to control OMAP display controller
//
class OMAPDisplayController
{
public:
    //------------------------------------------------------------------------------
    //
    //  Method: constructor
    //
    OMAPDisplayController();

    //------------------------------------------------------------------------------
    //
    //  Method: destructor
    //
    virtual
    ~OMAPDisplayController();


    //------------------------------------------------------------------------------
    //
    //  Method: Initialization
    //
    //  Calls to initialize subsystems of the display controller
    //
    BOOL    InitController(BOOL bEnableGammaCorr, BOOL bEnableWaitForVerticalBlank, BOOL bEnableISPResizer);
    BOOL    InitLCD();
    BOOL    InitInterrupts(DWORD irq, DWORD istPriority);
    VOID    UninitInterrupts();
    BOOL    SetSurfaceMgr( OMAPSurfaceManager *pSurfMgr );
    
    //------------------------------------------------------------------------------
    //
    //  Method: Interrupt handling
    //
    //  Calls to handle the interrupts from the display controller
    //
    VOID    DssProcessInterrupt();
    static DWORD   DssInterruptHandler( void* pData );
    //------------------------------------------------------------------------------
    //
    //  Properties: LCD Pixel Format
    //              LCD Width and Height
    //              TV Width and Height
    //
    OMAP_DSS_PIXELFORMAT    GetLCDPixelFormat() { return m_eLcdPixelFormat; }
    DWORD                   GetLCDWidth() { return m_dwLcdWidth; }
    DWORD                   GetLCDHeight() { return m_dwLcdHeight; }


    //------------------------------------------------------------------------------
    //
    //  Method: SetPipelineAttribs
    //
    //  Sets the attributes for a selected DSS pipeline
    //
    BOOL    SetPipelineAttribs(
                OMAP_DSS_PIPELINE       ePipeline,
                OMAP_DSS_DESTINATION    eDestination,
                OMAPSurface*            pSurface,
                DWORD                   dwPosX = 0,
                DWORD                   dwPosY = 0
                );

    //------------------------------------------------------------------------------
    //
    //  Method: SetScalingAttribs
    //
    //  Sets the scaling attributes for a selected DSS pipeline (VID1 or VID2 only)
    //  Given width and height are the size to scale the pipeline's surface output to.
    //  The Src and Dest RECTs are used to perform scaling and cliping of the overlay
    //  surface
    //
    BOOL    SetScalingAttribs(
                OMAP_DSS_PIPELINE       ePipeline,
                DWORD                   dwWidth,
                DWORD                   dwHeight,
                DWORD                   dwPosX = 0,
                DWORD                   dwPosY = 0
                );

    BOOL    SetScalingAttribs(
                OMAP_DSS_PIPELINE       ePipeline,
                RECT*                   pSrcRect,
                RECT*                   pDestRect
                );

    //------------------------------------------------------------------------------
    //
    //  Method: EnablePipeline
    //          DisablePipeline
    //
    //  Enables/disables a selected DSS pipeline. Pipeline attributes need to be 
    //  configured before enabling.
    //
    BOOL    EnablePipeline(
                OMAP_DSS_PIPELINE       ePipeline
                );            

    BOOL    DisablePipeline(
                OMAP_DSS_PIPELINE       ePipeline
                );

    //------------------------------------------------------------------------------
    //
    //  Method: FlipPipeline
    //          MovePipeline
    //          RotatePipeline
    //          MirrorPipeline
    //          UpdateScalingAttribs
    //
    //  Enables various output operations for a selected DSS pipeline.
    //
    //      Flip -      Changes pipeline output to display given surface (backbuffer flipping)
    //      IsPipelineFlipping - Queries if the pipeline is flipping to the given surface
    //      Move -      Updates position of pipeline output with respect to LCD origin
    //      Rotate -    Configures pipeline to handle rotation of surfaces
    //      Mirror -    Configures pipeline to handle mirorring (horizontal) of surfaces
    //      Scaling -   Changes the pipeline scaling attributes
    //
    BOOL    FlipPipeline(
                OMAP_DSS_PIPELINE       ePipeline,
                OMAPSurface*            pSurface
                );            

    BOOL    IsPipelineFlipping(
                OMAP_DSS_PIPELINE       ePipeline,
                OMAPSurface*            pSurface,
                BOOL                    matchExactSurface = FALSE
                );

    BOOL    MovePipeline(
                OMAP_DSS_PIPELINE       ePipeline,
                LONG                    lXPos,
                LONG                    lYPos
                );

    BOOL    RotatePipeline(
                OMAP_DSS_PIPELINE       ePipeline,
                OMAP_DSS_ROTATION       eRotation
                );            

    BOOL    MirrorPipeline(
                OMAP_DSS_PIPELINE       ePipeline,
                BOOL                    bMirror
                );            
                
    BOOL    UpdateScalingAttribs(
                OMAP_DSS_PIPELINE       ePipeline,
                RECT*                   pSrcRect,
                RECT*                   pDestRect
                );

    //------------------------------------------------------------------------------
    //
    //  Method: EnableColorKey
    //          DisableColorKey
    //
    //  Enables/disables a selected color key.  Color key are a tied to LCD and TV out
    //  channels and not to invidual pipelines.  Enabling a color key will enable will set
    //  the color for both LCD and TV out (overall enabling of LCD and/or TV out drives
    //  actual use of key).  
    //
    //      Transparent color keys are orthogonal to alpha global values (enabling one disables the other)
    //      Transparent source and destination values are orthogonal (enabling one disables the other)
    //
    BOOL    EnableColorKey(
                OMAP_DSS_COLORKEY       eColorKey,
                OMAP_DSS_DESTINATION    eDestination,
                DWORD                   dwColor = 0
                );            

    BOOL    DisableColorKey(
                OMAP_DSS_COLORKEY       eColorKey,
                OMAP_DSS_DESTINATION    eDestination
                );


    //------------------------------------------------------------------------------
    //
    //  Method: GetContrastLevel
    //          SetContrastLevel
    //
    //  Gets/sets contrast level using gamma correction tables.
    //

    DWORD   GetContrastLevel() { return m_dwContrastLevel; }
    BOOL    SetContrastLevel(
                DWORD dwContrastLevel
                );


    //------------------------------------------------------------------------------
    //
    //  Method: GetPowerLevel
    //          SetPowerLevel
    //          SaveRegisters
    //          RestoreRegisters
    //
    //  Saves off and restores DSS registers for OFF mode handling.
    //  Sets power level for display
    //
    BOOL    SaveRegisters(
                OMAP_DSS_DESTINATION eDestination
                );

    BOOL    RestoreRegisters(
                OMAP_DSS_DESTINATION eDestination
                );

    DWORD   GetPowerLevel() { return m_dwPowerLevel; }
    BOOL    SetPowerLevel(
                DWORD dwPowerLevel
                );


    //------------------------------------------------------------------------------
    //
    //  Method: PixelFormatToPixelSize
    //
    //  Various global utility functions
    //
    static
    DWORD   PixelFormatToPixelSize(
                OMAP_DSS_PIXELFORMAT    ePixelFormat
                );    

    //------------------------------------------------------------------------------
    //
    //  Method: EnableVSyncInterrupt
    //          DisableVSyncInterrupt
    //          InVSync
    //          WaitForVsync
    //
    //  Operations for enabling, disabling and waiting for VSYNC interrupts
    //
    void    EnableVSyncInterrupt();
    BOOL    EnableVSyncInterruptEx();
    VOID    DisableVSyncInterrupt();
    BOOL    InVSync(BOOL bClearStatus);
    VOID WaitForVsync();


    //------------------------------------------------------------------------------
    //
    //  Method: EnableScanLineInterrupt
    //          DisableScanLineInterrupt
    //          GetScanLine
    //          WaitForScanLine
    //
    //  Operations for enabling, disabling and waiting for scan line interrupt
    //
    VOID    EnableScanLineInterrupt(DWORD dwLineNumber);
    VOID    DisableScanLineInterrupt();
    DWORD   GetScanLine();
    VOID    WaitForScanLine(DWORD dwLineNumber);



    //------------------------------------------------------------------------------
    //
    //  Method: EnableLPR
    //
    //  Enables/Disables the LowPowerRefresh capability of DSS based on bEnable
    //
    VOID EnableLPR(
             BOOL bEnable,
             BOOL bHdmiEnable = FALSE
             );

    //------------------------------------------------------------------------------
    //
    //  Method: EnableOverlayOptimization
    //
    //  Enables/Disables the overlay optimization capability of DSS based on bEnable
    //
    VOID EnableOverlayOptimization(
             BOOL bEnable
             );

    //------------------------------------------------------------------------------
    //
    //  Method: ConfigureDsiPll
    //
    //  This function configures the DSI Pll for the given clock frequency
    //
    BOOL ConfigureDsiPll(ULONG clockFreq);

    //------------------------------------------------------------------------------
    //
    //  Method: GetDssInfo
    //
    //  This function returns information filled up by the DSS driver
    //
	DSS_INFO* GetDssInfo() { return &m_dssinfo;};

protected:
    //------------------------------------------------------------------------------
    //
    //  Method: 
    //
    //
    BOOL    ResetDSS(); 
    BOOL    ResetDISPC(); 
    BOOL    ResetVENC(); 

    BOOL    AccessRegs();
    BOOL    ReleaseRegs();

    BOOL    FlushRegs(
                DWORD   dwDestGo
                );
    
    BOOL    RequestClock(
                DWORD   dwClock
                );

    BOOL    ReleaseClock(
                DWORD   dwClock
                );

    BOOL    WaitForFrameDone(
                DWORD dwTimeout = DISPLAY_TIMEOUT
                );

    BOOL    WaitForIRQ(
                DWORD dwIRQ,
                DWORD dwTimeout = DISPLAY_TIMEOUT
                );

    BOOL    SwitchDssFclk(
                OMAP_DSS_FCLK eFclkSrc, 
                OMAP_DSS_FCLKVALUE eFclkValue
                );
                
    BOOL    SelectDSSSourceClocks(
        ULONG count, 
        UINT pClockSrc[]
    );

    BOOL    WaitForFlushDone(
                DWORD dwDestGo
                );

    BOOL    InitDsiPll();
    
    BOOL    DeInitDsiPll();
    
    BOOL    SetDssFclk(
                OMAP_DSS_FCLK      eDssFclkSource,
                OMAP_DSS_FCLKVALUE eDssFclkValue
                );
    
private:
    //  OMAP Display Subsystem Registers
    OMAP_DSS_REGS         *m_pDSSRegs;
    OMAP_DISPC_REGS       *m_pDispRegs;
    OMAP_VENC_REGS        *m_pVencRegs;
    OMAP_DSI_REGS           *m_pDSIRegs;
    OMAP_DSI_PLL_REGS       *m_pDSIPLLRegs;

    //  Register Context Save/Restore globals
    OMAP_DISPC_REGS         g_rgDisplaySaveRestore;
    OMAP_DISPC_REGS         g_rgDisplaySaveRestore_eLcd;

    DWORD                   m_dwPowerLevel;
    CRITICAL_SECTION        m_csPowerLock;

    DWORD                   *m_pColorSpaceCoeffs;

    //  LCD and TV parameters
    OMAP_DSS_PIXELFORMAT    m_eLcdPixelFormat;
    DWORD                   m_dwLcdWidth;
    DWORD                   m_dwLcdHeight;
    DWORD                   m_dwPixelClock;

    DWORD                   m_dwContrastLevel;
    DWORD                   *m_pGammaBufVirt;
    DWORD                   m_dwGammaBufPhys;
    
    // Gamma Correction parameter
    BOOL                       m_bGammaEnable;

    //  Surface Manager reference
    OMAPSurfaceManager      *m_pSurfaceMgr;
    HANDLE                  m_hDssIntEvent;
    HANDLE                  m_hDssIntThread;
    DWORD                  m_dwDssSysIntr;
    BOOL                     m_bDssIntThreadExit;

    HANDLE                  m_hVsyncEvent;
    HANDLE                  m_hVsyncEventSGX;
	HANDLE                  m_hVsyncEventWSEGL;
    DWORD                   m_dwVsyncPeriod;

    DWORD                   m_dwEnableWaitForVerticalBlank;
    HANDLE                  m_hScanLineEvent;

    // DSS FCLK Configuration Parameter
    OMAP_DSS_FCLK           m_eDssFclkSource;
    OMAP_DSS_FCLKVALUE      m_eDssFclkValue;
    BOOL                    m_bLPREnable;

    //Handle to the SmartReflex Power Policy Adapter 
    HANDLE                  m_hSmartReflexPolicyAdapter;

    DSS_INFO    m_dssinfo;

    
    DWORD                   m_lastVsyncIRQStatus;

};






//------------------------------------------------------------------------------
//
//  Class:  OMAPSurface
//
//  Class to manage a generic display surface
//
class OMAPSurface
{
public:
    //------------------------------------------------------------------------------
    //
    //  Method: constructor
    //
    OMAPSurface() {};

    //------------------------------------------------------------------------------
    //
    //  Method: destructor
    //
    virtual
    ~OMAPSurface() {};


    //------------------------------------------------------------------------------
    //
    //  Properties:
    //
    //  Various properties of the surface:
    //
    //      PixelFormat -       Pixel type of the surface
    //      PixelSize -         Size in bytes of a single pixel
    //      Orientation -       Current orientation of surface relative to LCD
    //      VirtualAddr -       Ptr to surface memory
    //

    OMAP_DSS_PIXELFORMAT    PixelFormat() { return m_ePixelFormat; }
    
    DWORD                   PixelSize() { return m_dwPixelSize; }

    OMAP_SURF_ORIENTATION   Orientation() { return m_eOrientation; }

    DWORD                   HorizontalScaling() { return m_dwHorizScale; }
    
    DWORD                   VerticalScaling() { return m_dwVertScale; }

    RECT*                   Clipping(){ return &m_rcClip; }

    RSZParams_t *           ResizeParams() { return &m_sRSZParams; }
    
    VOID                    SetRSZParams(RSZParams_t rszParams) { m_sRSZParams = rszParams; }
    
    OMAPSurface *           OmapAssocSurface(){return m_pAssocSurface;}    
    
    HANDLE                  GetRSZHandle(BOOL alloc);
    
    VOID                    SetRSZHandle(HANDLE rszHandle, BOOL freeHandle);
    
    BOOL                    SetSurfaceType(OMAP_SURFACE_TYPE eSurfaceType);
  
    BOOL                    SetAssocSurface(OMAPSurface* pAssocSurface);
      
    BOOL                    UseResizer(BOOL bUseResizer );    
    
    BOOL                    isResizerEnabled();

    virtual
    VOID*                   VirtualAddr() = 0;
        
    //------------------------------------------------------------------------------
    //
    //  DMA Properties:
    //
    //  These properties of the surface are relative to the current orientation of
    //  the suface as well as the given rotation angle requested.  These properties
    //  are used for configuring DSS DMA output
    //

    virtual
    DWORD   Width(
                OMAP_DSS_ROTATION   eRotation = OMAP_DSS_ROTATION_0 ) = 0;

    virtual
    DWORD   Height(
                OMAP_DSS_ROTATION   eRotation = OMAP_DSS_ROTATION_0 ) = 0;

    virtual
    DWORD   Stride(
                OMAP_DSS_ROTATION   eRotation = OMAP_DSS_ROTATION_0 ) = 0;

    virtual
    DWORD   PhysicalAddr(
                OMAP_DSS_ROTATION   eRotation = OMAP_DSS_ROTATION_0,
                BOOL                bMirror = FALSE,
                OMAP_ASSOC_SURF_USAGE eUseAssocSurface = OMAP_ASSOC_SURF_DEFAULT) = 0;   
    
    virtual
    DWORD   PixelIncr(
                OMAP_DSS_ROTATION   eRotation = OMAP_DSS_ROTATION_0,
                BOOL                bMirror = FALSE ) = 0;
                                
    virtual
    DWORD   RowIncr(
                OMAP_DSS_ROTATION   eRotation = OMAP_DSS_ROTATION_0,
                BOOL                bMirror = FALSE ) = 0;
                                

    //------------------------------------------------------------------------------
    //
    //  Method: SetClipping
    //
    //  Sets the clipping region of the surface.  By default, the clipping region
    //  is set to the size of the surface.  This setting allows for a smaller portion
    //  of the surface to be displayed.  The clipping region configures the DMA
    //  properties of the surface such that only the area of the surface bound
    //  by the clipping region is displayed.  Passing in a NULL pointer for the
    //  clipping region resets the region to be the size of the surface.
    //
    virtual
    BOOL    SetClipping(
                RECT*               pClipRect
                );

    //------------------------------------------------------------------------------
    //
    //  Method: UpdateClipping
    //
    //  Adjusts clipping rectangle based on current scaling and orientation settings
    //  of the surface.
    virtual
    BOOL   UpdateClipping(
                RECT* pClipRect
                );

    //------------------------------------------------------------------------------
    //
    //  Method: GetClipping
    //
    //  Gets the current clipping region of the surface.
    virtual
    RECT    GetClipping();

    //------------------------------------------------------------------------------
    //
    //  Method: AdjustClippingRect
    //
    //  Adjusts clipping rectangle coordinates and resulting width/height to be a multiple
    //  of the horzValue and vertValue. The resulting clipping rectangle is contained within the
    //  original given rectangle.
    virtual
    BOOL    AdjustClippingRect(
        RECT *srcRect,
        UINT8 horzValue,
        UINT8 vertValue
        );



    //------------------------------------------------------------------------------
    //
    //  Method: SetHorizontalScaling
    //          SetVerticalScaling
    //
    //  Sets the scaling factor to decimate the surface by 1, 1/2, 1/4 or 1/8.  These
    //  settings configure the DMA properties of the surface to divide the horizontal
    //  and/or vertical size of the surface to be smaller than it truely is.  This
    //  technique (DMA decimation) is used for scaling down very large surfaces as well
    //  as for configuring interlaced TV output.  Valid values are 1, 2, 4, and 8.
    //
    virtual
    BOOL    SetHorizontalScaling(
                DWORD               dwScaleFactor
                );

    virtual
    BOOL    SetVerticalScaling(
                DWORD               dwScaleFactor
                );

    //------------------------------------------------------------------------------
    //
    //  Method: SetOrientation
    //
    //  Sets the orientation of the surface relative to its original creation
    //  width and height.  This will cause a reprogramming of the VRFB tile (page)
    //  and size registers in order to swap surface width and height.
    //
    virtual
    BOOL    SetOrientation(
                OMAP_SURF_ORIENTATION       eOrientation
                ) = 0;
 
  
 protected:
    //  Generic surface properties  
    
    OMAP_DSS_PIXELFORMAT        m_ePixelFormat;
    DWORD                       m_dwPixelSize;
    OMAP_SURF_ORIENTATION       m_eOrientation;
    DWORD                       m_dwWidth;
    DWORD                       m_dwHeight;
    RECT                        m_rcClip;
    DWORD                       m_dwHorizScale;
    DWORD                       m_dwVertScale;
    BOOL                        m_bUseResizer;
    HANDLE                      m_hRSZHandle;
    RSZParams_t                 m_sRSZParams;    
    OMAP_SURFACE_TYPE           m_eSurfaceType;
    OMAPSurface *               m_pAssocSurface;    
};




//------------------------------------------------------------------------------
//
//  Class:  OMAPSurfaceManager
//
//  Class to manage the creation of OMAP display surfaces
//
class OMAPSurfaceManager
{
public:
    //------------------------------------------------------------------------------
    //
    //  Method: constructor
    //
    OMAPSurfaceManager() {};

    //------------------------------------------------------------------------------
    //
    //  Method: destructor
    //
    virtual
    ~OMAPSurfaceManager() {};
 
 
    //------------------------------------------------------------------------------
    //
    //  Method: Initialize
    //
    //  Initializes the display surface manager.  The dwOffscreenMemory allocates
    //  additional physical memory to support offscreen surface DMA BLT operations.
    //  A value of 0 for offscreen surfaces disables the feature.
    //
    virtual
    BOOL    Initialize(
                DWORD   dwOffscreenMemory = 0 ) = 0;
   

    //------------------------------------------------------------------------------
    //
    //  Properties:
    //
    //  Various properties of the surface:
    //
    //      TotalMemorySize -       Total amount of virtual display memory
    //      FreeMemorySize -        Total amount of free virtual display memory
    //      VirtualBaseAddr -       Base virtual address of display memory
    //      NumPhysicalAddr -       Number of discontinuous phys display memory segments
    //      PhysicalLen -           Length of physical memory segment n
    //      PhysicalAddr -          Address of physical memory segment n
    //      SupportsRotation -      Returns TRUE if rotation is supported
    //
    virtual
    DWORD       TotalMemorySize() = 0;

    virtual
    DWORD       FreeMemorySize() = 0;

    virtual
    VOID*       VirtualBaseAddr() = 0;

    virtual
    DWORD       NumPhysicalAddr() = 0;

    virtual
    DWORD       PhysicalLen(DWORD dwIndex) = 0;

    virtual
    DWORD       PhysicalAddr(DWORD dwIndex) = 0;

    virtual
    BOOL        SupportsRotation() = 0;

    virtual
    BOOL        SupportsOffscreenSurfaces() = 0;


    //------------------------------------------------------------------------------
    //
    //  Method: Allocate
    //
    //  Allocates an OMAP Surface object
    //
    virtual
    BOOL    Allocate(
                OMAP_DSS_PIXELFORMAT    ePixelFormat,
                DWORD                   dwWidth,
                DWORD                   dwHeight,
                OMAPSurface**           ppSurface
                ) = 0;
    
    virtual
    BOOL    Allocate(
                OMAP_DSS_PIXELFORMAT    ePixelFormat,
                DWORD                   dwWidth,
                DWORD                   dwHeight,
                OMAPSurface**           ppAssocSurface,
                OMAPSurface*            pSurface
                ) = 0;

    virtual
    BOOL    AllocateGDI(
                OMAP_DSS_PIXELFORMAT    ePixelFormat,
                DWORD                   dwWidth,
                DWORD                   dwHeight,
                OMAPSurface**           ppSurface
                ) = 0;

};


//------------------------------------------------------------------------------
//
//  Class:  OMAPFlatSurface
//
//  Class to manage a display surface that that is on flat physical memory
//  Derived from OMAPSurface
//
class OMAPFlatSurface : public OMAPSurface
{
public:
    //------------------------------------------------------------------------------
    //
    //  Method: constructor
    //
    OMAPFlatSurface();

    //------------------------------------------------------------------------------
    //
    //  Method: destructor
    //
    virtual
    ~OMAPFlatSurface();


    //------------------------------------------------------------------------------
    //
    //  Method: Allocate
    //
    //  Allocates a surface for with the given parameters
    //
    BOOL    Allocate(
                OMAP_DSS_PIXELFORMAT    ePixelFormat,
                DWORD                   dwWidth,
                DWORD                   dwHeight,
                HANDLE                  hHeap,
                DWORD                   dwBasePhysicalAddr );

    //------------------------------------------------------------------------------
    //
    //  Properties:
    //
    virtual
    VOID*   VirtualAddr();

    //------------------------------------------------------------------------------
    //
    //  DMA Properties:
    //
    //  These properties of the surface are relative to the current orientation of
    //  the suface as well as the given rotation angle requested.  These properties
    //  are used for configuring DSS DMA output
    //

    virtual
    DWORD   Width(
                OMAP_DSS_ROTATION   eRotation );

    virtual
    DWORD   Height(
                OMAP_DSS_ROTATION   eRotation );

    virtual
    DWORD   Stride(
                OMAP_DSS_ROTATION   eRotation = OMAP_DSS_ROTATION_0 );

    virtual
    DWORD   PhysicalAddr(
                OMAP_DSS_ROTATION   eRotation = OMAP_DSS_ROTATION_0,
                BOOL                bMirror = FALSE,
                OMAP_ASSOC_SURF_USAGE eUseAssocSurface = OMAP_ASSOC_SURF_DEFAULT );
    
    virtual
    DWORD   PixelIncr(
                OMAP_DSS_ROTATION   eRotation = OMAP_DSS_ROTATION_0,
                BOOL                bMirror = FALSE );
                                
    virtual
    DWORD   RowIncr(
                OMAP_DSS_ROTATION   eRotation = OMAP_DSS_ROTATION_0,
                BOOL                bMirror = FALSE );
                                

    //------------------------------------------------------------------------------
    //
    //  Method: SetOrientation
    //
    //  Sets the orientation of the surface relative to its original creation
    //  width and height.  This will cause a reprogramming of the VRFB tile (page)
    //  and size registers in order to swap surface widht and height.
    //
    virtual
    BOOL    SetOrientation(
                OMAP_SURF_ORIENTATION       eOrientation
                );
  

protected:
    //  Flat memory surface properties
    HANDLE                  m_hHeap;    
    DWORD                   m_dwPhysicalAddr;    
    DWORD                   m_dwActualWidth;
    DWORD                   m_dwActualHeight;
    DWORD                   m_dwWidthFactor;    
};

//------------------------------------------------------------------------------
//
//  Class:  OMAPFlatSurfaceManager
//
//  Class to manage the creation of OMAP display surfaces on flat memory
//  Derived from OMAPSurfaceManager
//
class OMAPFlatSurfaceManager : public OMAPSurfaceManager
{
public:
    //------------------------------------------------------------------------------
    //
    //  Method: constructor
    //
    OMAPFlatSurfaceManager();

    //------------------------------------------------------------------------------
    //
    //  Method: destructor
    //
    virtual
    ~OMAPFlatSurfaceManager();
 
 
    //------------------------------------------------------------------------------
    //
    //  Method: Initialize
    //
    //  Initializes the display surface manager.  The dwOffscreenMemory allocates
    //  additional physical memory to support offscreen surface DMA BLT operations.
    //  A value of 0 for offscreen surfaces disables the feature.
    //
    virtual
    BOOL    Initialize(
                DWORD   dwOffscreenMemory = 0 );
   

    //------------------------------------------------------------------------------
    //
    //  Properties:
    //
    //  Various properties of the surface:
    //
    //      TotalMemorySize -       Total amount of virtual display memory
    //      FreeMemorySize -        Total amount of free virtual display memory
    //      VirtualBaseAddr -       Base virtual address of display memory
    //      NumPhysicalAddr -       Number of discontinuous phys display memory segments
    //      PhysicalLen -           Length of physical memory segment n
    //      PhysicalAddr -          Address of physical memory segment n
    //      SupportsRotation -      Returns TRUE if rotation is supported
    //
    virtual
    DWORD       TotalMemorySize();

    virtual
    DWORD       FreeMemorySize();

    virtual
    VOID*       VirtualBaseAddr();

    virtual
    DWORD       NumPhysicalAddr();

    virtual
    DWORD       PhysicalLen(DWORD dwIndex);

    virtual
    DWORD       PhysicalAddr(DWORD dwIndex);

    virtual
    BOOL        SupportsRotation() { return TRUE; }

    virtual
    BOOL        SupportsOffscreenSurfaces();

    //------------------------------------------------------------------------------
    //
    //  Method: Allocate
    //
    //  Allocates an OMAP Surface object
    //
    virtual
    BOOL    Allocate(
                OMAP_DSS_PIXELFORMAT    ePixelFormat,
                DWORD                   dwWidth,
                DWORD                   dwHeight,
                OMAPSurface**           ppSurface
                );
    virtual
    BOOL    Allocate(
                OMAP_DSS_PIXELFORMAT    ePixelFormat,
                DWORD                   dwWidth,
                DWORD                   dwHeight,
                OMAPSurface**           ppAssocSurface,
                OMAPSurface*            pSurface
                );

    virtual
    BOOL    AllocateGDI(
                OMAP_DSS_PIXELFORMAT    ePixelFormat,
                DWORD                   dwWidth,
                DWORD                   dwHeight,
                OMAPSurface**           ppSurface
                );

protected:

    HANDLE      m_hHeap;
    DWORD       m_dwDisplayBufferSize;
    DWORD       m_dwPhysicalDisplayAddr;
    VOID*       m_pVirtualDisplayBuffer;
                    
    HANDLE      m_hOffscreenHeap;
    VOID*       m_pOffscreenBuffer;
    DWORD       m_dwOffscreenPhysical;
};


#endif __DSSAI_H__

