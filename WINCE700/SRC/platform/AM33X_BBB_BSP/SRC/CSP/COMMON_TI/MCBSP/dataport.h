// All rights reserved ADENEO EMBEDDED 2010
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
//  File: dataport.h
//

#include <omap_sdma_utility.h>

#include "debug.h"
#include "mcbsptypes.h"


//------------------------------------------------------------------------------
//  DataPort_t class
//
class DataPort_t
{
// typedefs and enums
//---------------------------
//
public:
    typedef enum
    {
        kBufferStart,
        kBufferMiddle,
        kBufferActive,
        kBufferInactive
    }
    BufferRequest_e;

// protected member variables
//---------------------------
//
protected:
    DmaDataInfo_t       m_DmaInfo;
    McBSPDevice_t      *m_pDevice;
    INT                 m_DmaLoopCounter;
    McBSPPortState_e    m_PortState;
    CRITICAL_SECTION    m_cs;
    DWORD               m_DmaPhysAddr;
    BYTE               *m_pActiveDmaBuffer;
    BYTE               *m_pDmaBufferStart;
    BYTE               *m_pDmaBufferMiddle;
    INT                 m_sizeDmaBuffer;
    INT                 m_SamplesPerPage;
    McBSPInstance_t    *m_pActiveInstance;

// public member variables
//---------------------------
//
public:
    HANDLE              m_hEvent;       // DMA interrupt handle
    HANDLE              m_hDmaChannel;  // DMA channel

// constructor/destructor
//-----------------------
//
public:
    DataPort_t(McBSPDevice_t *pDevice);
    virtual ~DataPort_t();

// inline methods
//-----------------------
//
public:
    void SetMaxLoopCount()
    {
        m_DmaLoopCounter = DMA_SAFETY_LOOP_NUM;
    }

    INT GetLoopCount() const
    {
        return m_DmaLoopCounter;
    }

    INT GetSamplesPerPage() const
    {
        return m_SamplesPerPage;
    }

    INT DecrementLoopCount()
    {
        return --m_DmaLoopCounter;
    }

    DmaDataInfo_t* GetDmaInfo()
    {
        return &m_DmaInfo;
    }

    McBSPPortState_e GetState() const
    {
        return m_PortState;
    }

    void Lock()    {}
    void Unlock()  {}

    virtual INT GetDataBufferSize() const
    {
        return m_sizeDmaBuffer;
    }

    virtual void ResetDataBuffer()
    {
        m_pActiveDmaBuffer = m_pDmaBufferStart;
    }

    void  RequestDmaStop()
    {
        if (m_DmaLoopCounter) m_DmaLoopCounter = 1;
    }

    McBSPInstance_t* GetActiveInstance() const
    {
        return m_pActiveInstance;
    }

    void SetActiveInstance(McBSPInstance_t *pInstance)
    {
        m_pActiveInstance = pInstance;
    }

    virtual void UpdateSamplesPerPage(DWORD nDmaBufferSize, DWORD datatype)
    {
        if (datatype == DMA_CSDP_DATATYPE_S32)
            {
            m_SamplesPerPage = (nDmaBufferSize >> 1) / sizeof(DWORD);
            }
        else if (datatype == DMA_CSDP_DATATYPE_S16)
            {
            m_SamplesPerPage = (nDmaBufferSize >> 1) / sizeof(WORD);
            }
        else if (datatype == DMA_CSDP_DATATYPE_S8)
            {
            m_SamplesPerPage = nDmaBufferSize / sizeof(WORD);
            }
    }

// public methods
//---------------
//
public:

    void SetDstPhysAddr(DWORD PhysAddr);
    void SetSrcPhysAddr(DWORD PhysAddr);

    virtual BOOL StopDma();
    virtual BOOL StartDma(BOOL bTransmitMode);

    virtual BOOL Initialize(DmaConfigInfo_t *pDmaConfigInfo,
                            DWORD nBufferSize,
                            UINT16 dmaSyncMap,
                            LPTHREAD_START_ROUTINE pIstDma);
    virtual BOOL RestoreDMAcontext(DmaConfigInfo_t *pDmaConfigInfo,
                                   DWORD nBufferSize,
                                   UINT16 dmaSyncMap);

    virtual BYTE* GetDataBuffer(BufferRequest_e type);
    virtual void SwapBuffer(BOOL bTransmitMode);

    virtual void PreprocessDataForRender(BufferRequest_e type, UINT count)  { UNREFERENCED_PARAMETER(type); UNREFERENCED_PARAMETER(count); };
    virtual void PostprocessDataForCapture(BufferRequest_e type, UINT count){ UNREFERENCED_PARAMETER(type); UNREFERENCED_PARAMETER(count); };

};

//------------------------------------------------------------------------------

