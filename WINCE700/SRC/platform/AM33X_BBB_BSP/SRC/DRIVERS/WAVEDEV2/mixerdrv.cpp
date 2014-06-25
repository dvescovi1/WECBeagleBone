//
// Copyright (c) MPC-Data Limited 2007.  All rights reserved.
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
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES.
//
// -----------------------------------------------------------------------------
//
//      THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//      ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//      THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//      PARTICULAR PURPOSE.
//
// -----------------------------------------------------------------------------
//
//  @doc    WDEV_EXT
//
//  @module mixerdrv.cpp | Implements the WODM_XXX and WIDM_XXX messages that are
//          passed to the wave audio driver via the <f WAV_IOControl> function.
//          This module contains code that is common or very similar between
//          input and output functions.
//
//  @xref   <t Wave Input Driver Messages> (WIDM_XXX) <nl>
//          <t Wave Output Driver Messages> (WODM_XXX)
//
// -----------------------------------------------------------------------------

#include "wavemain.h"

#define MIXERCONTROL_CONTROLTYPE_EQPRESET         (MIXERCONTROL_CONTROLTYPE_SINGLESELECT+100)

#define ZONE_HWMIXER    ZONE_VERBOSE
#define ZONE_VOLUME     ZONE_VERBOSE

#define LOGICAL_VOLUME_MAX  0xFFFF
#define LOGICAL_VOLUME_MIN  0
#define LOGICAL_VOLUME_STEPS 1

#define DEVICE_NAME     L"Audio Mixer"
#define DRIVER_VERSION  0x100

#define NELEMS(x) (sizeof(x)/sizeof((x)[0]))

// mixer handle data types
typedef DWORD (* PFNDRIVERCALL)(DWORD hmx, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2);
typedef struct _tagMCB MIXER_CALLBACK, * PMIXER_CALLBACK;
struct _tagMCB
{
    DWORD           hmx;
    PFNDRIVERCALL   pfnCallback;
    PMIXER_CALLBACK pNext;
};

#ifdef DEBUG

// DEBUG-only support for displaying control type codes in readable form

typedef struct
{
    DWORD   val;
    PWSTR   str;
} MMSYSCODE;

#define MXCTYPE(typ) {typ, TEXT(#typ)}

MMSYSCODE ctype_table[] =
{
    MXCTYPE(MIXERCONTROL_CONTROLTYPE_CUSTOM),
    MXCTYPE(MIXERCONTROL_CONTROLTYPE_BASS),
    MXCTYPE(MIXERCONTROL_CONTROLTYPE_EQUALIZER),
    MXCTYPE(MIXERCONTROL_CONTROLTYPE_FADER),
    MXCTYPE(MIXERCONTROL_CONTROLTYPE_TREBLE),
    MXCTYPE(MIXERCONTROL_CONTROLTYPE_VOLUME),
    MXCTYPE(MIXERCONTROL_CONTROLTYPE_MIXER),
    MXCTYPE(MIXERCONTROL_CONTROLTYPE_MULTIPLESELECT),
    MXCTYPE(MIXERCONTROL_CONTROLTYPE_MUX),
    MXCTYPE(MIXERCONTROL_CONTROLTYPE_SINGLESELECT),
    MXCTYPE(MIXERCONTROL_CONTROLTYPE_BOOLEANMETER),
    MXCTYPE(MIXERCONTROL_CONTROLTYPE_PEAKMETER),
    MXCTYPE(MIXERCONTROL_CONTROLTYPE_SIGNEDMETER),
    MXCTYPE(MIXERCONTROL_CONTROLTYPE_UNSIGNEDMETER),
    MXCTYPE(MIXERCONTROL_CONTROLTYPE_DECIBELS),
    MXCTYPE(MIXERCONTROL_CONTROLTYPE_PERCENT),
    MXCTYPE(MIXERCONTROL_CONTROLTYPE_SIGNED),
    MXCTYPE(MIXERCONTROL_CONTROLTYPE_UNSIGNED),
    MXCTYPE(MIXERCONTROL_CONTROLTYPE_PAN),
    MXCTYPE(MIXERCONTROL_CONTROLTYPE_QSOUNDPAN),
    MXCTYPE(MIXERCONTROL_CONTROLTYPE_SLIDER),
    MXCTYPE(MIXERCONTROL_CONTROLTYPE_BOOLEAN),
    MXCTYPE(MIXERCONTROL_CONTROLTYPE_BUTTON),
    MXCTYPE(MIXERCONTROL_CONTROLTYPE_LOUDNESS),
    MXCTYPE(MIXERCONTROL_CONTROLTYPE_MONO),
    MXCTYPE(MIXERCONTROL_CONTROLTYPE_MUTE),
    MXCTYPE(MIXERCONTROL_CONTROLTYPE_ONOFF),
    MXCTYPE(MIXERCONTROL_CONTROLTYPE_STEREOENH),
    MXCTYPE(MIXERCONTROL_CONTROLTYPE_MICROTIME),
    MXCTYPE(MIXERCONTROL_CONTROLTYPE_MILLITIME),
};

PWSTR
COMPTYPE(DWORD dwValue)
{
    int Index;

    for (Index = 0; Index < NELEMS(ctype_table); Index++)
    {
        if (ctype_table[Index].val == dwValue)
        {
            return ctype_table[Index].str;
        }
    }
    return TEXT("<unknown>");

}

#else
#define COMPTYPE(n) TEXT("<>")
#endif

// Destinations:
//    LINE_OUT - line out jack
//      PCM_IN - input to ADC
// Sources
//  MIC - MIC jack

// mixer line ID are 16-bit values formed by concatenating the source and destination line indices
//
#define MXLINEID(dst,src)       ((USHORT) ((USHORT)(dst) | (((USHORT) (src)) << 8)))


enum
{
    LINE_OUT = 0x80,
    PCM_IN,
    MIC,
    NOLINE   = 0xff // doesn't HAVE to be FF, but makes it easier to see
};

const USHORT
g_dst_lines[] =
{
    MXLINEID(LINE_OUT,NOLINE),
    MXLINEID(PCM_IN,NOLINE)
};

const USHORT
g_PCM_IN_sources[] =
{
    MXLINEID(PCM_IN,MIC)
};


// MXLINEDESC corresponds to MIXERLINE, but is designed to conserve space

typedef struct tagMXLINEDESC  const * PMXLINEDESC, MXLINEDESC;

struct tagMXLINEDESC
{
    DWORD dwComponentType;
    PCWSTR szShortName;
    PCWSTR szName;
    DWORD ucFlags;
    USHORT const * pSources;
    USHORT usLineID;
    UINT8 ucChannels;
    UINT8 ucConnections;
    UINT8 ucControls;
    DWORD       dwTargetType;
    UINT8   ucDstIndex;
    UINT8   ucSrcIndex;
} ;

// mixerline ID
#define _MXLE(id,dst,src,flags,comptype,nch,ncnx,nctrl,sname,lname,target,sarray)\
    {comptype,\
    TEXT(sname),TEXT(lname),\
    flags, \
    sarray,\
    id,\
    nch,ncnx,nctrl,\
    target,\
    dst,src}

// declare a destination line
#define MXLED(id,dst,flags,comptype,nch,nctrl,sname,lname,target,srcarray,nsrcs)\
    _MXLE(MXLINEID(id,NOLINE),dst,NOLINE,flags,comptype,nch,nsrcs,nctrl,sname,lname,target,srcarray)\

// declare a source line
#define MXSLE(id,dst,src,flags,comptype,nch,nctrl,sname,lname,target)\
    _MXLE(id,dst,src,flags,comptype,nch,0,nctrl,sname,lname,target,NULL)\

MXLINEDESC
g_mixerline[] =
{
    // dst line 0 - speaker out
    MXLED(LINE_OUT, 0,
          MIXERLINE_LINEF_ACTIVE, // no flags
          MIXERLINE_COMPONENTTYPE_DST_SPEAKERS,
          2, // stereo
          5, // controls: volume, mute, SPDIF, SPDIF-WmaPro, eq preset
          "Volume Control","Volume Control",
          MIXERLINE_TARGETTYPE_WAVEOUT,
          NULL, 0
         ),
    // dst line 1 - microphone input
    MXLED(PCM_IN, 1,
          MIXERLINE_LINEF_ACTIVE, // flags
          MIXERLINE_COMPONENTTYPE_DST_WAVEIN,
          1, // mono
          1, // 1 control (MUX) - the record gain is virtualized across all mux inputs
          "Recording Contr","Recording Control",
          MIXERLINE_TARGETTYPE_WAVEIN,
          g_PCM_IN_sources, NELEMS(g_PCM_IN_sources)
         ),

    // ----------------------
    // PCM_IN Sources Lines
    // ----------------------

    // src line - Microphone
    MXSLE(MXLINEID(PCM_IN, MIC), 1, 1,
          MIXERLINE_LINEF_ACTIVE | MIXERLINE_LINEF_SOURCE, // flags
          MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE,
          1, // mono
          2, // controls: volume, mute
          "Microphone","Microphone",
          MIXERLINE_TARGETTYPE_WAVEIN
         ),

};
const int nlines = NELEMS(g_mixerline);
#undef _MXLE
#undef MXLED
#undef MXSLE

PMXLINEDESC
LookupMxLine(USHORT usLineID)
{
    // scan for mixer line
    int Index;
    for (Index = 0; Index < nlines; Index++)
    {
        if (g_mixerline[Index].usLineID == usLineID)
        {
            return &g_mixerline[Index];
        }
    }
    return NULL;
}

// Base class for mixer control
class MixerControl
{
public:
    MixerControl() : m_dwControlID((DWORD)(-1)) {}
    virtual ~MixerControl() {}

    virtual void SetControlID(DWORD dwControlID) {m_dwControlID = dwControlID;}
    DWORD GetControlID() {return m_dwControlID;}

    virtual void CopyMixerControl(PMIXERCONTROL pamxctrl)
    {
        pamxctrl->cbStruct = sizeof(MIXERCONTROL);
        pamxctrl->dwControlID = GetControlID();
        pamxctrl->dwControlType = GetControlType();
    }

    virtual DWORD GetControlDetails (PMIXERCONTROLDETAILS pDetail, DWORD dwFlags) {return MMSYSERR_NOTSUPPORTED;}
    virtual DWORD SetControlDetails (PMIXERCONTROLDETAILS pDetail, DWORD dwFlags) {return MMSYSERR_NOTSUPPORTED;}
    virtual DWORD GetControlType()=0;
    virtual USHORT GetLineID()=0;

    DWORD GetComponentType()
    {
        PMXLINEDESC pLine = LookupMxLine(GetLineID());
        return pLine->dwComponentType;
    }

private:
    DWORD   m_dwControlID;
};

// Intermediate class for volume controls
class MixerControlVolume: public MixerControl
{
public:
    MixerControlVolume() : MixerControl() {}

    void CopyMixerControl(PMIXERCONTROL pamxctrl)
    {
        MixerControl::CopyMixerControl(pamxctrl);

        pamxctrl->fdwControl = 0;
        pamxctrl->cMultipleItems = 0;
        pamxctrl->Bounds.dwMinimum = LOGICAL_VOLUME_MIN;
        pamxctrl->Bounds.dwMaximum = LOGICAL_VOLUME_MAX;
        pamxctrl->Metrics.cSteps =  LOGICAL_VOLUME_STEPS;
    }

    DWORD GetControlType()
    {
        return MIXERCONTROL_CONTROLTYPE_VOLUME;
    }

    // Derived classes must implement the following methods to set and get volume
    virtual void SetGain(DWORD dwSetting) = 0;
    virtual DWORD GetGain() = 0;

    DWORD GetControlDetails (PMIXERCONTROLDETAILS pDetail, DWORD dwFlags)
    {
        PMXLINEDESC pLine = LookupMxLine(GetLineID());

        ULONG ulControlValue = GetGain();

        MIXERCONTROLDETAILS_UNSIGNED * pValue = (MIXERCONTROLDETAILS_UNSIGNED * ) pDetail->paDetails;
        ULONG ulVolR, ulVolL;

        // Low order word is left channel setting
        ulVolL = ulControlValue & 0xffff;
        if (pLine->ucChannels == 2)
        {
            ulVolR = (ulControlValue >> 16) & 0xffff;
        }
        else
        {
            ulVolR = ulVolL;
        }

        if (pDetail->cChannels == 1)
        {
            pValue[0].dwValue = (ulVolR + ulVolL)/2;
        }
        else
        {
            pValue[0].dwValue = ulVolL;
            pValue[1].dwValue = ulVolR;
        }

        return MMSYSERR_NOERROR;
    }

    DWORD SetControlDetails (PMIXERCONTROLDETAILS pDetail, DWORD dwFlags)
    {
        PMXLINEDESC pLine = LookupMxLine(GetLineID());

        MIXERCONTROLDETAILS_UNSIGNED * pValue = (MIXERCONTROLDETAILS_UNSIGNED * ) pDetail->paDetails;
        DWORD dwSetting, dwSettingL, dwSettingR;
        dwSettingL = pValue[0].dwValue;

        // setting might be mono or stereo. For mono, apply same volume to both channels
        if (pDetail->cChannels == 2)
        {
            dwSettingR = pValue[1].dwValue;
        }
        else
        {
            dwSettingR = dwSettingL;
        }

        if ( (dwSettingL > LOGICAL_VOLUME_MAX) || (dwSettingR > LOGICAL_VOLUME_MAX) )
        {
            DEBUGMSG(ZONE_ERROR, (TEXT("SetMixerControlDetails: Volume exceeds bounds\r\n")));
            return MMSYSERR_INVALPARAM;
        }

        if (pLine->ucChannels == 1)
        {
            dwSetting = (dwSettingL + dwSettingR) / 2;
        }
        else
        {
            // Low order word is left channel setting
            dwSetting = (dwSettingR << 16) | dwSettingL;
        }

        SetGain(dwSetting);

        return MMSYSERR_NOERROR;
    }

};

// Final class for master ouput volume
class MixerControlVolumeMaster: public MixerControlVolume
{
public:
    MixerControlVolumeMaster() : MixerControlVolume() {}

    void CopyMixerControl(PMIXERCONTROL pamxctrl)
    {
        MixerControlVolume::CopyMixerControl(pamxctrl);
        wcscpy(pamxctrl->szName, TEXT("Master Volume"));
        wcscpy(pamxctrl->szShortName, TEXT("Master Volume"));
    }
    USHORT GetLineID() {return MXLINEID(LINE_OUT,NOLINE);}

    void SetGain(DWORD dwSetting)
    {
        g_pHWContext->SetMasterVolume(dwSetting);
    }

    DWORD GetGain()
    {
        
        return g_pHWContext->GetMasterVolume();
       
    }
};

// Intermediate class for mute controls
class MixerControlBoolean: public MixerControl
{
public:
    MixerControlBoolean() : MixerControl() {}

    void CopyMixerControl(PMIXERCONTROL pamxctrl)
    {
        MixerControl::CopyMixerControl(pamxctrl);
        pamxctrl->fdwControl = MIXERCONTROL_CONTROLF_UNIFORM;
        pamxctrl->cMultipleItems = 0;
        pamxctrl->Bounds.dwMinimum = 0;
        pamxctrl->Bounds.dwMaximum = 1;
        pamxctrl->Metrics.cSteps =  0;
    }

    DWORD GetControlType() {return MIXERCONTROL_CONTROLTYPE_BOOLEAN;}

    // Derived classes must implement the following to set & get the mute status
    virtual BOOL GetValue() = 0;
    virtual void SetValue(BOOL bValue) = 0;

    DWORD GetControlDetails (PMIXERCONTROLDETAILS pDetail, DWORD dwFlags)
    {
        ULONG ulControlValue = GetValue();

        MIXERCONTROLDETAILS_BOOLEAN * pValue = (MIXERCONTROLDETAILS_BOOLEAN *) pDetail->paDetails;
        pValue[0].fValue = ulControlValue;
        return MMSYSERR_NOERROR;
    }

    DWORD SetControlDetails (PMIXERCONTROLDETAILS pDetail, DWORD dwFlags)
    {
        MIXERCONTROLDETAILS_BOOLEAN * pValue = (MIXERCONTROLDETAILS_BOOLEAN *) pDetail->paDetails;

        // now apply the setting to the hardware
        SetValue(pValue[0].fValue);

        return MMSYSERR_NOERROR;
    }
};

// Final class for master output mute
class MixerControlMuteMaster: public MixerControlBoolean
{
public:
    MixerControlMuteMaster() : MixerControlBoolean() {}

    void CopyMixerControl(PMIXERCONTROL pamxctrl)
    {
        MixerControlBoolean::CopyMixerControl(pamxctrl);
        wcscpy(pamxctrl->szName, TEXT("Master Mute"));
        wcscpy(pamxctrl->szShortName, TEXT("Master Mute"));
    }
    DWORD GetControlType() {return MIXERCONTROL_CONTROLTYPE_MUTE;}
    USHORT GetLineID() { return MXLINEID(LINE_OUT,NOLINE); }
    BOOL GetValue() { return g_pHWContext->GetMasterMute(); }
    void SetValue(BOOL bMute) { g_pHWContext->SetMasterMute(bMute); }
};

// Intermediate class for single or multiple selection-type controls
class MixerControlSingleMultiSelect: public MixerControl
{
public:
    MixerControlSingleMultiSelect() : MixerControl() {}

    void CopyMixerControl(PMIXERCONTROL pamxctrl)
    {
        MixerControl::CopyMixerControl(pamxctrl);
        pamxctrl->Bounds.dwMinimum = 0;
        pamxctrl->Bounds.dwMaximum = 1;
        pamxctrl->Metrics.cSteps =  0;
        pamxctrl->cMultipleItems = GetNumSelections();
    }

    // Derived classes need to implement the following methods:

    // Return the number of selections...
    virtual DWORD GetNumSelections() = 0;

    // Return the listtext information for a specific selection
    virtual void GetListText(MIXERCONTROLDETAILS_LISTTEXT * pValue, DWORD Index) = 0;

    // Get the current selection
    virtual DWORD GetCurrSelection(DWORD Index) = 0;

    // Set the current selection
    virtual void SetCurrSelection(DWORD Index, BOOL bSet) = 0;

    DWORD GetControlDetails (PMIXERCONTROLDETAILS pDetail, DWORD dwFlags)
    {
        DWORD NumSelections = GetNumSelections();

        if (pDetail->cMultipleItems != NumSelections)
        {
            return MMSYSERR_INVALPARAM;
        }

        switch (dwFlags)
        {
            case MIXER_GETCONTROLDETAILSF_LISTTEXT:
            {
                if ( pDetail->cbDetails != sizeof(MIXERCONTROLDETAILS_LISTTEXT) )
                {
                    return MMSYSERR_INVALPARAM;
                }

                MIXERCONTROLDETAILS_LISTTEXT * pValue = (MIXERCONTROLDETAILS_LISTTEXT * ) pDetail->paDetails;

                for (DWORD i=0;i<NumSelections;i++)
                {
                    GetListText(pValue,i);
                }

                break;
            }
            case MIXER_GETCONTROLDETAILSF_VALUE:
            {
                if ( pDetail->cbDetails != sizeof(MIXERCONTROLDETAILS_BOOLEAN) )
                {
                    return MMSYSERR_INVALPARAM;
                }

                MIXERCONTROLDETAILS_BOOLEAN * pValue = (MIXERCONTROLDETAILS_BOOLEAN *) pDetail->paDetails;
                for (DWORD i=0;i<NumSelections;i++)
                {
                    pValue[i].fValue = GetCurrSelection(i);
                }
                break;
            }
        }

        return MMSYSERR_NOERROR;
    }

    DWORD SetControlDetails (PMIXERCONTROLDETAILS pDetail, DWORD dwFlags)
    {
        DWORD NumSelections = GetNumSelections();

        if (pDetail->cMultipleItems != NumSelections)
        {
            return MMSYSERR_INVALPARAM;
        }

        switch (dwFlags)
        {
            case MIXER_SETCONTROLDETAILSF_VALUE:
            {
                if ( pDetail->cbDetails != sizeof(MIXERCONTROLDETAILS_BOOLEAN) )
                {
                    return MMSYSERR_INVALPARAM;
                }

                MIXERCONTROLDETAILS_BOOLEAN * pValue = (MIXERCONTROLDETAILS_BOOLEAN *) pDetail->paDetails;
                for (DWORD i=0;i<NumSelections;i++)
                {
                    SetCurrSelection(i, pValue[i].fValue);
                }
                break;
            }
        }
        return MMSYSERR_NOERROR;
    }

};

// Final class to implement output equalizer preset.
class MixerControlEqPreset: public MixerControlSingleMultiSelect
{
public:
    MixerControlEqPreset() : m_CurrSelection(0), MixerControlSingleMultiSelect() {}

    void CopyMixerControl(PMIXERCONTROL pamxctrl)
    {
        MixerControlSingleMultiSelect::CopyMixerControl(pamxctrl);
        wcscpy(pamxctrl->szName, TEXT("Eq Preset"));
        wcscpy(pamxctrl->szShortName, TEXT("Eq Preset"));
        pamxctrl->fdwControl = MIXERCONTROL_CONTROLF_UNIFORM|MIXERCONTROL_CONTROLF_MULTIPLE;
    }

    USHORT GetLineID() { return MXLINEID(LINE_OUT,NOLINE); }
    DWORD GetControlType() {return MIXERCONTROL_CONTROLTYPE_EQPRESET;}

    DWORD GetNumSelections() { return 3; }

    void GetListText(MIXERCONTROLDETAILS_LISTTEXT * pValue, DWORD Index)
    {
        static const TCHAR *szNames[] =
        {
                TEXT("Rock"),
                TEXT("Classical"),
                TEXT("Jazz")
        };

        pValue[Index].dwParam1 = 0;
        pValue[Index].dwParam2 = 0;
        wcscpy(pValue[Index].szName,szNames[Index]);
    }

    // Get the current selection
    DWORD GetCurrSelection(DWORD Index)
    {
        return (Index == m_CurrSelection);
    }

    // Set the current selection
    void SetCurrSelection(DWORD Index, BOOL bSet)
    {
        if (bSet)
        {
            m_CurrSelection = Index;
        }
    }
private:
    DWORD m_CurrSelection;
};


class MixerControlList
{
public:
    MixerControlList()
    {
        m_MixerControlList[0] = &m_MixerControlVolumeMaster;
        m_MixerControlList[1] = &m_MixerControlMuteMaster;
        m_MixerControlList[2] = &m_MixerControlEqPreset;

        for (DWORD i=0;i<m_cControls;i++)
        {
            m_MixerControlList[i]->SetControlID(i);
        }
    };

    virtual ~MixerControlList()
    {
    }

    MixerControl *LookupControlByIndex (DWORD Index)
    {
        if (Index >= m_cControls)
        {
            return NULL;
        }
        return m_MixerControlList[Index];
    }

    MMRESULT SetMasterVolume(DWORD dwVolume);

private:
    static const DWORD m_cControls = 3;

    MixerControlVolumeMaster        m_MixerControlVolumeMaster;
    MixerControlMuteMaster          m_MixerControlMuteMaster;
    MixerControlEqPreset            m_MixerControlEqPreset;
    
    MixerControl *m_MixerControlList[m_cControls];

    friend MMRESULT SetMasterVolume(DWORD dwVolume);
    friend DWORD GetMasterVolume();
};

static MixerControlList *g_pMixerControlList = NULL;

static PMIXER_CALLBACK g_pHead; // list of open mixer instances

void
PerformMixerCallbacks(DWORD dwMessage, DWORD dwId)
{
    PMIXER_CALLBACK pCurr;
    for (pCurr = g_pHead; pCurr != NULL; pCurr = pCurr->pNext)
    {
        if (pCurr->pfnCallback != NULL)
        {
            DEBUGMSG(ZONE_HWMIXER, (TEXT("MixerCallback(%d)\r\n"), dwId));
            pCurr->pfnCallback(pCurr->hmx, dwMessage, 0, dwId, 0);
        }
    }
}

DWORD
wdev_MXDM_GETDEVCAPS (PMIXERCAPS pCaps, DWORD dwSize)
{
    pCaps->wMid = MM_MICROSOFT;
    pCaps->wPid = MM_MSFT_WSS_MIXER;
    wcscpy(pCaps->szPname, DEVICE_NAME);
    pCaps->vDriverVersion = DRIVER_VERSION;
    pCaps->cDestinations = NELEMS(g_dst_lines);
    pCaps->fdwSupport = 0;

    return MMSYSERR_NOERROR;
}

DWORD
wdev_MXDM_OPEN (PDWORD phMixer, PMIXEROPENDESC pMOD, DWORD dwFlags)
{
    PMIXER_CALLBACK pNew;
    pNew = (PMIXER_CALLBACK) LocalAlloc(LMEM_FIXED, sizeof(MIXER_CALLBACK));
    if (pNew == NULL)
    {
        ERRMSG("wdev_MXDM_OPEN: out of memory");
        return MMSYSERR_NOMEM;
    }

    pNew->hmx = (DWORD) pMOD->hmx;
    if (dwFlags & CALLBACK_FUNCTION)
    {
        pNew->pfnCallback = (PFNDRIVERCALL) pMOD->dwCallback;
    }
    else
    {
        pNew->pfnCallback = NULL;
    }
    pNew->pNext = g_pHead;
    g_pHead = pNew;
    *phMixer = (DWORD) pNew;

    return MMSYSERR_NOERROR;
}

DWORD
wdev_MXDM_CLOSE (DWORD dwHandle)
{
    PMIXER_CALLBACK pCurr, pPrev;
    pPrev = NULL;
    for (pCurr = g_pHead; pCurr != NULL; pCurr = pCurr->pNext)
    {
        if (pCurr == (PMIXER_CALLBACK) dwHandle)
        {
            if (pPrev == NULL)
            {
                // we're deleting the first item
                g_pHead = pCurr->pNext;
            }
            else
            {
                pPrev->pNext = pCurr->pNext;
            }
            LocalFree(pCurr);
            break;
        }
        pPrev = pCurr;
    }

    return MMSYSERR_NOERROR;
}

DWORD
wdev_MXDM_GETLINEINFO(PMIXERLINE pDetail, DWORD dwFlags)
{ int Index;

    // pDetail is validated by API - points to accessible, properly sized MIXERLINE structure

    // result - assume failure
    PMXLINEDESC pFound = NULL;
    MMRESULT mmRet = MIXERR_INVALLINE;
    USHORT usLineID = NOLINE;

    switch (dwFlags & MIXER_GETLINEINFOF_QUERYMASK)
    {
    case MIXER_GETLINEINFOF_DESTINATION:
        DEBUGMSG(ZONE_HWMIXER, (TEXT("GetMixerLineInfo DESTINATION %x\r\n"), pDetail->dwDestination));
        {
            if (pDetail->dwDestination >= NELEMS(g_dst_lines))
            {
                DEBUGMSG(ZONE_ERROR, (TEXT("GetMixerLineInfo: invalid destination line %d\r\n"), pDetail->dwDestination));
                return MIXERR_INVALLINE;
            }
            usLineID = g_dst_lines[pDetail->dwDestination];
        }
        break;
    case MIXER_GETLINEINFOF_LINEID:
        DEBUGMSG(ZONE_HWMIXER, (TEXT("GetMixerLineInfo LINEID %x\r\n"), pDetail->dwLineID));
        usLineID = (USHORT) pDetail->dwLineID;
        break;
    case MIXER_GETLINEINFOF_SOURCE:
        DEBUGMSG(ZONE_HWMIXER, (TEXT("GetMixerLineInfo SOURCE %x %x\r\n"), pDetail->dwDestination, pDetail->dwSource));
        {
            PMXLINEDESC pLine;
            // look up the destination line, then index into it's source table
            // to find the indexed source.
            if (pDetail->dwDestination >= NELEMS(g_dst_lines))
            {
                DEBUGMSG(ZONE_ERROR, (TEXT("GetMixerLineInfo: invalid destination line %d\r\n"), pDetail->dwDestination));
                return MIXERR_INVALLINE;
            }
            pLine = LookupMxLine(g_dst_lines[pDetail->dwDestination]);
            if (pLine == NULL)
            {
                DEBUGMSG(ZONE_ERROR, (TEXT("GetMixerLineInfo: inconsistent internal mixer line table\r\n")));
                return MMSYSERR_ERROR;
            }
            if (pDetail->dwSource >= pLine->ucConnections)
            {
                DEBUGMSG(ZONE_ERROR, (TEXT("GetMixerLineInfo: invalid source line %d\r\n"), pDetail->dwSource));
                return MIXERR_INVALLINE;
            }
            usLineID = pLine->pSources[pDetail->dwSource];
        }
        break;
    case MIXER_GETLINEINFOF_COMPONENTTYPE:
        DEBUGMSG(ZONE_HWMIXER, (TEXT("GetMixerLineInfo COMPONENT\r\n")));
        break;

    case MIXER_GETLINEINFOF_TARGETTYPE:
        DEBUGMSG(ZONE_HWMIXER, (TEXT("GetMixerLineInfo TARGET\r\n")));
        // valid query, but we're not going to form usLineID
        break;
    default:
        DEBUGMSG(ZONE_ERROR, (TEXT("GetMixerLineInfo: invalid query %08x\r\n"), dwFlags & MIXER_GETLINEINFOF_QUERYMASK));
        return MMSYSERR_INVALPARAM;
    }

    switch (dwFlags & MIXER_GETLINEINFOF_QUERYMASK)
    {
    case MIXER_GETLINEINFOF_COMPONENTTYPE:
        // scan for line of proper type
        for (Index = 0; Index < nlines; Index++)
        {
            if (g_mixerline[Index].dwComponentType == pDetail->dwComponentType)
            {
                pFound = &g_mixerline[Index];
                break;
            }
        }
#ifdef DEBUG
        if (pFound == NULL)
        {
            DEBUGMSG(ZONE_ERROR, (TEXT("GetMixerLineInfo: no line of component type %08x\r\n"), pDetail->dwComponentType));
        }
#endif
        break;
    case MIXER_GETLINEINFOF_TARGETTYPE:
        // scan for target type
        for (Index = 0; Index < nlines; Index++)
        {
            if (g_mixerline[Index].dwTargetType == pDetail->Target.dwType)
            {
                pFound = &g_mixerline[Index];
                break;
            }
        }
#ifdef DEBUG
        if (pFound == NULL)
        {
            DEBUGMSG(ZONE_ERROR, (TEXT("GetMixerLineInfo: no line of target type %08x\r\n"), pDetail->Target.dwType));
        }
#endif
        break;

    case MIXER_GETLINEINFOF_DESTINATION:
    case MIXER_GETLINEINFOF_LINEID:
    case MIXER_GETLINEINFOF_SOURCE:
        pFound = LookupMxLine(usLineID);
        if (pFound == NULL)
        {
            DEBUGMSG(ZONE_ERROR, (TEXT("GetMixerLineInfo: invalid line ID %08x\r\n"), usLineID));
            return MMSYSERR_ERROR;
        }
        break;
    default:
        // should never happen - we filter for this in the first switch()
        break;

    }

    if (pFound != NULL)
    {
        pDetail->cChannels = pFound->ucChannels;
        pDetail->cConnections = pFound->ucConnections;
        pDetail->cControls = pFound->ucControls;
        pDetail->dwComponentType = pFound->dwComponentType;
        pDetail->dwLineID = pFound->usLineID;
        pDetail->dwDestination = pFound->ucDstIndex;
        pDetail->dwSource = pFound->ucSrcIndex;
        pDetail->fdwLine = pFound->ucFlags;
        pDetail->Target.dwDeviceID = 0;
        pDetail->Target.dwType = pFound->dwTargetType;
        pDetail->Target.vDriverVersion = DRIVER_VERSION;
        pDetail->Target.wMid = MM_MICROSOFT;
        pDetail->Target.wPid = MM_MSFT_WSS_MIXER;
        wcscpy(pDetail->szName, pFound->szName);
        wcscpy(pDetail->szShortName, pFound->szShortName);
        wcscpy(pDetail->Target.szPname, DEVICE_NAME);
        mmRet = MMSYSERR_NOERROR;

        DEBUGMSG(ZONE_HWMIXER, (TEXT("GetMixerLineInfo: \"%s\" %08x\r\n"), pFound->szName, pFound->ucFlags));

    }

    return mmRet;
}

DWORD
wdev_MXDM_GETLINECONTROLS (PMIXERLINECONTROLS pDetail, DWORD dwFlags)
{

    PMIXERCONTROL pamxctrl = pDetail->pamxctrl;
    USHORT usLineID = (USHORT) pDetail->dwLineID;
    DWORD dwCount = pDetail->cControls;

    switch (dwFlags & MIXER_GETLINECONTROLSF_QUERYMASK)
    {
    case MIXER_GETLINECONTROLSF_ALL:
        DEBUGMSG(ZONE_HWMIXER, (TEXT("GetMixerLineControls: ALL %x\r\n"), usLineID));
        // retrieve all controls for the line pDetail->dwLineID
        {
            PMXLINEDESC pFound = LookupMxLine(usLineID);
            if (pFound == NULL)
            {
                DEBUGMSG(ZONE_ERROR, (TEXT("GetMixerLineControls: invalid line ID %04x\r\n"), usLineID));
                return MIXERR_INVALLINE;
            }
            if (pFound->ucControls != dwCount)
            {
                DEBUGMSG(ZONE_ERROR, (TEXT("GetMixerLineControls: incorrect number of controls. Expect %d, found %d.\r\n"),dwCount,pFound->ucControls));
                return MMSYSERR_INVALPARAM;
            }

            int Index;
            for (Index = 0; dwCount > 0; Index++)
            {
                MixerControl *pMixerControl = g_pMixerControlList->LookupControlByIndex(Index);
                if (!pMixerControl)
                {
					// We never found as many controls as we claimed to have...
					return MMSYSERR_INVALPARAM;
                }

                if (pMixerControl->GetLineID() == usLineID)
                {
                    pMixerControl->CopyMixerControl(pamxctrl);
                    pamxctrl++;
                    dwCount--;
                }
            }
        }
        break;

    case MIXER_GETLINECONTROLSF_ONEBYID:
        {
            DEBUGMSG(ZONE_HWMIXER, (TEXT("GetMixerLineControls: ONEBYID %x\r\n"), pDetail->dwControlID));

            if (dwCount < 1)
            {
                DEBUGMSG(ZONE_ERROR, (TEXT("GetMixerLineControls: control count must be nonzero\r\n")));
                return MMSYSERR_INVALPARAM;
            }

            MixerControl *pMixerControl = g_pMixerControlList->LookupControlByIndex(pDetail->dwControlID);
            if (!pMixerControl)
            {
                return MIXERR_INVALCONTROL;
            }

            pMixerControl->CopyMixerControl(pamxctrl);
            pDetail->dwLineID = pMixerControl->GetLineID();
            break;
        }

    case MIXER_GETLINECONTROLSF_ONEBYTYPE:
        // retrieve the control specified by pDetail->dwLineID and pDetail->dwControlType
        {
            DEBUGMSG(ZONE_HWMIXER, (TEXT("GetMixerLineControls: ONEBYTYPE %x \r\n"), usLineID));

            if (dwCount < 1)
            {
                DEBUGMSG(ZONE_ERROR, (TEXT("GetMixerLineControls: control count must be non zero\r\n")));
                return MMSYSERR_INVALPARAM;
            }

            MixerControl *pMixerControl = NULL;

            int Index;
            for (Index = 0;;Index++)
            {
                pMixerControl = g_pMixerControlList->LookupControlByIndex(Index);
                if (!pMixerControl)
                {
                    break;
                }

                if ( (pMixerControl->GetLineID() == usLineID) && (pMixerControl->GetControlType() == pDetail->dwControlType) )
                {
                    break;
                }
            }

            if (!pMixerControl)
            {
                // not to be alarmed: SndVol32 queries for LOTS of control types we don't have
                DEBUGMSG(ZONE_HWMIXER, (TEXT("GetMixerLineControls: line %04x has no control of type %s (%08x)\r\n"), usLineID, COMPTYPE(pDetail->dwControlType), pDetail->dwControlType));
                return MMSYSERR_INVALPARAM;
            }
            else
            {
                pMixerControl->CopyMixerControl(pamxctrl);
                return MMSYSERR_NOERROR;
            }
        }
        break;
    default:
        DEBUGMSG(ZONE_ERROR, (TEXT("GetMixerLineControls: invalid query %08x\r\n"), dwFlags & MIXER_GETLINECONTROLSF_QUERYMASK));
        break;

    }
    return MMSYSERR_NOERROR;
}

DWORD
wdev_MXDM_GETCONTROLDETAILS (PMIXERCONTROLDETAILS pDetail, DWORD dwFlags)
{
    // API guarantees that pDetail points to accessible, aligned, properly sized MIXERCONTROLDETAILS structure
    DEBUGMSG(ZONE_HWMIXER, (TEXT("GetMixerControlDetails(%d)\r\n"), pDetail->dwControlID));

    MixerControl *pMixerControl = g_pMixerControlList->LookupControlByIndex(pDetail->dwControlID);
    if (!pMixerControl)
    {
        return MIXERR_INVALCONTROL;
    }

    return pMixerControl->GetControlDetails(pDetail, dwFlags);
}

DWORD
wdev_MXDM_SETCONTROLDETAILS (PMIXERCONTROLDETAILS pDetail, DWORD dwFlags)
{
//    MIXERCONTROLDETAILS_UNSIGNED * pValue = (MIXERCONTROLDETAILS_UNSIGNED * ) pDetail->paDetails;

    // API guarantees that pDetail points to accessible, aligned, properly siezd MIXERCONTROLDETAILS structure
    DEBUGMSG(ZONE_HWMIXER, (TEXT("SetMixerControlDetails(%d)\r\n"), pDetail->dwControlID));

    MixerControl *pMixerControl = g_pMixerControlList->LookupControlByIndex(pDetail->dwControlID);
    if (!pMixerControl)
    {
        return MIXERR_INVALCONTROL;
    }

    pMixerControl->SetControlDetails(pDetail, dwFlags);

    PerformMixerCallbacks (MM_MIXM_CONTROL_CHANGE, pDetail->dwControlID);

    return MMSYSERR_NOERROR;
}

BOOL InitMixerControls()
{
    g_pMixerControlList = new MixerControlList;
    return TRUE;
}

BOOL HandleMixerMessage(PMMDRV_MESSAGE_PARAMS pParams, DWORD *pdwResult)
{
    MMRESULT dwRet;

    _try
    {
        switch (pParams->uMsg)
        {
        case MXDM_GETNUMDEVS:
            PRINTMSG(ZONE_WODM, (TEXT("MXDM_GETNUMDEVS\r\n")));
            dwRet = 1;
            break;

        case MXDM_GETDEVCAPS:
            PRINTMSG(ZONE_WODM, (TEXT("MXDM_GETDEVCAPS\r\n")));
            dwRet = wdev_MXDM_GETDEVCAPS((PMIXERCAPS) pParams->dwParam1, pParams->dwParam2);
            break;

        case MXDM_OPEN:
            PRINTMSG(ZONE_WODM, (TEXT("MXDM_OPEN\r\n")));
            dwRet = wdev_MXDM_OPEN((PDWORD) pParams->dwUser, (PMIXEROPENDESC) pParams->dwParam1, pParams->dwParam2);
            break;

        case MXDM_CLOSE:
            PRINTMSG(ZONE_WODM, (TEXT("MXDM_CLOSE\r\n")));
            dwRet = wdev_MXDM_CLOSE(pParams->dwUser);
            break;

        case MXDM_GETLINEINFO:
            PRINTMSG(ZONE_WODM, (TEXT("MXDM_GETLINEINFO\r\n")));
            dwRet = wdev_MXDM_GETLINEINFO((PMIXERLINE) pParams->dwParam1, pParams->dwParam2);
            break;

        case MXDM_GETLINECONTROLS:
            PRINTMSG(ZONE_WODM, (TEXT("MXDM_GETLINECONTROLS\r\n")));
            dwRet = wdev_MXDM_GETLINECONTROLS((PMIXERLINECONTROLS) pParams->dwParam1, pParams->dwParam2);
            break;

        case MXDM_GETCONTROLDETAILS:
            PRINTMSG(ZONE_WODM, (TEXT("MXDM_GETCONTROLDETAILS\r\n")));
            dwRet = wdev_MXDM_GETCONTROLDETAILS((PMIXERCONTROLDETAILS) pParams->dwParam1, pParams->dwParam2);
            break;

        case MXDM_SETCONTROLDETAILS:
            PRINTMSG(ZONE_WODM, (TEXT("MXDM_SETCONTROLDETAILS\r\n")));
            dwRet = wdev_MXDM_SETCONTROLDETAILS((PMIXERCONTROLDETAILS) pParams->dwParam1, pParams->dwParam2);
            break;

        default:
            ERRMSG("Unsupported mixer message");
            dwRet = MMSYSERR_NOTSUPPORTED;
            break;
        }      // switch (pParams->uMsg)
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        dwRet  = MMSYSERR_INVALPARAM;
    }

    *pdwResult = dwRet;
    return TRUE;
}

// Helper functions to set/get the master wave volume from outside the mixer API.
MMRESULT SetMasterVolume(DWORD dwVolume)
{
    g_pMixerControlList->m_MixerControlVolumeMaster.SetGain(dwVolume);
    PerformMixerCallbacks (MM_MIXM_CONTROL_CHANGE, g_pMixerControlList->m_MixerControlVolumeMaster.GetControlID());
    return MMSYSERR_NOERROR;
}

DWORD GetMasterVolume()
{
    return g_pMixerControlList->m_MixerControlVolumeMaster.GetGain();
}

