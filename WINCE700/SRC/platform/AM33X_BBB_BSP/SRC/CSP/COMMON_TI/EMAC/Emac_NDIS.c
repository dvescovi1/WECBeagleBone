//
// Copyright (c) Texas Instruments Incorporated 2010. All Rights Reserved.
//
//------------------------------------------------------------------------------

//! \file Emac_NDIS.c
//! \brief Contains NDIS Miniport entry functions.
//!
//! This source file contains the DriverEntry which is being exposed by 
//! miniport driver.
//!
//!
//! \version  1.00 Created on Aug 22nd 2006

// Includes
#include <Ndis.h>
#include "Emac_Adapter.h"
#include "Emac_Proto.h"
#include "Emac_Queue.h"

#ifdef DEBUG
DBGPARAM dpCurSettings = 
{
  TEXT("Emac"), 
  {
    TEXT("Init"),TEXT("Critical"),TEXT("Interrupt"),TEXT("Message"),
    TEXT("Send"),TEXT("Receive"),TEXT("Info"),TEXT("Function"),
    TEXT("Oid"),TEXT(""),TEXT(""),TEXT(""),
    TEXT(""),TEXT(""),TEXT("Warnings"), TEXT("Errors")
  },
    0xC001
};

#endif  // DEBUG



//========================================================================
//!  \fn NDIS_STATUS DriverEntry(PVOID  DriverObject, PVOID  RegistryPath)
//!  \brief It is a required function that the system calls first in any NDIS driver. 
//!  \param DriverObject PVOID Points to a system-supplied parameter
//!  \param RegistryPathcount PVOID Points to a second system-supplied parameter
//!  \return NDIS_STATUS Returns success or error states.
//========================================================================

NDIS_STATUS    
DriverEntry(
    PVOID  DriverObject, 
    PVOID  RegistryPath
    )
{
    NDIS_STATUS                   Status;
    NDIS_HANDLE                   NdisWrapperHandle;
    NDIS_MINIPORT_CHARACTERISTICS EmacChar;

    DEBUGMSG(DBG_FUNC, (L"---> DriverEntry\r\n"));

    /*  Notify the  NDIS library that driver is about to register itself as
     *  a miniport. NDIS sets up the state it needs to track the driver and
     *  returns an NDISWrapperHandle which driver uses for subsequent calls.
     */
    
     NdisMInitializeWrapper(
        &NdisWrapperHandle,
        DriverObject,
        RegistryPath,
        NULL);

    if (NdisWrapperHandle == NULL)
    {
        Status = NDIS_STATUS_FAILURE;

        DEBUGMSG(Status, (L"<--- DriverEntry failed to InitWrapper, Status=%x \r\n", Status));
        return Status;
    }

    /* Fill in the Miniport characteristics structure with the version numbers 
     * and the entry points for driver-supplied MiniportXxx 
     */
     
    NdisZeroMemory(&EmacChar, sizeof(EmacChar));

    EmacChar.MajorNdisVersion         = EMAC_NDIS_MAJOR_VERSION;
    EmacChar.MinorNdisVersion         = EMAC_NDIS_MINOR_VERSION;

    EmacChar.CheckForHangHandler      = Emac_MiniportCheckForHang;
    EmacChar.DisableInterruptHandler  = Emac_MiniportDisableInterrupt;
    EmacChar.EnableInterruptHandler   = NULL;
    EmacChar.HaltHandler              = Emac_MiniportHalt;
    EmacChar.HandleInterruptHandler   = Emac_MiniportHandleInterrupt;  
    EmacChar.InitializeHandler        = Emac_MiniportInitialize;
    EmacChar.ISRHandler               = Emac_MiniportIsr;  
    EmacChar.QueryInformationHandler  = Emac_MiniportQueryInformation;
    EmacChar.ReconfigureHandler       = NULL;  
    EmacChar.ResetHandler             = Emac_MiniportReset;
    EmacChar.SendHandler              = NULL;
    EmacChar.SetInformationHandler    = Emac_MiniportSetInformation;
    EmacChar.TransferDataHandler      = NULL;  
    EmacChar.ReturnPacketHandler      = Emac_MiniportReturnPacket;
    EmacChar.SendPacketsHandler       = Emac_MiniportSendPacketsHandler;
    EmacChar.AllocateCompleteHandler  = NULL;
    
    DEBUGMSG(TRUE, (L"Calling NdisMRegisterMiniport with NDIS_MINIPORT_MAJOR_VERSION %d" \
                     L"& NDIS_MINIPORT_MINOR_VERSION %d\r\n",
                      NDIS_MINIPORT_MAJOR_VERSION,NDIS_MINIPORT_MINOR_VERSION));
    
    /* Calling NdisMRegisterMiniport causes driver's MiniportInitialise
     * function to run in the context of NdisMRegisterMiniport.
     */
     
    
    Status = NdisMRegisterMiniport(
                 NdisWrapperHandle,
                 &EmacChar,
                 sizeof(NDIS_MINIPORT_CHARACTERISTICS));
    
    if(Status != NDIS_STATUS_SUCCESS)
    {
        /* Call NdisTerminateWrapper, and return the error code to the OS. */
        NdisTerminateWrapper(
            NdisWrapperHandle, 
            NULL);          /* Ignored */            
    }
        
    DEBUGMSG(DBG_INFO, (L"<--- DriverEntry, Status= %x\r\n", Status));

    return Status;
    
}

//========================================================================
//!  \fn BOOL WINAPI DllMain( HINSTANCE DllInstance, DWORD Reason, LPVOID Reserved)
//!  \brief This function is entry point for the DLL.
//!  \param DllInstance HINSTANCE Handle to the Dll.
//!  \param Reason DWORD Specifies a flag indicating why the DLL entry-point 
//!                         function is being called
//!  \param Reserved LPVOID Specifies further aspects of DLL initialization 
//!                         and cleanup
//!  \return BOOL indicating the status.Here returns TRUE.
//========================================================================
BOOL WINAPI 
DllMain(
    HINSTANCE DllInstance,
    DWORD Reason,
    LPVOID Reserved
    )
{
    switch (Reason)
    {
        case DLL_PROCESS_ATTACH:
                DEBUGREGISTER(DllInstance);
                DEBUGMSG(DBG_INIT, (L"EmacMiniport : DLL Process Attach\r\n"));
                DisableThreadLibraryCalls((HMODULE) DllInstance);
                break;
                
        case DLL_PROCESS_DETACH:
                DEBUGMSG(DBG_INIT, (L"EmacMiniport :  DLL Process Detach\r\n"));
                break;
    }
    return TRUE;
}
