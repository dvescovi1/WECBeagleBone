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
//-----------------------------------------------------------------------------
//
//  File: t2transceiver.h
//
#ifndef __TPS659XX_BCI_H
#define __TPS659XX_BCI_H

//-----------------------------------------------------------------------------
// T2 BCI Register Values
//-----------------------------------------------------------------------------

// TWL_BOOT_BCI
#define BCIAUTOWEN                          (1 << 5)
#define CONFIG_DONE                         (1 << 4)
#define CVENAC                              (1 << 2)
#define BCIAUTOUSB                          (1 << 1)
#define BCIAUTOAC                           (1 << 0)

// TWL_BCIMDKEY
#define KEY_BCIACLINEAR                     (0x25)
#define KEY_BCIUSBLINEAR                    (0x26)
#define KEY_BCIACPULSE                      (0x27)
#define KEY_BCIACACCESSORY                  (0x28)
#define KEY_BCIUSBACCESSORY                 (0x29)
#define KEY_BCIOFF                          (0x2A)

// TWL_BCIMFKEY
#define KEY_BCIMFEN1                        (0x57)
#define KEY_BCIMFEN2                        (0x73)
#define KEY_BCIMFEN3                        (0x9C)
#define KEY_BCIMFEN4                        (0x3E)
#define KEY_BCIMFTH1                        (0xD2)
#define KEY_BCIMFTH2                        (0x7F)
#define KEY_BCIMFTH3                        (0x6D)
#define KEY_BCIMFTH4                        (0xEA)
#define KEY_BCIMFTH5                        (0xC4)
#define KEY_BCIMFTH6                        (0xBC)
#define KEY_BCIMFTH7                        (0xC3)
#define KEY_BCIMFTH8                        (0xF4)
#define KEY_BCIMFTH9                        (0xE7)

// TWL_BCIWDKEY
#define KEY_BCIWDKEY1                       (0xAA)
#define KEY_BCIWDKEY2                       (0x55)
#define KEY_BCIWDKEY3                       (0xDB)
#define KEY_BCIWDKEY4                       (0xBD)
#define KEY_BCIWDKEY5                       (0xF3)
#define KEY_BCIWDKEY6                       (0x33)

// TWL_BCIMFEN2
#define ACCHGOVEN                           (1 << 7)
#define ACCHGOVCF                           (1 << 6)
#define VBUSOVEN                            (1 << 5)
#define VBUSOVCF                            (1 << 4)
#define TBATOR1EN                           (1 << 3)
#define TBATOR1CF                           (1 << 2)
#define TBATOR2EN                           (1 << 1)
#define TBATOR2CF                           (1 << 0)

// TWL_BCIMFEN3
#define ICHGEOCEN                           (1 << 7)
#define ICHGEOCCF                           (1 << 6)
#define ICHGLOWEN                           (1 << 5)
#define ICHGLOWCF                           (1 << 4)
#define ICHGHIGHEN                          (1 << 3)
#define ICHGHIGHCF                          (1 << 2)
#define HSMODE                              (1 << 1)
#define HSEN                                (1 << 0)

// TWL_BCIMFEN4
#define BATSTSMCHGEN                        (1 << 2)
#define VBATOVEN                            (1 << 1)
#define VBATOVCF                            (1 << 0)

// TWL_BCIMFTH3
#define VBATOVTH                            (3 << 4)
#define ACCHGOVTH                           (3 << 2)
#define VBUSOVTH                            (3 << 0)

// TWL_BCIMFSTS2
#define VBATOV                              (1 << 6)
#define VBATOV4                             (1 << 5)
#define VBATOV3                             (1 << 4)
#define VBATOV2                             (1 << 3)
#define VBATOV1                             (1 << 2)
#define VBUSOV                              (1 << 1)
#define ACCHGOV                             (1 << 0)

// TWL_BCIMFSTS3
#define BATSTSMCHG                          (1 << 6)
#define CHGSTSTMAIN                         (1 << 5)
#define TBATOR1                             (1 << 4)
#define TBATOR2                             (1 << 3)
#define ICHGEOC                             (1 << 2)
#define ICHGLOW                             (1 << 1)
#define ICHGHIGH                            (1 << 0)

// TWL_BCIMFSTS4
#define SYSACTIV                            (1 << 4)
#define SUSPENDM                            (1 << 3)
#define USBFASTMCHG                         (1 << 2)
#define USBSLOWMCHG                         (1 << 1)
#define HTSAVESTS                           (1 << 0)

// TWL_BCICTL1
#define CGAIN                               (1 << 5)
#define TYPEN                               (1 << 4)
#define ITHEN                               (1 << 3)
#define MESVBUS                             (1 << 2)
#define MESBAT                              (1 << 1)
#define MESVAC                              (1 << 0)

// TWL_CTRL1
#define MADCON                              (1 << 0)
#define VBATREF_LSB_SHIFT                   (3)
#define VBATREF_LSB_MASK                    (0xF8)
#define VBATREF_LSB(x)                      ((x & VBATREF_LSB_MASK) << VBATREF_LSB_SHIFT)

#define BCIREF1_HBIT                        (1 << 9)

#endif //__TPS659XX_BCI_H

