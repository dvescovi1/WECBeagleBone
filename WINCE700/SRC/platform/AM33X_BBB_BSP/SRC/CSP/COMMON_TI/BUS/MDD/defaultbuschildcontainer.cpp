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
#include <windows.h>
#include <DefaultBusChildContainer.h>
#include <Registry.hxx>
#include <vector.hxx>

//------------------------------------------------------------------------------
//
//  Constructor:  DefaultBusChildContainer_t
//
DefaultBusChildContainer_t::
DefaultBusChildContainer_t(
    BusDriver_t *pBus
    ) : m_pBus(pBus)
{
    // We get copy of pointer
    m_pBus->AddRef();
}

//------------------------------------------------------------------------------
//
//  Destructor:  DefaultBusChildContainer_t
//
DefaultBusChildContainer_t::
~DefaultBusChildContainer_t(
    )
{
    // We need to release reference
    m_pBus->Release();
}

//------------------------------------------------------------------------------
//
//  Method:  DefaultBusChildContainer_t::Init
//
BOOL
DefaultBusChildContainer_t::
Init(
    LPCWSTR busRegistryKey
    )
{
    BOOL rc = FALSE;
    HKEY hKey = NULL, hChildKey;
    LONG code;
    DWORD subKeys;
    ce::wstring keyName;
    BusDriverChild_t* pChild;
    ce::vector<DWORD> orders;
    DWORD order;


    // Open template registry key
    code = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE, busRegistryKey, 0, 0, &hKey
        );
    if (code != ERROR_SUCCESS)
        {
        goto cleanUp;
        }

    // Find number of subkeys
    code = ce::RegQueryInfoKeySubKeys(hKey, subKeys);
    if (code != ERROR_SUCCESS)
        {
        goto cleanUp;
        }

    for (DWORD index = 0; index < subKeys; index++)
        {
        // Get subkey name
        code = ce::RegEnumKey(hKey, index, keyName);
        if (code != ERROR_SUCCESS)
            {
            continue;
            }
        // Get full name
        keyName.insert(0, L"\\");
        keyName.insert(0, busRegistryKey);                

        // Create child device object
        pChild = m_pBus->CreateBusDeviceChild(keyName);
        if (pChild == NULL)
            {
            continue;
            }

        // Initialize child device object
        if (!pChild->Init(index, keyName))
            {
            // This will result in delete
            pChild->Release();
            continue;
            }

        // Get load order information
        code = RegOpenKeyEx(
            HKEY_LOCAL_MACHINE, keyName, 0, 0, &hChildKey
            );
        if (code != ERROR_SUCCESS)
            {
            // This will result in delete
            pChild->Release();
            continue;
            }
        // Get order value or use default one
        ce::RegQueryValue(hChildKey, L"Order", order, 0xFFFFFFFF);
        RegCloseKey(hChildKey);
        
        // Add child object to array based on order 
        iterator ppChild = begin();
        iterator ppChildEnd = end();
        ce::vector<DWORD>::iterator pOrder = orders.begin();
        while ((ppChild != ppChildEnd) && (*pOrder <= order))
            {
            ppChild++;
            pOrder++;
            }

        // Insert 
        insert(ppChild, pChild);
        orders.insert(pOrder, order);
        
    }
    
    rc = TRUE;
    
cleanUp:
    if (hKey != NULL) RegCloseKey(hKey);
    return rc;
}

//------------------------------------------------------------------------------
//
//  Method:  DefaultBusChildContainer_t::FindChildByName
//
BusDriverChild_t*
DefaultBusChildContainer_t::
FindChildByName(
    LPCWSTR deviceName
    )
{
    BusDriverChild_t *pChild = NULL;
    DWORD offset;

    offset = (wcsnicmp(deviceName, L"$bus\\", 5) == 0) ? 5 : 0;

    // Look for device by walking through list
    for (iterator ppChild = begin(); ppChild != end(); ppChild++)
        {
        LPCWSTR childName = (*ppChild)->DeviceName();

        if (wcsicmp(childName, &deviceName[offset]) == 0) 
            {
            // Following the rule of COM.  Since we are returning a 
            // pointer to an object we need to perform the addref
            // and client needs to perform the release.
            pChild = *ppChild;
            pChild->AddRef();
            break;
            }
        }
        
    return pChild;
}

//------------------------------------------------------------------------------
//
//  Method:  DefaultBusChildContainer_t::FindChildByDeviceId
//
BusDriverChild_t*
DefaultBusChildContainer_t::
FindChildByDriverId(
    UINT driverId
    )
{
    BusDriverChild_t *pChild = NULL;

    // Look for device by walking through list
    for (iterator ppChild = begin(); ppChild != end(); ppChild++)
        {
        if ((*ppChild)->DriverId() == driverId) 
            {
            // Following the rule of COM.  Since we are returning a 
            // pointer to an object we need to perform the addref
            // and client needs to perform the release.
            pChild = *ppChild;
            pChild->AddRef();
            break;
            }
        }
        
    return pChild;
}

//------------------------------------------------------------------------------
//
//  Method:  DefaultBusChildContainer_t::RemoveChild
//
void
DefaultBusChildContainer_t::
RemoveChild(
    BusDriverChild_t *pChild
    )
{
    iterator it = begin();
    while(it != end())
        {
        if (*it == pChild)
            {
            erase(it);
            pChild->Release();
            break;
            }
        ++it;
        }
}


//------------------------------------------------------------------------------

