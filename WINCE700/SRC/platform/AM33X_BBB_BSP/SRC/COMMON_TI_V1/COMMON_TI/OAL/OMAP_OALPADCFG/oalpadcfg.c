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
//  File:  oal_padcfg.c
//

#include "omap.h"
#include "oalex.h"
#include "sdk_padcfg.h"
#include "soc_cfg.h"
#include "bsp_cfg.h"

static BOOL _PostInit = FALSE;
static CRITICAL_SECTION _cs;
static int g_NbPads;
static PAD_INFO* g_bspPadInfo;

static int SortPadInfoArray(PAD_INFO* padArray);
static int FindPad(UINT16 padId);

BOOL OALPadCfgInit()
{
    g_bspPadInfo = BSPGetAllPadsInfo();
    g_NbPads = SortPadInfoArray(g_bspPadInfo); //sort the array to have better lookup performances
    if (g_bspPadInfo)
    {

        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

BOOL OALPadCfgPostInit()
{
    InitializeCriticalSection(&_cs);

	_PostInit = TRUE;

    return TRUE;
}
static void PadCfgLock()
{
    if (_PostInit)
    {
        EnterCriticalSection(&_cs);
    }
}
static void PadCfgUnlock()
{
    if (_PostInit)
    {
        LeaveCriticalSection(&_cs);
    }
}

BOOL RequestPad(UINT16 padid)
{    
    BOOL rc = FALSE;
    int padIndex;
    padIndex = FindPad(padid);
    if (padIndex == -1) 
    {
        return FALSE;
    }
    
    if (padIndex < g_NbPads)
    {
        PadCfgLock();
        if (!g_bspPadInfo[padIndex].inUse)
        {
            g_bspPadInfo[padIndex].inUse = 1;
            rc = TRUE;
        }
        PadCfgUnlock();
    }
    return rc;
}

BOOL ReleasePad(UINT16 padid)
{
    BOOL rc = FALSE;
    int padIndex;
    padIndex = FindPad(padid);
    if (padIndex == -1) 
    {
        return FALSE;
    }
    
    if (padIndex < g_NbPads)
    {
        PadCfgLock();
        if (g_bspPadInfo[padIndex].inUse)
        {
            g_bspPadInfo[padIndex].inUse = 0;
            SOCSetPadConfig(padid,(UINT16) g_bspPadInfo[padIndex].Cfg); // configure the PIn with its default unused configuration
            rc = TRUE;
        }
        PadCfgUnlock();
    }
    return rc;
}

BOOL ConfigurePad(UINT16 padId,UINT16 cfg)
{
    BOOL rc = FALSE;    
    int padIndex;
    padIndex = FindPad(padId);
    if (padIndex == -1) 
    {
        return FALSE;
    }
    
    if (padIndex < g_NbPads)
    {
        PadCfgLock();
        if (g_bspPadInfo[padIndex].inUse)
        {            
            rc = SOCSetPadConfig(padId,cfg);            
        }
        PadCfgUnlock();
    }
    return rc;
}

BOOL GetPadConfiguration(UINT16 padId,UINT16* pCfg)
{
    BOOL rc = FALSE;    
    int padIndex;
    
    if (pCfg == NULL)
    {
        return FALSE;
    }

    padIndex = FindPad(padId);
    if (padIndex == -1) 
    {
        return FALSE;
    }
    
    if (padIndex < g_NbPads)
    {
        PadCfgLock();
        if (g_bspPadInfo[padIndex].inUse)
        {            
            rc = SOCGetPadConfig(padId,pCfg);            
        }
        PadCfgUnlock();
    }
    return rc;
}
BOOL RequestAndConfigurePad(UINT16 padId,UINT16 cfg)
{
    BOOL rc = FALSE;    
    int padIndex;
    padIndex = FindPad(padId);
    if (padIndex == -1) 
    {
        return FALSE;
    }
    
    if (padIndex < g_NbPads)
    {
        PadCfgLock();
        if (!g_bspPadInfo[padIndex].inUse)
        {                
            rc = SOCSetPadConfig(padId,cfg);            
            if (rc) 
            {
                g_bspPadInfo[padIndex].inUse = 1;
            }
        }
        PadCfgUnlock();
    }
    return rc;
}


// NOTE : this function is not optimized at all. It's intended to be used very seldom
BOOL RequestAndConfigurePadArray(const PAD_INFO* padArray)
{
    BOOL rc = TRUE;
    int i = 0;
    
    PadCfgLock();
    // Check that all pads are valid and released
    i=0;
    while (rc && (padArray[i].padID != (UINT16) -1))
    {
        int padIndex;
        padIndex = FindPad(padArray[i].padID);
        if (padIndex == -1) 
        {
            goto error;
        }
        if (padIndex >= g_NbPads)
        {
            ASSERT(0);
            goto error;
        }
        if (g_bspPadInfo[padIndex].inUse)
        {                
            goto error;
        }
        i++;
    }

    // Request and Configure all pads
    i = 0;
    while (rc && (padArray[i].padID != (UINT16) -1))
    {
        int padIndex;
        padIndex = FindPad(padArray[i].padID);
        g_bspPadInfo[padIndex].inUse = 1;
        SOCSetPadConfig(padArray[i].padID,(UINT16)padArray[i].Cfg);     
        i++;
    }

    PadCfgUnlock();
    return TRUE;

error:
    PadCfgUnlock();
    return FALSE;
}

BOOL ReleasePadArray(const PAD_INFO* padArray)
{   
    BOOL rc = TRUE;
    int i = 0;
        
    // Check that all pads are valid and released
    i=0;
    while (rc && (padArray[i].padID != (UINT16) -1))
    {
       rc = ReleasePad(padArray[i].padID);
	   i++;
    }    
    return rc;
}

static int SortPadInfoArray(PAD_INFO* padArray)
{
    int i,j;
    int nbPad = 0;
    while (padArray[nbPad].padID != (UINT16) -1)
    {
        nbPad++;
    }
    //simple bubble sorting
    for(i=0;i<(nbPad);i++)
    {
        for(j=0;j<(nbPad-1)-i;j++)
        {
            if(padArray[j].padID > padArray[j+1].padID)
            {
                PAD_INFO temp;
                temp = padArray[j];
                padArray[j] = padArray[j+1];
                padArray[j+1] = temp;
            }
        }
    }
    return nbPad;
}

// simple dichotomic search in the sorted pad array
static int FindPad(UINT16 padId)
{    
    int index = 0;
    int inf = 0;
    int sup = g_NbPads-1;
    for(;;)
    {
        index = (inf+sup) / 2;
        if (inf == sup)
        {
            //whatever the result this is the end of the search
            break;
        }
        if (g_bspPadInfo[index].padID < padId)
        {
            inf = index+1;
        }
        else if (g_bspPadInfo[index].padID > padId)
        {
            sup = index-1;
        }
        else // (g_bspPadInfo[index].padID == padId)
        {
            return index;
        } 
    }
    if (g_bspPadInfo[index].padID == padId)
    {        
            return index;
    }
    ERRORMSG(1,(TEXT("PAD %d not found in pad configuration array\r\n"),padId));
    return -1;
}

BOOL RequestDevicePads(OMAP_DEVICE device)
{
    BOOL rc = FALSE;
    const PAD_INFO* p = BSPGetDevicePadInfo(device);

    if (p)
    {
        rc = RequestAndConfigurePadArray(p);
    }

    return rc;
}
BOOL ReleaseDevicePads(OMAP_DEVICE device)
{
    BOOL rc = FALSE;
    const PAD_INFO* p = BSPGetDevicePadInfo(device);
    
    if (p)
    {
        rc = ReleasePadArray(p);
    }

    return rc;
}