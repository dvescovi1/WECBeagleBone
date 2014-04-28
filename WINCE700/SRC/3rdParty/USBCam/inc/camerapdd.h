//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this sample source code is subject to the terms of the Microsoft
// license agreement under which you licensed this sample source code. If
// you did not accept the terms of the license agreement, you are not
// authorized to use this sample source code. For the terms of the license,
// please see the license agreement between you and Microsoft or, if applicable,
// see the LICENSE.RTF on your install media or the root of your tools installation.
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//

#ifndef _CAMERAPDD_H
#define _CAMERAPDD_H

#ifdef __cplusplus
extern "C" {
#endif

    /////////////////////////////////////////////////////////////////////////////////////
    // The following methods are corresponding to the adapter

    // PDD_Init is called when the driver is first loaded. MDDContext is what MDD receive in its XXX_Init call.
    // PDD needs to return a context which the MDD will later send back in the rest of the PDD functions.
    // PDD will also populate pPDDFuncTbl which is a list of entry points the PDD implements.
    PVOID PDD_Init( PVOID MDDContext, PPDDFUNCTBL pPDDFuncTbl );

    // PDD_DeInit is called when the driver is being Unloaded. At this point PDD should free up any 
    // unfreed resources.
    DWORD PDD_DeInit( LPVOID PDDContext );

    // PDD_GetAdapterInfo is called at the initialization time to get information about the Adapter
    DWORD PDD_GetAdapterInfo( LPVOID PDDContext, PADAPTERINFO pAdapterInfo );

    // Handle VideoProcAmp specific property changes.
    DWORD PDD_HandleVidProcAmpChanges( LPVOID PDDContext, DWORD dwPropId, LONG lFlags, LONG lValue);

    // Handle CameraControl specific property changes.
    DWORD PDD_HandleCamControlChanges( LPVOID PDDContext, DWORD dwPropId, LONG lFlags, LONG lValue );
    
    // Handle CSPROPERTY_VIDEOCONTROL_CAPS property.
    DWORD PDD_HandleVideoControlCapsChanges( LPVOID PDDContext, LONG lModeType ,ULONG ulCaps );

    // Set driver power state.
    DWORD PDD_SetPowerState( LPVOID PDDContext, CEDEVICE_POWER_STATE PowerState );

    // Handle Adapter level custom/proprietary properties
    DWORD PDD_HandleAdapterCustomProperties( LPVOID PDDContext, PUCHAR pInBuf, DWORD  InBufLen, PUCHAR pOutBuf, DWORD  OutBufLen, PDWORD pdwBytesTransferred );
   
    
    // End of adapter specific methods
    /////////////////////////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////////////////////////
    // The following methods are corresponding to sensor modes. This is why the 2nd parameter
    // of all these methods is the Mode Type.

    // PDD_InitSensorMode() is called when the MDD wants PDD to allocate all the resources related 
    // to a given mode ulModeType. Any Buffer allocation or DMA setup should be done in this
    // function.
    DWORD PDD_InitSensorMode( LPVOID PDDContext, ULONG ulModeType, LPVOID ModeContext );

    // PDD_DeInitSensorMode() is called when the MDD wants PDD to deallocate all the resources 
    // related to a given mode ulModeType.
    DWORD PDD_DeInitSensorMode( LPVOID PDDContext, ULONG ulModeType );

    // There are three valid states, i.e., CSSTATE_STOP, CSSTATE_PAUSE, CSSTATE_RUN. 
    // PDD_SetSensorState() is only called for Preview or Capture sensor modes. For Still sensor mode, 
    // PDD_TakeStillPicture (discussed further below) is called.
    // PDD should only capture data when put in CSSTATE_RUN.
    DWORD PDD_SetSensorState( LPVOID PDDContext, ULONG ulModeType, CSSTATE CsState );

    // PDD_TakeStillPicture is called to take still picture. The 2nd parameter pBurstModeInfo is not
    // used in this version. MDD will always set it to NULL.
    // When the PDD receives this function call, it should setup the sensor for still mode. It may have to 
    // disable sending data for Capture or preview sensor mode.
    DWORD PDD_TakeStillPicture( LPVOID PDDContext, LPVOID pBurstModeInfo );

    // PDD_GetSensorModeInfo is currently called during the instantiation time but this can be called from any place.
    DWORD PDD_GetSensorModeInfo( LPVOID PDDContext, ULONG ulModeType, PSENSORMODEINFO pSensorModeInfo );

    // PDD_SetSensorModeFormat() is called to set the format of a particular sensor mode i.e., ulModeType. PDD must not
    // allocate resources at this time. Instead it should wait for PDD_InitSensorMode to do the actual allocation.
    DWORD PDD_SetSensorModeFormat( LPVOID PDDContext, ULONG ulModeType, PCS_DATARANGE_VIDEO pCsDataRangeVideo );
    
    // PDD_AllocateBuffer is called when the MDD wants PDD to allocate its Mode specific buffer. This method is called
    // n times where n is the total number of buffers that the application agreed with driver at the time of initialization
    // Each mode (Capture, Still etc.)will have its own set of buffers. This method is only called if the Memory Model 
    // is CSPROPERTY_BUFFER_DRIVER
    PVOID PDD_AllocateBuffer( LPVOID PDDContext, ULONG ulModeType );

    // PDD_DeAllocateBuffer is called when the MDD wants PDD to deallocate its Mode specific buffer. MDD will call
    // this function n times where n is the total number of buffers agreed by the driver and Application(DShow)
    // This method is only called if the Memory Model is CSPROPERTY_BUFFER_DRIVER
    DWORD PDD_DeAllocateBuffer( LPVOID PDDContext, ULONG ulModeType, PVOID pBuffer );

    // PDD_RegisterClientBuffer is called by MDD to give PDD the pointers to buffers allocated by the 
    // application(DShow). MDD will call this function n times where n is the total number of buffers 
    // agreed by the driver and Application(DShow)At this point, PDD can setup DMA using these pointers.
    // This method is only called if the Memory Model IS NOT CSPROPERTY_BUFFER_DRIVER
    DWORD PDD_RegisterClientBuffer( LPVOID PDDContext, ULONG ulModeType, PVOID pBuffer );

    // PDD_UnRegisterClientBuffer is called by MDD to tell PDD that the particular pointer is no more 
    // available. PDD must not used this pointer after this function call.
    // This method is only called if the Memory Model IS NOT CSPROPERTY_BUFFER_DRIVER
    DWORD PDD_UnRegisterClientBuffer( LPVOID PDDContext, ULONG ulModeType, PVOID pBuffer );

    // When the PDD receives an interrupt for sensor data availabilitly, it will call MDD_HandleIO() 
    // implemented by the MDD. MDD_HandleIO() will internally call PDD_FillBuffer() to to let PDD
    // transfer/DMA data to the buffer. MDD_HandleIO 
    DWORD PDD_FillBuffer( LPVOID PDDContext, ULONG ulModeType, PUCHAR pImage );

    // PDD_HandleModeCustomProperties handles Sensor Mode specific Custom/Proprietary properties
    DWORD PDD_HandleModeCustomProperties( LPVOID PDDContext, ULONG ulModeType, PUCHAR pInBuf, DWORD  InBufLen, PUCHAR pOutBuf, DWORD  OutBufLen, PDWORD pdwBytesTransferred );

    // End of Mode specific methods
    /////////////////////////////////////////////////////////////////////////////////////
  

#ifdef __cplusplus
}
#endif

#endif
