// All rights reserved ADENEO EMBEDDED 2010
// Portions Copyright (c) 2009 BSQUARE Corporation. All rights reserved.
//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this source code is subject to the terms of the Microsoft end-user
// license agreement (EULA) under which you licensed this SOFTWARE PRODUCT.
// If you did not accept the terms of the EULA, you are not authorized to use
// this source code. For a copy of the EULA, please see the LICENSE.RTF on your
// install media.
//
// Portions Copyright (c) Texas Instruments.  All rights reserved.
//
//------------------------------------------------------------------------------
//
#include <windows.h>
#include <ceddk.h>
#include <ceddkex.h>

#include "sdk_i2c.h"
#include "sdk_gpio.h"
#include "bsp_cfg.h"

#include "ispreg.h"
#include "tvp5146.h"
#include "util.h"
#include "params.h"
#include "ISPctrl.h"
#include "dbgsettings.h"

#define FOURCC_UYVY     mmioFOURCC('U', 'Y', 'V', 'Y')  // MSYUV: 1394 conferencing camera 4:4:4 mode 1 and 3
#define FOURCC_YUY2     mmioFOURCC('Y', 'U', 'Y', '2')

//-----------------------------------------------------------------------------
// Global variable

//------------------------------------------------------------------------------
//  Variable define
#define GPIO_MODE_MASK_EVENPIN  0xfffffff8
#define GPIO_MODE_MASK_ODDPIN   0xfff8ffff

//------------------------------------------------------------------------------
//  Buffer size define
#ifdef ENABLE_PACK8
    #define IMAGE_CAMBUFF_SIZE              (MAX_X_RES*MAX_Y_RES*2)//video input buffer size
    #define NUM_BYTES_LINE                  (MAX_X_RES*2)   
#else
    #define IMAGE_CAMBUFF_SIZE              (MAX_X_RES*MAX_Y_RES*4)//video input buffer size
    #define NUM_BYTES_LINE                  (MAX_X_RES*4)
#endif //ENABLE_PACK8

CIspCtrl::CIspCtrl()
{   
    m_pCCDCRegs = NULL;
    m_pYUVDMAAddr=NULL;
    m_pYUVVirtualAddr=NULL;
    m_pYUVPhysicalAddr=NULL;
    m_bEnabled= FALSE;
}

CIspCtrl::~CIspCtrl()
{
}

//-----------------------------------------------------------------------------
//
//  Function:       GetPhysFromVirt
//
//  Maps the Virtual address passed to a physical address.
//
//  returns a physical address with a page boundary size.
//
LPVOID
CIspCtrl::GetPhysFromVirt(
    ULONG ulVirtAddr
    )
{
    ULONG aPFNTab[1];
    ULONG ulPhysAddr;


    if (LockPages((LPVOID)ulVirtAddr, UserKInfo[KINX_PAGESIZE], aPFNTab,
                   LOCKFLAG_QUERY_ONLY))
        {
         // Merge PFN with address offset to get physical address         
         ulPhysAddr= ((*aPFNTab << UserKInfo[KINX_PFN_SHIFT]) & UserKInfo[KINX_PFN_MASK])|(ulVirtAddr & 0xFFF);
        } else {
        ulPhysAddr = 0;
        }
    return ((LPVOID)ulPhysAddr);
}
//------------------------------------------------------------------------------
//
//  Function:  MapCameraReg
//
//  Map camera registers to virtual space
//      
BOOL CIspCtrl::MapCameraReg()
{
    DEBUGMSG(ZONE_FUNCTION,(TEXT("+MapCameraReg\r\n")));
    PHYSICAL_ADDRESS pa;
           
    // To map Camera ISP CCDC register
    pa.QuadPart = GetAddressByDevice(BSPGetCameraDevice());
    m_pCCDCRegs = (CAM_ISP_CCDC_REGS *)MmMapIoSpace(pa, sizeof(CAM_ISP_CCDC_REGS), FALSE);
    if (m_pCCDCRegs == NULL)
    {
        ERRORMSG(ZONE_ERROR,(TEXT("Failed map Camera ISP CCDC physical address to virtual address!\r\n")));
        return FALSE;
    }
    
    return TRUE;
}

//------------------------------------------------------------------------------
//
//  Function:  MapCameraReg
//
//  Read data from register
//      
VOID CIspCtrl::UnMapCameraReg()
{
    if (m_pCCDCRegs != NULL)
    {
	    MmUnmapIoSpace((PVOID)m_pCCDCRegs, sizeof(CAM_ISP_CCDC_REGS));
    }
}

//------------------------------------------------------------------------------
//
//  Function:  ConfigGPIO4MDC
//
//  Set GPIO58  mode4 output  0->1 (reset)
//  Set GPIO134 mode4 output  0 
//  Set GPIO136 mode4 output  1 
//  Set GPIO54  mode4 output  1         
//  Set GPIO139 mode4 input 
//
/*
BOOL CIspCtrl::ConfigGPIO4MDC()
{
    DEBUGMSG(ZONE_FUNCTION,(TEXT("+ConfigGPIO4MDC\r\n")));

    HANDLE hGpio = GPIOOpen();
  
    Sleep(20);
    GPIOClrBit(hGpio, VIDEO_CAPTURE_RESET);
    GPIOSetMode(hGpio, VIDEO_CAPTURE_RESET, GPIO_DIR_OUTPUT);
    Sleep(20);
    GPIOSetBit(hGpio, VIDEO_CAPTURE_RESET);
    GPIOSetMode(hGpio, VIDEO_CAPTURE_RESET, GPIO_DIR_OUTPUT);

    GPIOClose(hGpio);

    return TRUE;
}       
*/

//------------------------------------------------------------------------------
//
//  Function:  CCDCInitSYNC
//
//  Init. ISPCCDC_SYN_MODE register 
//
//
BOOL CIspCtrl::CCDCInitSYNC()
{
        DEBUGMSG(ZONE_FUNCTION, (TEXT("+CCDCInitSYNC\r\n")));
        
        UINT32 syn_mode = 0 ;
        syn_mode |= ISPCCDC_SYN_MODE_WEN;// Video data to memory 
        syn_mode |= ISPCCDC_SYN_MODE_DATSIZ_10;// cam_d is 10 bits
        syn_mode |= ISPCCDC_SYN_MODE_VDHDEN;// Enable timing generator
#ifdef ENABLE_PROGRESSIVE_INPUT
        syn_mode |= ISPCCDC_SYN_MODE_FLDMODE_PROGRESSIVE; // Set field mode: interlaced
#else
        syn_mode |= ISPCCDC_SYN_MODE_FLDMODE_INTERLACED; // Set field mode: interlaced
#endif

        syn_mode = (syn_mode & ISPCCDC_SYN_MODE_INPMOD_MASK)| ISPCCDC_SYN_MODE_INPMOD_RAW;//Set input mode: RAW
#ifdef ENABLE_PACK8     
        syn_mode |=ISPCCDC_SYN_MODE_PACK8; //pack 8-bit in memory
#endif //   ENABLE_PACK8    
        ISP_OutReg32(&m_pCCDCRegs->CCDC_SYN_MODE, syn_mode);
        
#ifdef ENABLE_BT656     
        ISP_OutReg32(&m_pCCDCRegs->CCDC_REC656IF, ISPCCDC_REC656IF_R656ON); //enable BT656
#endif //ENABLE_BT656
        return TRUE;
}
//------------------------------------------------------------------------------
//
//  Function:  CCDCInitCFG
//
//  Init. Camera CCDC   
//
BOOL CIspCtrl::CCDCInitCFG(PCS_DATARANGE_VIDEO pCsDataRangeVideo)
{
        DEBUGMSG(ZONE_FUNCTION, (TEXT("+CCDCInitCFG\r\n")));

#ifdef ENABLE_PACK8
        PCS_VIDEOINFOHEADER pCsVideoInfoHdr = &(pCsDataRangeVideo->VideoInfoHeader);
        UINT biCompression = pCsVideoInfoHdr->bmiHeader.biCompression & ~BI_SRCPREROTATE;
#endif
        // Request Init
        UINT32 setting = 0 ;
        ISP_InReg32(&m_pCCDCRegs->CCDC_CFG, &setting);
        
        setting |= (ISPCCDC_CFG_VDLC | (1 << ISPCCDC_CFG_FIDMD_SHIFT)); // must be set to 1 according to TRM

        setting &= ~ISPCCDC_CFG_MSBINVI; // MSB of chroma input signal not inverted when stored to memory
        
        #ifdef ENABLE_PACK8
            if (FOURCC_UYVY == biCompression) {
                setting &= ~ISPCCDC_CFG_BSWD; // normal - don't swap bytes
            }
            else if (FOURCC_YUY2 == biCompression) {
                setting |= ISPCCDC_CFG_BSWD; //swap byte
            }
            else {
                // Unsupported format
                DEBUGMSG(ZONE_ERROR, 
                    (TEXT("CIspCtrl::CCDCInitCFG: ERROR - Unsupported FOURCC\r\n")));
                return FALSE;
            }
        #endif
        
        #ifdef ENABLE_BT656
            #ifndef ENABLE_PACK8
                setting |=ISPCCDC_CFG_BW656; //using 10-bit BT656
            #endif //ENABLE_PACK8
        #endif //ENABLE_BT656
        
        ISP_OutReg32(&m_pCCDCRegs->CCDC_CFG, setting);
        return TRUE;
}
//------------------------------------------------------------------------------
//
//  Function:  ConfigOutlineOffset
//
//  Configures the output line offset when stored in memory.
//  Configures the num of even and odd line fields in case of rearranging
//  the lines
//  offset: twice the Output width and aligned on 32byte boundary.
//  oddeven: odd/even line pattern to be chosen to store the output
//  numlines: Configure the value 0-3 for +1-4lines, 4-7 for -1-4lines
//
BOOL CIspCtrl::ConfigOutlineOffset(UINT32 offset, UINT8 oddeven, UINT8 numlines)
{       
     DEBUGMSG(ZONE_FUNCTION, (TEXT("+ConfigOutlineOffset\r\n")));
     UINT32 setting = 0;    
     
    // Make sure offset is multiple of 32bytes. ie last 5bits should be zero 
    setting = offset & ISP_32B_BOUNDARY_OFFSET;
    ISP_OutReg32(&m_pCCDCRegs->CCDC_HSIZE_OFF, setting);

    // By default Donot inverse the field identification 
    ISP_InReg32(&m_pCCDCRegs->CCDC_SDOFST, &setting);
    setting &= (~ISPCCDC_SDOFST_FINV);
    ISP_OutReg32(&m_pCCDCRegs->CCDC_SDOFST, setting);

    // By default one line offset
    ISP_InReg32(&m_pCCDCRegs->CCDC_SDOFST, &setting);
    setting &= ISPCCDC_SDOFST_FOFST_1L;
    ISP_OutReg32(&m_pCCDCRegs->CCDC_SDOFST, setting);

    switch (oddeven) {
    case EVENEVEN:      /*even lines even fields*/
        ISP_InReg32(&m_pCCDCRegs->CCDC_SDOFST, &setting);
        setting |= ((numlines & 0x7) << ISPCCDC_SDOFST_LOFST0_SHIFT);
        ISP_OutReg32(&m_pCCDCRegs->CCDC_SDOFST, setting);
        break;
    case ODDEVEN:       /*odd lines even fields*/
        ISP_InReg32(&m_pCCDCRegs->CCDC_SDOFST, &setting);
        setting |= ((numlines & 0x7) << ISPCCDC_SDOFST_LOFST1_SHIFT);
        ISP_OutReg32(&m_pCCDCRegs->CCDC_SDOFST, setting);
        break;
    case EVENODD:       /*even lines odd fields*/
        ISP_InReg32(&m_pCCDCRegs->CCDC_SDOFST, &setting);
        setting |= ((numlines & 0x7) << ISPCCDC_SDOFST_LOFST2_SHIFT);
        ISP_OutReg32(&m_pCCDCRegs->CCDC_SDOFST, setting);
        break;
    case ODDODD:        /*odd lines odd fields*/
        ISP_InReg32(&m_pCCDCRegs->CCDC_SDOFST, &setting);
        setting |= ((numlines & 0x7) << ISPCCDC_SDOFST_LOFST3_SHIFT);
        ISP_OutReg32(&m_pCCDCRegs->CCDC_SDOFST, setting);
        break;
    default:
        break;
    }
    return TRUE;
}       
//------------------------------------------------------------------------------
//
//  Function:  CCDCSetOutputAddress
//
//  Configures the memory address where the output should be stored.
//
BOOL CIspCtrl::CCDCSetOutputAddress(ULONG SDA_Address)
{       
	ULONG addr = (SDA_Address);
	addr = addr & ISP_32B_BOUNDARY_BUF;
	ISP_OutReg32(&m_pCCDCRegs->CCDC_SDR_ADDR, addr);
	return TRUE;            
}   
//------------------------------------------------------------------------------
//
//  Function:  CCDCEnable
//
//  Enables the CCDC module.
//
BOOL CIspCtrl::CCDCEnable(BOOL bEnable)
{       
    DEBUGMSG(ZONE_FUNCTION, (TEXT("+CCDCEnable\r\n"))); 

	UINT32 setting = 0;
    
    ISP_InReg32(&m_pCCDCRegs->CCDC_PCR, &setting);  
    if (bEnable)
	{
        setting |= (ISPCCDC_PCR_EN);
	}
    else
	{
        setting &= ~(ISPCCDC_PCR_EN);
	}
        
    return ISP_OutReg32(&m_pCCDCRegs->CCDC_PCR, setting);
}   

//------------------------------------------------------------------------------
//
//  Function:  AllocBuffer
//
//  AllocBuffer for video input and format transfer out 
//
BOOL CIspCtrl::AllocBuffer()
{
    DWORD dwSize = IMAGE_CAMBUFF_SIZE;
    PHYSICAL_ADDRESS   pCamBufferPhys; 
    DMA_ADAPTER_OBJECT camBuffer;

	DEBUGMSG(ZONE_FUNCTION, (TEXT("+AllocBuffer\r\n")));

    if(m_pYUVDMAAddr)
	{
        return TRUE;
	}
      
    camBuffer.ObjectSize = sizeof(camBuffer);
    camBuffer.InterfaceType = Internal;
    camBuffer.BusNumber = 0;        
    
    m_pYUVDMAAddr = (PBYTE)HalAllocateCommonBuffer(&camBuffer, dwSize, &pCamBufferPhys, TRUE );
    
    if (m_pYUVDMAAddr == NULL)
    {   
        ERRORMSG(ZONE_ERROR, (TEXT("HalAllocateCommonBuffer failed !!!\r\n")));
        return FALSE;
    }
        
    m_pYUVVirtualAddr = (PBYTE)VirtualAlloc(NULL,dwSize, MEM_RESERVE,PAGE_NOACCESS);        

    if (m_pYUVVirtualAddr == NULL)
    {   ERRORMSG(ZONE_ERROR, (TEXT("Sensor buffer memory alloc failed !!!\r\n")));
        return FALSE;
    }
  
	VirtualCopy(m_pYUVVirtualAddr, (VOID *) (pCamBufferPhys.LowPart >> 8), dwSize, PAGE_READWRITE | PAGE_PHYSICAL); // | PAGE_NOCACHE  );

	m_pYUVPhysicalAddr = GetPhysFromVirt((ULONG)m_pYUVVirtualAddr);
	if(!m_pYUVPhysicalAddr)
	{
		ERRORMSG(ZONE_ERROR,(_T("GetPhysFromVirt 0x%08X failed: \r\n"), m_pYUVVirtualAddr));
		return FALSE;
	}

	DEBUGMSG(MASK_DMA, (TEXT("m_pYUVVirtualAddr=0x%x\r\n"),m_pYUVVirtualAddr));   
	DEBUGMSG(MASK_DMA, (TEXT("m_pYUVPhysicalAddr=0x%x\r\n"),m_pYUVPhysicalAddr)); 
	DEBUGMSG(MASK_DMA, (TEXT("m_pYUVDMAAddr=0x%x\r\n"),m_pYUVDMAAddr));   

	return TRUE;      
}   
//------------------------------------------------------------------------------
//
//  Function:  DeAllocBuffer
//
//  DeAllocBuffer for video input and format transfer out   
//
BOOL CIspCtrl::DeAllocBuffer()
{
    DEBUGMSG(ZONE_FUNCTION, (TEXT("+DeAllocBuffer\r\n")));

    if(!m_pYUVDMAAddr)
	{
        return TRUE;
	}

    if(!VirtualFree( m_pYUVDMAAddr, 0, MEM_RELEASE ))
	{
        DEBUGMSG(1,(_T("CIspCtrl::DeAllocBuffer failed \r\n")));
		return FALSE;
	}
        
    m_pYUVVirtualAddr=NULL;
    m_pYUVPhysicalAddr=NULL;
    m_pYUVDMAAddr=NULL;

    return TRUE;    
}
//------------------------------------------------------------------------------
//
//  Function:  CCDCInit
//
//  Init. Camera CCDC   
//
BOOL CIspCtrl::CCDCInit(PCS_DATARANGE_VIDEO pCsDataRangeVideo)
{
    DEBUGMSG(ZONE_FUNCTION, (TEXT("+CCDCInit\r\n")));

    if (!CCDCInitCFG(pCsDataRangeVideo))
	{
		ERRORMSG(TRUE, (_T("CCDCInit: Failed to initialize CCDC configuration\r\n")));
        return FALSE;
	}

    if (!CCDCInitSYNC())
	{
		ERRORMSG(TRUE, (_T("CCDCInit: Failed to initialize CCDC synchronization\r\n")));
        return FALSE;
	}

    if (!CCDCSetOutputAddress((ULONG) m_pYUVPhysicalAddr)) 
	{
		ERRORMSG(TRUE, (_T("CCDCInit: Failed to set CCDC output address\r\n")));
        return FALSE;
    }
    
    return TRUE;        
}       

//------------------------------------------------------------------------------
//
//  Function:  ISPConfigSize
//
//
// Configures CCDC HORZ/VERT_INFO registers to decide the start line
// stored in memory.
//
// output_w : output width from the CCDC in number of pixels per line
// output_h : output height for the CCDC in number of lines
//
//
BOOL CIspCtrl::ISPConfigSize(UINT width, UINT height, UINT bpp)
{
    DEBUGMSG(ZONE_FUNCTION, (TEXT("+ISPConfigSize\r\n")));

    UINT32 setting ;
    UINT numBytesLine = 0;
    UINT lines = 0;

    if (bpp != 16) 
	{
        DEBUGMSG(ZONE_ERROR, 
            (TEXT("CIspCtrl::ISPConfigSize: ERROR - Unsupported bpp value %d\r\n"), 
            bpp));
        return FALSE;
    }

    numBytesLine = (width * bpp)/8;

#ifdef ENABLE_PROGRESSIVE_INPUT
    lines = height;
#else
    lines = (height/2);
#endif
    // Set output_w         
    setting = (HORZ_INFO_SPH_VAL << ISPCCDC_HORZ_INFO_SPH_SHIFT) | ((numBytesLine - 1)<< ISPCCDC_HORZ_INFO_NPH_SHIFT);
    ISP_OutReg32(&m_pCCDCRegs->CCDC_HORZ_INFO, setting);
    
    //vertical shift
    setting = ((VERT_START_VAL) << ISPCCDC_VERT_START_SLV0_SHIFT | (VERT_START_VAL) << ISPCCDC_VERT_START_SLV1_SHIFT);
    ISP_OutReg32(&m_pCCDCRegs->CCDC_VERT_START, setting);
         
    // Set output_h
    setting = (lines - 1) << ISPCCDC_VERT_LINES_NLV_SHIFT;
    ISP_OutReg32(&m_pCCDCRegs->CCDC_VERT_LINES, setting);

    ConfigOutlineOffset(numBytesLine, 0, 0);
#ifdef ENABLE_DEINTERLACED_OUTPUT    //There is no field pin connected to OMAP3 from tvp5416, so only for BT656.
    //de-interlace
    ConfigOutlineOffset(numBytesLine, EVENEVEN, 1);
    ConfigOutlineOffset(numBytesLine, ODDEVEN, 1);
    ConfigOutlineOffset(numBytesLine, EVENODD, 1);
    ConfigOutlineOffset(numBytesLine, ODDODD, 1);   
#endif //ENABLE_DEINTERLACED_OUTPUT

    return TRUE;        
}

//------------------------------------------------------------------------------
//
//  Function:  IsCCDCBusy
//
//  To check CCDC busy bit
//
BOOL CIspCtrl::IsCCDCBusy()
{
    DEBUGMSG(ZONE_FUNCTION, (TEXT("+IsCCDCBusy\r\n")));
            
    UINT32 setting = 0;
    
    ISP_InReg32(&m_pCCDCRegs->CCDC_PCR, &setting); 
    setting &= ISPCCDC_PCR_BUSY;

	return (setting) ? TRUE : FALSE;
}
        
//------------------------------------------------------------------------------
//
//  Function:  InitializeCamera
//
//  To initialize Camera .
//
BOOL CIspCtrl::InitializeCamera()
{    
    DEBUGMSG(ZONE_FUNCTION, (TEXT("+InitializeCamera\r\n")));    

    MapCameraReg(); //Map camera registers
	ISPInit();

    return TRUE;
}

//------------------------------------------------------------------------------
//
//  Function:  DeInitializeCamera
//
//  To deinitialize Camera .
//
VOID CIspCtrl::DeInitializeCamera()
{    
    DEBUGMSG(ZONE_FUNCTION, (TEXT("+InitializeCamera\r\n")));    

	ISPDeInit();
    UnMapCameraReg(); 
}

//------------------------------------------------------------------------------
//
//  Function:  EnableCamera
//
//  To enable Camera.
//
BOOL CIspCtrl::EnableCamera(PCS_DATARANGE_VIDEO pCsDataRangeVideo)
{    
    if (m_bEnabled) 
	{
        return TRUE;
    }

	ISPEnable(TRUE);

    DEBUGMSG(ZONE_FUNCTION, (TEXT("+EnableCamera\r\n")));
    
    PCS_VIDEOINFOHEADER pCsVideoInfoHdr = &(pCsDataRangeVideo->VideoInfoHeader);

    UINT biWidth        = pCsVideoInfoHdr->bmiHeader.biWidth;
    UINT biHeight       = abs(pCsVideoInfoHdr->bmiHeader.biHeight);
    UINT biSizeImage    = pCsVideoInfoHdr->bmiHeader.biSizeImage;
    UINT biBitCount     = pCsVideoInfoHdr->bmiHeader.biBitCount;

    if (!CCDCInit(pCsDataRangeVideo)) // Init. CCDC  
	{
		ERRORMSG(TRUE, (TEXT("EnableCamera : Failed to initialize CCDC\r\n")));
        return FALSE;
	}

    if (!ISPConfigSize(biWidth, biHeight, biBitCount))// Init. video size 
	{
		ERRORMSG(TRUE, (TEXT("EnableCamera : Failed to configure video size\r\n")));
		return FALSE;
	}

    memset(m_pYUVVirtualAddr, 0, biSizeImage); //clear the frame buffer

	DumpRegisters();

    if (!CCDCEnable(TRUE))
	{
		ERRORMSG(TRUE, (TEXT("EnableCamera : Failed to enable CCDC\r\n")));
        return FALSE;
	}

    m_bEnabled = TRUE;

    return TRUE;
}

//------------------------------------------------------------------------------
//
//  Function:  DisableCamera
//
//  To disable Camera.
//
BOOL CIspCtrl::DisableCamera()
{
    if (!m_bEnabled) 
	{
        return TRUE;
    }

    DEBUGMSG(ZONE_FUNCTION, (TEXT("+DisableCamera\r\n")));

    if (!CCDCEnable(FALSE))
	{
		ERRORMSG(TRUE, (TEXT("DisableCamera : Failed to disable CCDC\r\n")));
		return FALSE;
	}

	ISPEnable(FALSE);	

	m_bEnabled = FALSE;

    return TRUE;
}

//------------------------------------------------------------------------------
//
//  Function:  ChangeFrameBuffer
//
//  To Change the frame buffer address to CCDC_SDR.
//
BOOL CIspCtrl::ChangeFrameBuffer(ULONG ulVirtAddr)
{
    DEBUGMSG(ZONE_FUNCTION, (TEXT("+ChangeFrameBuffer\r\n")));  

    m_pYUVVirtualAddr= (LPVOID) ulVirtAddr;
    m_pYUVPhysicalAddr = GetPhysFromVirt((ULONG)m_pYUVVirtualAddr);
    if(!m_pYUVPhysicalAddr)
	{
        return FALSE;   
	}

    //Set CCDC_SDR address
    CCDCSetOutputAddress((ULONG) m_pYUVPhysicalAddr);
    
    return TRUE;
}

VOID CIspCtrl::DumpRegisters()
{
	DEBUGMSG(TRUE, (TEXT("DumpRegisters:\r\n")));  
	DEBUGMSG(TRUE, (TEXT("--------------\r\n")));  
	DEBUGMSG(TRUE, (TEXT("CCDC_SYN_MODE=0x%08x\r\n"),	INREG32(&m_pCCDCRegs->CCDC_SYN_MODE)));  
	DEBUGMSG(TRUE, (TEXT("CCDC_REC656IF=0x%08x\r\n"),	INREG32(&m_pCCDCRegs->CCDC_REC656IF)));  
	DEBUGMSG(TRUE, (TEXT("CCDC_CFG=0x%08x\r\n"),		INREG32(&m_pCCDCRegs->CCDC_CFG)));  
	DEBUGMSG(TRUE, (TEXT("CCDC_HSIZE_OFF=0x%08x\r\n"),	INREG32(&m_pCCDCRegs->CCDC_HSIZE_OFF)));  
	DEBUGMSG(TRUE, (TEXT("CCDC_SDOFST=0x%08x\r\n"),		INREG32(&m_pCCDCRegs->CCDC_SDOFST)));  
	DEBUGMSG(TRUE, (TEXT("CCDC_SDR_ADDR=0x%08x\r\n"),	INREG32(&m_pCCDCRegs->CCDC_SDR_ADDR)));  
	DEBUGMSG(TRUE, (TEXT("CCDC_PCR=0x%08x\r\n"),		INREG32(&m_pCCDCRegs->CCDC_PCR)));  
	DEBUGMSG(TRUE, (TEXT("CCDC_HORZ_INFO=0x%08x\r\n"),	INREG32(&m_pCCDCRegs->CCDC_HORZ_INFO)));  
	DEBUGMSG(TRUE, (TEXT("CCDC_VERT_START=0x%08x\r\n"),	INREG32(&m_pCCDCRegs->CCDC_VERT_START)));  
	DEBUGMSG(TRUE, (TEXT("CCDC_VERT_LINES=0x%08x\r\n"),	INREG32(&m_pCCDCRegs->CCDC_VERT_LINES)));  
}