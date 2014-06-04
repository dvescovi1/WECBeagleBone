// All rights reserved ADENEO EMBEDDED 2010
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
#pragma once
// -----------------------------------------------------------------------------
//
//      THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//      ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//      THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//      PARTICULAR PURPOSE.
//
// -----------------------------------------------------------------------------

// Enable to turn on linear interpolation between samples in sine table
// #instructions in inner loop: ~7 added
#define MIDI_OPTIMIZE_LINEAR_INTERPOLATE 0

#define NUMCHANNELS  (16)
#define NUMNOTES     (32)

// Special channel reserved for playing arbitrary tones
#define FREQCHANNEL (NUMCHANNELS)

class CMidiNote;
class CMidiStream;

class CMidiNote
{
public:
    UINT32 NoteVal();
    UINT32 NoteChannel();
    HRESULT NoteOn(CMidiStream *pMidiStream, UINT32 dwChannel, UINT32 dwNote, UINT32 dwVelocity);
    HRESULT NoteOff(UINT32 dwVelocity);
    PBYTE Render(PBYTE pBuffer, PBYTE pBufferEnd, PBYTE pBufferLast, TRANSFER_STATUS *pTransferStatus);
    PBYTE Render2(PBYTE pBuffer, PBYTE pBufferEnd, PBYTE pBufferLast, TRANSFER_STATUS *pTransferStatus);

    void GainChange();

    void SetVelocity(UINT32 Velocity);
    void ResetBaseInfo();

    LIST_ENTRY m_Link;

private:
    static const INT16 SineTable[0x101];                // Sine table

    CMidiStream *m_pMidiStream;
    UINT32 m_Note;
    UINT32 m_Velocity;
    UINT32 m_Channel;

    UINT32 m_Index;          // Current index into wavetable
    UINT32 m_IndexDelta;     // Amount to increment index on each sample

    DWORD   m_dwGain;           // gain based on velocity
    DWORD   m_fxpGain[2];       // effective gain, after composing with device and class gains.

    DWORD   m_dwBytesLeft;
};

class CMidiStream : public StreamContext
{
public:
    DWORD Open(DeviceContext *pDeviceContext, LPWAVEOPENDESC lpWOD, DWORD dwFlags);
    DWORD Close();

    PBYTE Render(PBYTE pBuffer, PBYTE pBufferEnd, PBYTE pBufferLast, TRANSFER_STATUS *pTranferStatus);

    UINT32 DeltaTicksToByteCount(UINT32 DeltaTicks);

    void NoteMoveToFreeList(CMidiNote *pCMidiNote);
    void NoteMoveToNoteList(CMidiNote *pCMidiNote);
    void NoteDone(CMidiNote *pCMidiNote);
    HRESULT NoteOn(UINT32 dwNote, UINT32 dwVelocity, UINT32 dwChannel);
    HRESULT NoteOff(UINT32 dwNote, UINT32 dwVelocity, UINT32 dwChannel);
    HRESULT AllNotesOff(UINT32 dwVelocity);

    DWORD MidiMessage(UINT32 dwMsg);
    HRESULT InternalMidiMessage(UINT32 dwData);
    HRESULT MidiData(UINT32 dwData);

    CMidiNote *FindNote(UINT32 dwNote, UINT32 dwChannel);
    CMidiNote *AllocNote(UINT32 dwNote, UINT32 dwChannel);

    UINT32 DeltaTicksToSamples(UINT32 DeltaTicks);
    HRESULT UpdateTempo();

    UINT32 ProcessMidiStream();

    DWORD MapNoteGain(DWORD NoteGain, DWORD Channel);
    void GainChange();

    DWORD Reset();

protected:
    CMidiNote       m_MidiNote[NUMNOTES];
    LIST_ENTRY      m_NoteList;
    LIST_ENTRY      m_FreeList;
    UINT32          m_ByteCount;
    BYTE            m_RunningStatus;
    UINT32          m_USecPerQuarterNote;     // Tempo, from midi stream
    UINT32          m_TicksPerQuarterNote;    // PPQN, from midi header
    UINT32          m_SamplesPerTick;         // Calculated as (SampleRate * Tempo)/(PPQN * 1000000)
    UINT32          m_DeltaSampleCount;       // # of samples since last midi event (init to 0)

public:
    void ResetBaseInfo();
    DWORD GetDeltaFromNote(DWORD Note);
    DWORD GetDeltaFromFreq(DWORD Freq);

private:
    static const DWORD ms_BasePitchTable[12];
    static DWORD ms_PitchTable[12];
    static DWORD ms_PitchTableInverseSampleRate;

};

