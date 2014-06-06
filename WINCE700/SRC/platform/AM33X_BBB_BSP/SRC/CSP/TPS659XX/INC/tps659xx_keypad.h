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
//  File:  keypad.h
//
#ifndef __KEYPAD_H
#define __KEYPAD_H

//------------------------------------------------------------------------------
//
//  Define: KEYPAD_ROWS/KEYPAD_COLUMNS
//
#define KEYPAD_ROWS         8
#define KEYPAD_COLUMNS      8

//------------------------------------------------------------------------------
//
//  Global:  g_keypadVK
//
//  Global array mapping physical keypad location to virtual key
//
extern const UCHAR g_keypadVK[KEYPAD_ROWS * KEYPAD_COLUMNS];
extern const UCHAR g_powerVK[1];

//------------------------------------------------------------------------------
// GPIO pins - Virtual Key mapping
typedef struct {
    const UINT16 gpio;                  // gpio pin
    const UCHAR  vkey;                  // virtual key
} KEYPAD_GPIO_VK_ITEM;

typedef struct {
    const USHORT count;
    const KEYPAD_GPIO_VK_ITEM *pItem;
} KEYPAD_GPIO_VK;

extern const KEYPAD_GPIO_VK g_GpioKeypadVK;

typedef struct {
    DWORD irq;                  // gpio irq
    DWORD sysintr;                 // Logical interrupt
} KEYPAD_GPIO_IRQ_ITEM;

extern KEYPAD_GPIO_IRQ_ITEM *g_GpioIrqSys;

//------------------------------------------------------------------------------

typedef struct {
    const UCHAR  vkey;                  // virtual key to be result of remapping
    const UCHAR  keys;                  // number of keys to map
    const USHORT delay;                 // debounce delay in ms 
    const UCHAR  *pVKeys;               // virtual keys to map from
} KEYPAD_REMAP_ITEM;

typedef struct {
    const USHORT count;
    const KEYPAD_REMAP_ITEM *pItem;
} KEYPAD_REMAP;

extern const KEYPAD_REMAP g_keypadRemap;
extern const KEYPAD_REMAP g_powerRemap;

//------------------------------------------------------------------------------

typedef struct {
    const UCHAR  count;                 // number of keys to block repeat
    const UCHAR  *pVKey;                // virtual keys blocking repeat
} KEYPAD_REPEAT_BLOCK;

typedef struct {
    const UCHAR  vkey;                  // virtual key to be repeated
    const USHORT firstDelay;            // delay before first repeat
    const USHORT nextDelay;             // delay before next repeat
    const BOOL   silent;                // repeat should be without key click
    const KEYPAD_REPEAT_BLOCK *pBlock;  // virtual keys blocking repeat
} KEYPAD_REPEAT_ITEM;

typedef struct {
    const USHORT count;
    const KEYPAD_REPEAT_ITEM *pItem;
} KEYPAD_REPEAT;

extern const KEYPAD_REPEAT g_keypadRepeat;
extern const KEYPAD_REPEAT g_powerRepeat;

//------------------------------------------------------------------------------
#define KEYPAD_EXTRASEQ_ORDER_EXTRAFIRST  0x00   // send extra vk first at press and release
#define KEYPAD_EXTRASEQ_ORDER_EXTRAORIG   0x01   // send extra vk first at press, send original vk first at release
#define KEYPAD_EXTRASEQ_ORDER_ORIGFIRST   0x02   // send original vk first at press and release
#define KEYPAD_EXTRASEQ_ORDER_NONE        0xFF   // no extra key send

typedef struct {
    const UCHAR  vk_orig;                   // original virtual key
    const UCHAR  vk_extra;                  // extra virtual key
    const UCHAR  order;                     // which one should be sent first
} KEYPAD_EXTRASEQ_ITEM;

typedef struct {
    const USHORT count;
    const KEYPAD_EXTRASEQ_ITEM *pItem;
} KEYPAD_EXTRASEQ;

extern const KEYPAD_EXTRASEQ g_keypadExtraSeq;

//------------------------------------------------------------------------------
#endif // __KEYPAD_H
