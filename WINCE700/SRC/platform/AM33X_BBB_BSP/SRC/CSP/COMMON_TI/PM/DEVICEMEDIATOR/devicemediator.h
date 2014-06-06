// All rights reserved ADENEO EMBEDDED 2010
/*
===============================================================================
*             Texas Instruments OMAP(TM) Platform Software
* (c) Copyright Texas Instruments, Incorporated. All Rights Reserved.
*
* Use of this software is controlled by the terms and conditions found
* in the license agreement under which this software has been supplied.
*
===============================================================================
*/
//
//  File: %FILE%
//


#if !defined(EA_7C1AF7F4_054C_487c_9ACF_659061E3F75C__INCLUDED_)
#define EA_7C1AF7F4_054C_487c_9ACF_659061E3F75C__INCLUDED_


#include "DeviceBase.h"
#include "contextlist.h"


//-----------------------------------------------------------------------------
/**
 * - Listens to the loaded devices
 * - Maintains a collection of loaded devices
 * - Notifies registers device list objects when drivers are loaded.
 * - 
 */
class DeviceMediator
{
public:
    //-------------------------------------------------------------------------
    // typedefs
    //-------------------------------------------------------------------------

protected:
    //-------------------------------------------------------------------------
    // member variables
    //-------------------------------------------------------------------------
    
    CRITICAL_SECTION                        m_cs;
    DeviceBase                             *m_pDeviceBaseHead;
    ContextList                            *m_pContextListHead;

public:
    //-------------------------------------------------------------------------
    // public methods
    //-------------------------------------------------------------------------    
    static DeviceMediator* CreateDeviceMediator();
    static void DeleteDeviceMediator(DeviceMediator *pDeviceMediator);

protected:
    //-------------------------------------------------------------------------
    // constructor/destructor
    //-------------------------------------------------------------------------
    DeviceMediator():m_pDeviceBaseHead(NULL), m_pContextListHead(NULL)
        {
        }

    void Lock()
        { 
        ::EnterCriticalSection(&m_cs); 
        }
    
    void Unlock()
        { 
        ::LeaveCriticalSection(&m_cs); 
        }

protected:
    //-------------------------------------------------------------------------
    // constructor/destructor
    //-------------------------------------------------------------------------
    BOOL Initialize();
    BOOL Uninitialize();

public:
    //-------------------------------------------------------------------------
    // public methods
    //-------------------------------------------------------------------------    
    BOOL SendMessage(UINT listId, UINT param, 
            DWORD idRequest, LPVOID pInBuf, DWORD dwInSize, LPVOID pOutBuf, 
            DWORD dwOutSize, LPDWORD pdwBytesRet
            );

    BOOL InsertDevice(LPCTSTR szDeviceName, DeviceBase *pDevice);
    DeviceBase* FindDevice(LPCTSTR szDevice);
    DeviceBase* RemoveDevice(LPCTSTR szDevice);

};

#endif // !defined(EA_7C1AF7F4_054C_487c_9ACF_659061E3F75C__INCLUDED_)

//-----------------------------------------------------------------------------

