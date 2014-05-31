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
//
//  File: dmamediator.h
//

#ifndef __DMAMEDIATOR_H
#define __DMAMEDIATOR_H

#include "dmacontroller.h"

//------------------------------------------------------------------------------
//  class DmaMediator
//
//  processes dma interrupts, restarts dma logical channels registered for 
//  repeat mode, and forward interrupts to the secondary interrupt handlers
//
class DmaMediator
{
    //--------------------------------------------------------------------------
    // typedefs and structs
    //
public:
    typedef struct 
    {
        HANDLE                  secondaryEventHandle;
        OMAP_DMA_LC_REGS       *pDmaRegs;
    }
    DmaChannelInfo_t;
        
    
    //--------------------------------------------------------------------------
    // member variables
    //
private:
    BOOL                        m_bExit;


    //--------------------------------------------------------------------------
    // member variables
    //
protected:
    HANDLE                      m_hIST;
    HANDLE                      m_hEvent;
    DWORD                       m_sysIntr;
    DWORD                       m_ffSIHList;
    int                         m_dmaChannelCount;
    DmaChannelInfo_t           *m_pDmaChannelInfo;
    HANDLE                      m_context;
    DmaControllerBase          *m_pDmaController;
    

    //--------------------------------------------------------------------------
    // constructor
    //
public:
    DmaMediator() : m_sysIntr(0), m_hIST(NULL), m_ffSIHList(0), 
                    m_pDmaChannelInfo(NULL), m_dmaChannelCount(0), 
                    m_pDmaController(NULL)
    {
    }


    //--------------------------------------------------------------------------
    // internal methods
    //
protected:


    //--------------------------------------------------------------------------
    // internal static methods
    //
protected:

    //--------------------------------------------------------------------------
    // InterruptHandler
    //  handles the interrupt associated with the dma interrupt line
    //
    static DWORD InterruptHandler(void* pData);
    

    //--------------------------------------------------------------------------
    // internal methods
    //
protected:

    //--------------------------------------------------------------------------
    // EnableInterrupt
    //  enables/disables interrupts for a logcial channel
    //
    virtual void EnableInterrupt(int index, BOOL bEnable)
    {
        // check for state change
        BOOL bStateChange = m_ffSIHList & (1 << index);
        bStateChange ^= bEnable;
                
        if (bStateChange == TRUE)
            {
            m_pDmaController->EnableInterrupt(index, bEnable);
            }
    }

    //--------------------------------------------------------------------------
    // ProcessInterrupt
    //  called an interrupt for the given channel occurred.  Processing time
    //  should be short as the call will block processing of other dma
    //  interrupt handlers
    //
    virtual BOOL ProcessInterrupt();


    //--------------------------------------------------------------------------
    // public methods
    //
public:
    //--------------------------------------------------------------------------
    // Initialize
    //  Initializes the class
    //
    BOOL Initialize(int count, OMAP_DMA_LC_REGS pDmaRegs[]);  

    //--------------------------------------------------------------------------
    // Uninitialize
    //  releases all resources
    //
    void Uninitialize();

    //--------------------------------------------------------------------------
    // StartInterruptHandler
    //  instantiates the necessary resources and listens for dma interrupts
    //
    BOOL StartInterruptHandler(DWORD irq, DWORD priority);

    //--------------------------------------------------------------------------
    // StopInterruptHandler
    //  stops the dma thread and releases all relevant resources
    //
    void StopInterruptHandler();  

    //--------------------------------------------------------------------------
    // SetEventHandler
    //  maps an event handle to a dma interrupt index
    //
    BOOL SetEventHandler(int index, HANDLE hEvent, DWORD processId);

    //--------------------------------------------------------------------------
    // ResetChannel
    //  resets a dma channel
    //
    BOOL ResetChannel(int index);

    //--------------------------------------------------------------------------
    // SetDmaController
    //  initializes a reference to the dma controller to query for dma
    //  interrupt mask
    //
    void SetDmaController(HANDLE context, DmaControllerBase *pDmaController);

    //--------------------------------------------------------------------------
    // GetLastInterruptStatus
    //  returns the dma status when it last generated an interrupt
    //
    BOOL InterruptDone(int index) 
    { 
        BOOL bReset = FALSE;
        if (m_ffSIHList & (1 << index))
            {
            m_pDmaController->EnableInterrupt(index, TRUE); 
            bReset = TRUE;
            }
        return bReset;
    }
};




#endif // __DMAOBSERVER_H
