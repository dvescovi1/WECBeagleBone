//-------------------------------------------------------------------------
// <copyright file="jpeg2rgb.cpp" company="Microsoft">
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
//    MJPEG DirectShow Filter for use with USB camera driver for 
//    Windows Embedded CE 6.0
// </summary>
//-------------------------------------------------------------------------
//======================================================================
// USB Camera driver for Windows Embedded CE 6.0
//======================================================================


#include <windows.h>
#include <initguid.h>
#include <imaging.h>

#include <streams.h>  // DirectShow base class library
#include "debug.h"

// Forward declares
void GetVideoInfoParameters(
        const VIDEOINFOHEADER *pvih, // Pointer to the format header.
        DWORD *pdwWidth,             // Returns the width in pixels.
        DWORD *pdwHeight             // Returns the height in pixels.
        );
bool IsValidRGB(const CMediaType *pmt);
bool IsValidJPEG(const CMediaType *pmt);
BOOL GetCodecCLSID(IImagingFactory* pImagingFactory, CLSID * pclsid, WCHAR * wszMimeType);


// Define the filter's CLSID:  {A6512C9F-A47B-45ba-A054-0DB0D4BB87F7}
static const GUID CLSID_JPEGFilter = 
{ 0xa6512c9f, 0xa47b, 0x45ba, { 0xa0, 0x54, 0xd, 0xb0, 0xd4, 0xbb, 0x87, 0xf7 } };

// A name for the filter 
static const WCHAR g_wszName[] = L"JPEG -> RGB Filter";


//----------------------------------------------------------------------------
// CJPEGFilter Class
//
// This class defines the filter. It inherits CTransformFilter, which is a 
// base class for copy-transform filters. 
//-----------------------------------------------------------------------------

class CJPEGFilter : public CTransformFilter
{
public:
    CJPEGFilter(TCHAR *pName, LPUNKNOWN pUnk, HRESULT *phr);
    ~CJPEGFilter();

    // Overridden CTransformFilter methods
    HRESULT CheckInputType(const CMediaType *mtIn);
    HRESULT CheckTransform(const CMediaType *mtIn, const CMediaType *mtOut);
    HRESULT DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProp);
    HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);
    HRESULT Transform(IMediaSample *pIn, IMediaSample *pOut);

    // Override this so we can grab the video format
    HRESULT SetMediaType(PIN_DIRECTION direction, const CMediaType *pmt);

    // Static object-creation method (for the class factory)
    static CUnknown * WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *pHr); 
    LPAMOVIESETUP_FILTER GetSetupData();

    DECLARE_IUNKNOWN;

private:

    // TODO: make static, call from DllInit()
    HRESULT InitImaging(); 

    IImagingFactory *m_pImagingFactory;

    VIDEOINFOHEADER m_VihIn;   // Holds the current video format (input)
    VIDEOINFOHEADER m_VihOut;  // Holds the current video format (output)

    BOOL m_bFirstFrame;
};



//----------------------------------------------------------------------------
//
// Global data
//
//----------------------------------------------------------------------------


//
// The next bunch of structures define information for the class factory.
// This filter only supports IJPG/MJPG input and RGB24 output.
//
// If you change this, you must also change the values under
//  [HKEY_CLASSES_ROOT\CLSID\{A6512C9F-A47B-45ba-A054-0DB0D4BB87F7}
//
AMOVIESETUP_MEDIATYPE subMediaTypeInput[] = {
    { &MEDIATYPE_Video, &MEDIASUBTYPE_MJPG },
    { &MEDIATYPE_Video, &MEDIASUBTYPE_IJPG },
};

AMOVIESETUP_MEDIATYPE subMediaTypeOutput[] = {
    { &MEDIATYPE_Video, &MEDIASUBTYPE_RGB24},
};

AMOVIESETUP_PIN sudPins[] = {
    {
    L"Input",                   // strName
    FALSE,                      // bRendered
    FALSE,                      // bOutput
    FALSE,                      // bZero
    FALSE,                      // bMany
    &CLSID_NULL,                // clsConnectsToFilter
    NULL,                       // strConnectsToPin
    NUMELMS(subMediaTypeInput), // nTypes
    subMediaTypeInput,          // lpTypes
    },
    {
    L"Output",                  // strName
    FALSE,                      // bRendered
    TRUE,                       // bOutput
    FALSE,                      // bZero
    FALSE,                      // bMany
    &CLSID_NULL,                // clsConnectsToFilter
    NULL,                       // strConnectsToPin
    NUMELMS(subMediaTypeOutput),// nTypes
    subMediaTypeOutput,         // lpTypes
    },

};


const AMOVIESETUP_FILTER sudFilterReg =
{
    &CLSID_JPEGFilter,  // CLSID
    g_wszName,          // Name
    MERIT_NORMAL,       // Merit
    NUMELMS(sudPins),   // Number of AMOVIESETUP_PIN structs
    sudPins,            // Pin registration information.
};

void DllInit( BOOL bLoading, const CLSID *rclsid )
{
    if (bLoading) {
        DEBUGMSG(ZONE_INIT, (TEXT("JPEG -> RGB filter loaded")));
        // TODO:
        //  Init COM, Imaging
    } else {
        DEBUGMSG(ZONE_INIT, (TEXT("JPEG -> RGB filter unloaded")));   
        // TODO:
        //  Cleanup COM, Imaging
    }
}

CFactoryTemplate g_Templates[] = 
{
    { 
      g_wszName,                // Name
      &CLSID_JPEGFilter,        // CLSID
      CJPEGFilter::CreateInstance,    // Method to create an instance of MyComponent
      DllInit,
      &sudFilterReg             // Set-up information (for filters)
    }
};

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);    



//----------------------------------------------------------------------------
// CJPEGFilter::CJPEGFilter
//
//----------------------------------------------------------------------------
CJPEGFilter::CJPEGFilter(TCHAR *pName, LPUNKNOWN pUnk, HRESULT *phr) 
    : CTransformFilter( pName, pUnk, CLSID_JPEGFilter), 
    m_bFirstFrame(TRUE) 
{
    DEBUGMSG(ZONE_INIT, (TEXT("%s constructed\n"), pName));
    *phr = S_OK;
}



//----------------------------------------------------------------------------
// CJPEGFilter::~CJPEGFilter
//
//----------------------------------------------------------------------------
CJPEGFilter::~CJPEGFilter()
{
    // TODO: release COM, Imaging?  Or do it on DLL unload?        
}


//----------------------------------------------------------------------------
// CJPEGFilter::GetSetupData
//
//----------------------------------------------------------------------------
LPAMOVIESETUP_FILTER CJPEGFilter::GetSetupData() 
{
    DEBUGMSG(ZONE_ENTER, (TEXT("CJPEGFilter::GetSetupData()\n")));
    return (LPAMOVIESETUP_FILTER) & sudFilterReg;
}



//----------------------------------------------------------------------------
// CJPEGFilter::CheckInputType
//
// Examine a proposed input type. Returns S_OK if we can accept his input type
// or VFW_E_TYPE_NOT_ACCEPTED otherwise. 
// This filter accepts IJPG and MJPG types only.
//-----------------------------------------------------------------------------

HRESULT CJPEGFilter::CheckInputType(const CMediaType *pmt)
{
    HRESULT hr;
    
    DEBUGMSG(ZONE_ENTER, (TEXT("+CJPEGFilter::CheckInputType()\n")));

    if (IsValidJPEG(pmt))
    {
        hr = S_OK;
    }
    else
    {
        hr = VFW_E_TYPE_NOT_ACCEPTED;
    }

    DEBUGMSG(ZONE_ENTER, (TEXT("-CJPEGFilter::CheckInputType() = 0x%x\n"), hr));
    
    return hr;
}



//----------------------------------------------------------------------------
// CJPEGFilter::CheckTransform
//
// Compare an input type with an output type, and see if we can convert from 
// one to the other. The input type is known to be OK from ::CheckInputType,
// so this is really a check on the output type.
//-----------------------------------------------------------------------------


HRESULT CJPEGFilter::CheckTransform(const CMediaType *mtIn, const CMediaType *mtOut)
{
    HRESULT hr;
    
    DEBUGMSG(ZONE_ENTER, (TEXT("+CJPEGFilter::CheckTransform()\n")));

    // Make sure the subtypes match
    if(!IsValidJPEG(mtIn) || !IsValidRGB(mtOut))
    {
        hr = VFW_E_TYPE_NOT_ACCEPTED;
    }


    BITMAPINFOHEADER *pBmi  = HEADER(mtIn);
    BITMAPINFOHEADER *pBmi2 = HEADER(mtOut);

    if ((pBmi->biWidth <= pBmi2->biWidth) &&
        (pBmi->biHeight == abs(pBmi2->biHeight)))
    {
       hr = S_OK;
    } 
    else 
    {
        DEBUGMSG(ZONE_WARNING, (TEXT("WARNING: CJPEGFilter: input res != output res; frame dropped\n")));
        hr = VFW_E_TYPE_NOT_ACCEPTED;
    }

    DEBUGMSG(ZONE_ENTER, (TEXT("-CJPEGFilter::CheckTransform(), hr = 0x%x\n"), hr));

    return hr;
}


//----------------------------------------------------------------------------
// CJPEGFilter::GetMediaType
//
// Return the output types that we like, in order of preference, by index number.
// NOTE: this filter only supports one output type at present: RGB24.
//
// iPosition:  index number
// pMediaType: Write the media type into this object.
//-----------------------------------------------------------------------------

HRESULT CJPEGFilter::GetMediaType(int iPosition, CMediaType *pOutputType)
{
    HRESULT hr;
    
    DEBUGMSG(ZONE_ENTER, (TEXT("+CJPEGFilter::GetMediaType()\n")));

    // The output pin calls this method only if the input pin is connected.
    ASSERT(m_pInput->IsConnected());
    ASSERT(pOutputType);

    if (iPosition < 0)
    {
        hr = E_INVALIDARG;
    }
    else if (iPosition == 0)
    {
        // First, copy the input type into the output type.
        // We do this so that fields have known good values.
        // Then, change the media subtype to RGB24.
        hr = m_pInput->ConnectionMediaType(pOutputType);
        pOutputType->SetSubtype(&MEDIASUBTYPE_RGB24);

        // TODO: do we have to change the pOutputType->Format struct
        // to account for new bitdepths, etc?  See: colour filter.

        VIDEOINFO *pVideoInfo = (VIDEOINFO *) pOutputType->Format();

        pVideoInfo->bmiHeader.biCompression = BI_RGB;
        pVideoInfo->bmiHeader.biBitCount = 24;
//        pVideoInfo->bmiHeader.biBitCount = GetBitCount(pSubtype);
        pVideoInfo->bmiHeader.biSizeImage = GetBitmapSize(&pVideoInfo->bmiHeader);
        pVideoInfo->bmiHeader.biPlanes = 1;
        pVideoInfo->bmiHeader.biClrUsed = 0;
        pVideoInfo->bmiHeader.biClrImportant = 0;
        ASSERT(pVideoInfo->bmiHeader.biBitCount);
        pOutputType->SetSampleSize(pVideoInfo->bmiHeader.biSizeImage);

        // Make any true colour adjustments
//            if (pVideoInfo->bmiHeader.biBitCount == 16) {
//                pVideoInfo = PrepareTrueColour(pmtOut);
//                if (pVideoInfo == NULL) {
//                    NOTE("No colour type");
//                    return E_OUTOFMEMORY;
//                }
//            }

        // First of all we check that the new output type requires a palette and
        // if so then we give it out fixed palette
//        if (pVideoInfo->bmiHeader.biBitCount == 8) {
//            pVideoInfo = PreparePalette(pmtOut);
//            if (pVideoInfo == NULL) {
//                NOTE("No palette type");
//                return E_OUTOFMEMORY;
//            }
//        }

#ifdef DEBUG        
        DisplayType("CJPEGFilter: reported output type", pOutputType);
#endif        
    }
    else
    {
        hr = VFW_S_NO_MORE_ITEMS;
    }
    
    DEBUGMSG(ZONE_ENTER, (TEXT("-CJPEGFilter::GetMediaType(), hr = 0x%x\n"), hr));

    return hr;
}



//----------------------------------------------------------------------------
// CJPEGFilter::DecideBufferSize
//
// Decide the buffer size and other allocator properties, for the downstream
// allocator.
//
// pAlloc: Pointer to the allocator. 
// pProp:  Contains the downstream filter's request (or all zeroes)
//-----------------------------------------------------------------------------

HRESULT CJPEGFilter::DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProp)
{
    // Make sure the input pin connected.
    HRESULT hr = S_OK;

    DEBUGMSG(ZONE_ENTER, (TEXT("+CJPEGFilter::DecideBufferSize()\n")));

    if (!pAlloc || !pProp)
    {
        hr = E_POINTER;   
        goto Exit;
    }

    if (!m_pInput->IsConnected()) 
    {
        hr = E_UNEXPECTED;
        goto Exit;
    }
 
    // Buffer alignment should be non-zero [zero alignment makes no sense!]
    if (pProp->cbAlign == 0)
    {
        pProp->cbAlign = 1;
    }

    // Number of buffers must be non-zero
    if (pProp->cbBuffer == 0)
    {
        pProp->cBuffers = 1;
    }

    // For buffer size, find the maximum of the upstream size and 
    // the downstream filter's request.
    ALLOCATOR_PROPERTIES FilterProps;
    FilterProps.cbBuffer = m_VihOut.bmiHeader.biWidth * m_VihOut.bmiHeader.biHeight * 3;

    pProp->cbBuffer = max(FilterProps.cbBuffer, pProp->cbBuffer);

    
   
    // Now set the properties on the allocator that was given to us,
    ALLOCATOR_PROPERTIES Actual;
    hr = pAlloc->SetProperties(pProp, &Actual);
    if (FAILED(hr)) 
    {
        goto Exit;
    }
    
    // Even if SetProperties succeeds, the actual properties might be
    // different than what we asked for. We check the result, but we only
    // look at the properties that we care about. The downstream filter
    // will look at them when NotifyAllocator is called.
    
    if (FilterProps.cbBuffer > Actual.cbBuffer) 
    {
        hr = E_FAIL;
        goto Exit;
    }

    hr = S_OK;
    
Exit:    
    DEBUGMSG(ZONE_ENTER, (TEXT("-CJPEGFilter::DecideBufferSize(), hr = 0x%x\n"), hr));
    
    return hr;
}



//----------------------------------------------------------------------------
// CJPEGFilter::SetMediaType
//
// The CTransformFilter class calls this method when the media type is 
// set on either pin. This gives us a chance to grab the format block. 
//
// direction: Which pin (input or output) 
// pmt:       The media type that is being set.
//
// Note: If the pins were friend classes of the filter, we could access the
// connection type directly. But this is easier than sub-classing the pins.
//-----------------------------------------------------------------------------

HRESULT CJPEGFilter::SetMediaType(PIN_DIRECTION direction, const CMediaType *pmt)
{
    HRESULT hr = S_OK;
    
    DEBUGMSG(ZONE_ENTER, (TEXT("+CJPEGFilter::SetMediaType()\n")));

    if (!pmt)
    {
        hr = E_POINTER;   
        goto Exit;
    }
    
    if (direction == PINDIR_INPUT)
    {
#ifdef DEBUG
        DisplayType("CJPEGFilter::SetMediaType(input)", pmt);
#endif        
        ASSERT(pmt->formattype == FORMAT_VideoInfo);
        VIDEOINFOHEADER *pVih = (VIDEOINFOHEADER*)pmt->pbFormat;

        // WARNING! In general you cannot just copy a VIDEOINFOHEADER
        // struct, because the BITMAPINFOHEADER member may be followed by
        // random amounts of palette entries or color masks. (See VIDEOINFO
        // structure in the DShow SDK docs.) Here it's OK because we just
        // want the information that's in the VIDEOINFOHEADER stuct itself.

        CopyMemory(&m_VihIn, pVih, sizeof(VIDEOINFOHEADER));

        DEBUGMSG(ZONEMASK_MTYPES,  
            (TEXT("CJPEGFilter: Input size: bmiWidth = %d, bmiHeight = %d, rcTarget width = %d"),
             m_VihIn.bmiHeader.biWidth, 
             m_VihIn.bmiHeader.biHeight, 
             m_VihIn.rcTarget.right));
    }
    else   // output pin
    {
#ifdef DEBUG
        DisplayType("CJPEGFilter::SetMediaType(output)", pmt);
#endif        

        ASSERT(direction == PINDIR_OUTPUT);
        ASSERT(pmt->formattype == FORMAT_VideoInfo);
        VIDEOINFOHEADER *pVih = (VIDEOINFOHEADER*)pmt->pbFormat;

        CopyMemory(&m_VihOut, pVih, sizeof(VIDEOINFOHEADER));

        DEBUGMSG(ZONEMASK_MTYPES,
            (TEXT("CJPEGFilter: Output size: bmiWidth = %d, bmiHeight = %d, rcTarget width = %d"),
             m_VihOut.bmiHeader.biWidth, 
             m_VihOut.bmiHeader.biHeight, 
             m_VihOut.rcTarget.right));

        //Initialize the Imaging Factory
        hr = InitImaging();
    }

Exit:
    DEBUGMSG(ZONE_ENTER, (TEXT("-CJPEGFilter::SetMediaType(), hr = 0x%x\n"), hr));

    return hr;
}



//----------------------------------------------------------------------------
// CJPEGFilter::Transform
//
// Transform the image.
//
// pSource: Contains the source image.
// pDest:   Write the transformed image here.

// Note: The filter has already set the sample properties on pOut,
// (see CTransformFilter::InitializeOutputSample).
// You can override the timestamps if you need - but not in our case.
//-----------------------------------------------------------------------------

HRESULT CJPEGFilter::Transform(IMediaSample *pSource, IMediaSample *pDest)
{
    HRESULT        hr            = S_OK;
    CMediaType    *pmt           = 0;
    IImageSink    *pEncodeSink   = NULL;
    IImageDecoder *pImageDecoder = NULL;
    IImageEncoder *pImageEncoder = NULL;
    IStream       *pStreamIn     = NULL;
    IStream       *pStreamOut    = NULL;
    CLSID          clsidEncoder;


    DEBUGMSG(ZONE_ENTER, (TEXT("+CJPEGFilter::Transform()\n")));

    if (!pSource || !pDest)
    {
        hr = E_POINTER;   
        goto Exit;
    }

    if (m_pImagingFactory == NULL)
    {
        hr = ERROR_CAN_NOT_COMPLETE;
        goto Exit;
    }


    // Look for format changes from the video renderer.
    if (S_OK == pDest->GetMediaType((AM_MEDIA_TYPE**)&pmt) && pmt)
    {
        DEBUGMSG(ZONEMASK_INFO1, (TEXT("CJPEGFilter: Handling format change from the renderer...")));

#ifdef DEBUG
        DisplayType("CJPEGFilter: output format changed by video renderer!", pmt);
#endif

        // Notify our own output pin about the new type.
        m_pOutput->SetMediaType(pmt);
        DeleteMediaType(pmt);
    }


    // Get pointers to the input and output buffers.
    BYTE *pBufferIn, *pBufferOut;
    pSource->GetPointer(&pBufferIn);
    pDest->GetPointer(&pBufferOut);
    if (pBufferIn == NULL || pBufferOut == NULL)
        goto Exit;


    // pImagingFactory->CreateImageFromBuffer() 
    //  creates decoded image.  Doesn't decode until you
    //  actually try to use the bits
    
    IImage *pJPEGImage = NULL;
    hr = m_pImagingFactory->CreateImageFromBuffer(
        pBufferIn,
        pSource->GetActualDataLength(),
        BufferDisposalFlagNone,
        &pJPEGImage
    );
    
    if (FAILED(hr)) {
        DEBUGMSG(ZONEMASK_ERROR, (TEXT("ERROR: CJPEGFilter: failed to parse input frame\r\n")));   
        goto Exit;
    }    

    ImageInfo imgInfo;
    pJPEGImage->GetImageInfo(&imgInfo);
    
#ifdef DEBUG
    DEBUGMSG(ZONE_INFO_MAX, (TEXT("CJPEGFilter::Transform(): incoming JPEG image:\r\n")));
    DEBUGMSG(ZONE_INFO_MAX, (TEXT("RawDataFormat = 0x%x\r\n"), imgInfo.RawDataFormat));
    DEBUGMSG(ZONE_INFO_MAX, (TEXT("PixelFormat   = 0x%x\r\n"), imgInfo.PixelFormat));
    DEBUGMSG(ZONE_INFO_MAX, (TEXT("Width         = %d\r\n"),   imgInfo.Width));
    DEBUGMSG(ZONE_INFO_MAX, (TEXT("Height        = %d\r\n"),   imgInfo.Height));
    DEBUGMSG(ZONE_INFO_MAX, (TEXT("TileWidth     = %d\r\n"),   imgInfo.TileWidth));
    DEBUGMSG(ZONE_INFO_MAX, (TEXT("TileHeight    = %d\r\n"),   imgInfo.TileHeight));
    DEBUGMSG(ZONE_INFO_MAX, (TEXT("Xdpi          = %d\r\n"),   imgInfo.Xdpi));
    DEBUGMSG(ZONE_INFO_MAX, (TEXT("Ydpi          = %d\r\n"),   imgInfo.Ydpi));
    DEBUGMSG(ZONE_INFO_MAX, (TEXT("Flags         = 0x%x\r\n"), imgInfo.Flags));
#endif

    //
    // Create output stream.
    // Unfortunately, can't create it from pDest, so create a temp stream that goes nowhere.  
    // We'll encode into it (converting from JPEG -> RGB).
    // Later we'll copy from the temp stream to pDest.
    //
    // *** TODO: Implement IStream wrapper that takes pDest and operates directly 
    //           on it.  Will save some copying of pixels.
    //
    if (FAILED(hr = CreateStreamOnHGlobal(NULL, TRUE, &pStreamOut)))
        goto Exit;

    // Create RGB ImageEncoder on output stream
    if (!GetCodecCLSID(m_pImagingFactory, &clsidEncoder, TEXT("image/bmp")))
        goto Exit;

    if (FAILED(hr = m_pImagingFactory->CreateImageEncoderToStream(&clsidEncoder, pStreamOut, &pImageEncoder)))
        goto Exit;

    // Get ImageSink from output encoder
    if (FAILED(hr = pImageEncoder->GetEncodeSink(&pEncodeSink)))
        goto Exit;


    // Push the decoded JPEG data into the RGB output encoder
    // Internally, this handles BeginSink/PushPixelData/EndSink for us.
    if (FAILED(hr = pJPEGImage->PushIntoSink( pEncodeSink )))
        goto Exit;
    

    pEncodeSink->Release();
    pEncodeSink = NULL;
    pImageEncoder->TerminateEncoder();
    

    //
    // Copy the output data from the (temp) IStream to the outgoing IMediaSample
    // Length of the destination image = X * Y  * 3 bytes per pixel (RGB24)
    //
    ULONG count;
    long  cbByte = m_VihOut.bmiHeader.biWidth * m_VihOut.bmiHeader.biHeight * 3;
    pStreamOut->Read( pBufferOut, cbByte, &count );


    //
    // Set output properties, as needed
    //
    ASSERT(pDest->GetSize() >= cbByte);    
//    pDest->SetActualDataLength(cbByte);
    pDest->SetActualDataLength(count);
    


Exit:  
    if (pJPEGImage)
        pJPEGImage->Release();
    pJPEGImage = NULL;
                 
    if (pStreamIn)
        pStreamIn->Release();
    pStreamIn = NULL;
        
    if (pStreamOut)
        pStreamOut->Release();
    pStreamOut = NULL;
    
    if (pEncodeSink)
        pEncodeSink->Release();
    pEncodeSink = NULL;

    if (pImageDecoder)
        pImageDecoder->Release();
    pImageDecoder = NULL;

    if (pImageEncoder)
        pImageEncoder->Release();
    pImageEncoder = NULL;

    DEBUGMSG(ZONE_ENTER, (TEXT("-CJPEGFilter::Transform(), hr = 0x%x\n"), hr));

    return hr;
}



HRESULT CJPEGFilter::InitImaging()
{
    HRESULT hr;
    
    DEBUGMSG(ZONE_ENTER, (TEXT("+CJPEGFilter::InitImaging()\n")));

    if (FAILED(hr = CoInitializeEx(NULL, COINIT_MULTITHREADED)))
    {
        DEBUGMSG(ZONE_ERROR, (TEXT("CoInitializeEx failed, hr: 0x%08x\r\n"), hr));
        goto Exit;
    }
    
    hr = CoCreateInstance(CLSID_ImagingFactory, 
                          NULL, 
                          CLSCTX_INPROC_SERVER, 
                          IID_IImagingFactory, 
                          (void**) &m_pImagingFactory);
    if (FAILED(hr))
        goto Exit;
        
    //
    // Query current installed codecs
    //
    ImageCodecInfo *pCodecInfo;
    UINT uCnt = 0;
    hr = m_pImagingFactory->GetInstalledDecoders (&uCnt, &pCodecInfo);
    if (SUCCEEDED(hr))
    {
        DEBUGMSG(ZONE_INIT, (TEXT("CJPEGFilter: detected %d decoders:\r\n"), uCnt));
        for (UINT i = 0; i < uCnt; i++)
        {
            DEBUGMSG(ZONE_INIT, (TEXT("\tName  >%s<\r\n"), pCodecInfo[i].CodecName));
            DEBUGMSG(ZONE_INIT, (TEXT("\tDesc  >%s<\r\n"), pCodecInfo[i].FormatDescription));
            DEBUGMSG(ZONE_INIT, (TEXT("\r\n")));
        }
        CoTaskMemFree ((PVOID)pCodecInfo);
    }

    hr = m_pImagingFactory->GetInstalledEncoders (&uCnt, &pCodecInfo);
    if (SUCCEEDED(hr))
    {
        DEBUGMSG(ZONE_INIT, (TEXT("CJPEGFilter: detected %d encoders:\r\n"), uCnt));
        for (UINT i = 0; i < uCnt; i++)
        {
            DEBUGMSG(ZONE_INIT, (TEXT("\tName  >%s<\r\n"), pCodecInfo[i].CodecName));
            DEBUGMSG(ZONE_INIT, (TEXT("\tDesc  >%s<\r\n"), pCodecInfo[i].FormatDescription));
            DEBUGMSG(ZONE_INIT, (TEXT("\r\n")));
        }
        CoTaskMemFree ((PVOID)pCodecInfo);
    }

Exit:    
    DEBUGMSG(ZONE_ENTER, (TEXT("-CJPEGFilter::InitImaging(), hr = 0x%x\n"), hr));

    return hr;
}



//----------------------------------------------------------------------------
// CJPEGFilter::CreateInstance
//
// Static method that returns a new instance of our filter.
// Note: The DirectShow class factory object needs this method.
//
// pUnk: Pointer to the controlling IUnknown (usually NULL)
// pHR:  Set this to an error code, if an error occurs
//-----------------------------------------------------------------------------

CUnknown * WINAPI CJPEGFilter::CreateInstance(LPUNKNOWN pUnk, HRESULT *pHR) 
{
    DEBUGMSG(ZONE_INIT, (TEXT("CJPEGFilter::CreateInstance()")));

    CJPEGFilter *pFilter = new CJPEGFilter(L"JPEG Transform Filter", pUnk, pHR );
    if (pFilter == NULL) 
    {
        *pHR = E_OUTOFMEMORY;
    }
    return pFilter;
} 



//----------------------------------------------------------------------------
// GetVideoInfoParameters
//
// Helper function to get the important information out of a VIDEOINFOHEADER
//
//-----------------------------------------------------------------------------

void GetVideoInfoParameters(
    const VIDEOINFOHEADER *pvih, // Pointer to the format header.
    DWORD *pdwWidth,         // Returns the width in pixels.
    DWORD *pdwHeight        // Returns the height in pixels.
    )
{
    //  If rcTarget is empty, use the whole image.
    if (IsRectEmpty(&pvih->rcTarget)) 
    {
        *pdwWidth = (DWORD)pvih->bmiHeader.biWidth;
        *pdwHeight = (DWORD)(abs(pvih->bmiHeader.biHeight));
        
       
    } 
    else   // rcTarget is NOT empty. Use a sub-rectangle in the image.
    {
        *pdwWidth = (DWORD)(pvih->rcTarget.right - pvih->rcTarget.left);
        *pdwHeight = (DWORD)(pvih->rcTarget.bottom - pvih->rcTarget.top);
        
       
    }
}


//----------------------------------------------------------------------------
// IsValidJPEG
//
//-----------------------------------------------------------------------------
bool IsValidJPEG(const CMediaType *pmt)
{
    if (!pmt)
    {
        return false;   
    }

    // Note: The pmt->formattype member indicates what kind of data
    // structure is contained in pmt->pbFormat. But it's important
    // to check that pbFormat is non-NULL and the size (cbFormat) is
    // what we think it is. 

#ifdef DEBUG
    DisplayType("CJPEGFilter: input type requested", pmt);
#endif

    if ((pmt->majortype == MEDIATYPE_Video) &&
        ((pmt->subtype == MEDIASUBTYPE_IJPG) || 
          pmt->subtype == MEDIASUBTYPE_MJPG))
        //(pmt->formattype == FORMAT_VideoInfo) &&
        //(pmt->pbFormat != NULL) &&
        //(pmt->cbFormat >= sizeof(VIDEOINFOHEADER)))
        {
       
            return true;
        }
   
    DEBUGMSG(ZONE_WARNING, (TEXT("WARNING: CJPEGFilter: rejected illegal input type\n")));

    return false;
}


//----------------------------------------------------------------------------
// IsValidRGB
//
//-----------------------------------------------------------------------------
bool IsValidRGB(const CMediaType *pmt)
{
    if (!pmt)
    {
        return false;
    }

#ifdef DEBUG
    DisplayType("CJPEGFilter: output type requested", pmt);
#endif

    // Note: The pmt->formattype member indicates what kind of data
    // structure is contained in pmt->pbFormat. But it's important
    // to check that pbFormat is non-NULL and the size (cbFormat) is
    // what we think it is. 


    if ((pmt->majortype == MEDIATYPE_Video) &&
        (pmt->subtype == MEDIASUBTYPE_RGB24) )
        //(pmt->formattype == FORMAT_VideoInfo) &&
        //(pmt->pbFormat != NULL) &&
        //(pmt->cbFormat >= sizeof(VIDEOINFOHEADER)))
        {
       
            return true;
        }
   

    DEBUGMSG(ZONE_WARNING, (TEXT("WARNING: CJPEGFilter: rejected illegal output type\n")));

    return false;
}



//----------------------------------------------------------------------------
// GetCodecCLSID
//
//-----------------------------------------------------------------------------
BOOL GetCodecCLSID(IImagingFactory* pImagingFactory, CLSID * pclsid, WCHAR * wszMimeType)
{
    UINT uiCount;
    ImageCodecInfo * codecs;
    BOOL fRet = FALSE;

    HRESULT hr = pImagingFactory->GetInstalledEncoders(&uiCount, &codecs);
    if (FAILED(hr))
        return FALSE;
    
    for (UINT i = 0; i < uiCount; i++)
    {
        DEBUGMSG(ZONE_INFO_MAX, (TEXT("Found codec: [%s]\r\n"), codecs[i].MimeType));
        if (wszMimeType && !wcscmp(wszMimeType, codecs[i].MimeType))
        {
            *pclsid = codecs[i].Clsid;
            fRet = TRUE;
            break;
        }
    }
    CoTaskMemFree(codecs);

    return fRet;
}


STDAPI DllRegisterServer(void)
{
    return AMovieDllRegisterServer2(TRUE);
}


STDAPI DllUnregisterServer()
{
    return AMovieDllRegisterServer2(FALSE);
}

