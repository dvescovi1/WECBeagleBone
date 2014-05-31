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
/*
*  File:  omap_sdhc_regs.h
*/
#ifndef __OMAP_SDHC_REGS_H
#define __OMAP_SDHC_REGS_H

#define MMCSLOT_1   1
#define MMCSLOT_2   2
#define MMCSLOT_3   3

//
//  MMC/SD/SDIO Registers
//

typedef volatile struct
{
    // for netra/centaurus, SD_HL_REV, SD_HL_HWINFO, SD_HL_SYSCONFIG 
    // are in this range before unused0[]
    // see CSP\AM389X\SOCCFG\devicemap.c
                                // offsets are w.r.t struct start
    UINT32 unused0[4];			// 0x0000 - 0x000C
    UINT32 MMCHS_SYSCONFIG;		// 0x0010
    UINT32 MMCHS_SYSSTATUS;		// 0x0014
    UINT32 unused1[3];			// 0x0018 - 0x0020
    UINT32 MMCHS_CSRE;			// 0x0024
    UINT32 MMCHS_SYSTEST;		// 0x0028
    UINT32 MMCHS_CON;			// 0x002C
    UINT32 MMCHS_PWCNT;			// 0x0030
    UINT32 unused2[52];			// 0x0034 - 0x0100
    UINT32 MMCHS_BLK;			// 0x0104
    UINT32 MMCHS_ARG;			// 0x0108
    UINT32 MMCHS_CMD;			// 0x010C
    UINT32 MMCHS_RSP10;			// 0x0110
    UINT32 MMCHS_RSP32;			// 0x0114
    UINT32 MMCHS_RSP54;			// 0x0118
    UINT32 MMCHS_RSP76;			// 0x011C
    UINT32 MMCHS_DATA;			// 0x0120
    UINT32 MMCHS_PSTATE;		// 0x0124
    UINT32 MMCHS_HCTL;			// 0x0128
    UINT32 MMCHS_SYSCTL;		// 0x012C
    UINT32 MMCHS_STAT;			// 0x0130
    UINT32 MMCHS_IE;			// 0x0134
    UINT32 MMCHS_ISE;			// 0x0138
    UINT32 MMCHS_AC12;			// 0x013C
    UINT32 MMCHS_CAPA;			// 0x0140
    UINT32 unused4[1];			// 0x0144
    UINT32 MMCHS_CUR_CAPA;		// 0x0148
    UINT32 unused5[44];			// 0x014C - 0x01F8  (for netra: SD_ADMAES, SD_ADMASAL, SD_ADMASAH are in this range)
    UINT32 MMCHS_REV;			// 0x01FC
} OMAP_MMCHS_REGS, OMAP_MMCHS_REGS;

// MMCHS_SYSCONFIG register fields

#define MMCHS_SYSCONFIG_AUTOIDLE                (1 << 0)
#define MMCHS_SYSCONFIG_SOFTRESET               (1 << 1)
#define MMCHS_SYSCONFIG_ENAWAKEUP               (1 << 2)
#define MMCHS_SYSCONFIG_SIDLEMODE(mode)         ((mode) << 3)
#define MMCHS_SYSCONFIG_CLOCKACTIVITY(act)      ((act) << 8)

#define SIDLE_FORCE                             (0)
#define SIDLE_IGNORE                            (1)
#define SIDLE_SMART                             (2)

// MMCHS_SYSSTATUS register fields

#define MMCHS_SYSSTATUS_RESETDONE               (1 << 0)

#define MMCHS_IE_CINS_SHIFT                     6
#define MMCHS_IE_CREM_SHIFT                     7

#define MMCHS_CON_CDP_SHIFT                     7
#define MMCHS_CON_WPP_SHIFT                     8

#define MMCHS_PSTAT_CINS_SHIFT                  16
#define MMCHS_PSTAT_WP_SHIFT                    19

// MMCHS_IE register fields

#define MMCHS_IE_CC                             (1 << 0)
#define MMCHS_IE_TC                             (1 << 1)
#define MMCHS_IE_BGE                            (1 << 2)
#define MMCHS_IE_BWR                            (1 << 4)
#define MMCHS_IE_BRR                            (1 << 5)
#define MMCHS_IE_CIRQ                           (1 << 8)
#define MMCHS_IE_CTO                            (1 << 16)
#define MMCHS_IE_CCRC                           (1 << 17)
#define MMCHS_IE_CEB                            (1 << 18)
#define MMCHS_IE_CIE                            (1 << 19)
#define MMCHS_IE_DTO                            (1 << 20)
#define MMCHS_IE_DCRC                           (1 << 21)
#define MMCHS_IE_DEB                            (1 << 22)
#define MMCHS_IE_ACE                            (1 << 24)
#define MMCHS_IE_CERR                           (1 << 28)
#define MMCHS_IE_BADA                           (1 << 29)

// for NETRA
#define MMCHS_IE_CINS                           (1 << MMCHS_IE_CINS_SHIFT)
#define MMCHS_IE_CREM                           (1 << MMCHS_IE_CREM_SHIFT)

// MMCHS_ISE register fields

#define MMCHS_ISE_CC                            (1 << 0)
#define MMCHS_ISE_TC                            (1 << 1)
#define MMCHS_ISE_BGE                           (1 << 2)
#define MMCHS_ISE_BWR                           (1 << 4)
#define MMCHS_ISE_BRR                           (1 << 5)
#define MMCHS_ISE_CIRQ                          (1 << 8)
#define MMCHS_ISE_CTO                           (1 << 16)
#define MMCHS_ISE_CCRC                          (1 << 17)
#define MMCHS_ISE_CEB                           (1 << 18)
#define MMCHS_ISE_CIE                           (1 << 19)
#define MMCHS_ISE_DTO                           (1 << 20)
#define MMCHS_ISE_DCRC                          (1 << 21)
#define MMCHS_ISE_DEB                           (1 << 22)
#define MMCHS_ISE_ACE                           (1 << 24)
#define MMCHS_ISE_CERR                          (1 << 28)
#define MMCHS_ISE_BADA                          (1 << 29)

// for NETRA
#define MMCHS_ISE_CINS                          (1 << MMCHS_IE_CINS_SHIFT)
#define MMCHS_ISE_CREM                          (1 << MMCHS_IE_CREM_SHIFT)

// MMCHS_STAT register fields

#define MMCHS_STAT_CC                           (1 << 0)
#define MMCHS_STAT_TC                           (1 << 1)
#define MMCHS_STAT_BGE                          (1 << 2)
#define MMCHS_STAT_BWR                          (1 << 4)
#define MMCHS_STAT_BRR                          (1 << 5)
#define MMCHS_STAT_CIRQ                         (1 << 8)
#define MMCHS_STAT_ERRI                         (1 << 15)
#define MMCHS_STAT_CTO                          (1 << 16)
#define MMCHS_STAT_CCRC                         (1 << 17)
#define MMCHS_STAT_CEB                          (1 << 18)
#define MMCHS_STAT_CIE                          (1 << 19)
#define MMCHS_STAT_DTO                          (1 << 20)
#define MMCHS_STAT_DCRC                         (1 << 21)
#define MMCHS_STAT_DEB                          (1 << 22)
#define MMCHS_STAT_ACE                          (1 << 24)
#define MMCHS_STAT_CERR                         (1 << 28)
#define MMCHS_STAT_BADA                         (1 << 29)

// for NETRA
// MMCHS_STAT_CINS=0x00000040
// MMCHS_STAT_CREM=0x00000080
#define MMCHS_STAT_CINS                         (1 << MMCHS_IE_CINS_SHIFT)
#define MMCHS_STAT_CREM                         (1 << MMCHS_IE_CREM_SHIFT)

// MMCHS_PSTAT register fields

#define MMCHS_PSTAT_CMDI                        (1 << 0)
#define MMCHS_PSTAT_DATI                        (1 << 1)
#define MMCHS_PSTAT_DLA                         (1 << 2)
#define MMCHS_PSTAT_WTA                         (1 << 8)
#define MMCHS_PSTAT_RTA                         (1 << 9)
#define MMCHS_PSTAT_BWE                         (1 << 10)
#define MMCHS_PSTAT_BRE                         (1 << 11)
#define MMCHS_PSTAT_WP                          (1 << MMCHS_PSTAT_WP_SHIFT)
#define MMCHS_PSTAT_DLEV                        (0xF << 20)
#define MMCHS_PSTAT_CLEV                        (1 << 24)

// for NETRA
#define MMCHS_PSTAT_CINS                        (1 << MMCHS_PSTAT_CINS_SHIFT)

// MMCHS_HCTL register fields

#define MMCHS_HCTL_DTW                          (1 << 1)
#define MMCHS_HCTL_SDBP                         (1 << 8)
#define MMCHS_HCTL_SDVS(vol)                    ((vol) << 9)
#define MMCHS_HCTL_SBGR                         (1 << 16)
#define MMCHS_HCTL_CR                           (1 << 17)
#define MMCHS_HCTL_RWC                          (1 << 18)
#define MMCHS_HCTL_IBG                          (1 << 19)
#define MMCHS_HCTL_IWE                          (1 << 24)

#define MMCHS_HCTL_SDVS_1V8                     (5 << 9)
#define MMCHS_HCTL_SDVS_3V0                     (6 << 9)
#define MMCHS_HCTL_SDVS_3V3                     (7 << 9)

// MMCHS_SYSCTL register fields

#define MMCHS_SYSCTL_ICE                        (1 << 0)
#define MMCHS_SYSCTL_ICS                        (1 << 1)
#define MMCHS_SYSCTL_CEN                        (1 << 2)
#define MMCHS_SYSCTL_CLKD(clkd)                 ((clkd) << 6)
#define MMCHS_SYSCTL_DTO(dto)                   ((dto) << 16)
#define MMCHS_SYSCTL_SRA                        (1 << 24)
#define MMCHS_SYSCTL_SRC                        (1 << 25)
#define MMCHS_SYSCTL_SRD                        (1 << 26)

#define MMCHS_SYSCTL_DTO_MASK                   (0xF0000)
#define MMCHS_SYSCTL_CLKD_MASK                  (0xFFC0)

// MMCHS_CMD register fields

#define MMCHS_CMD_DE                            (1 << 0)
#define MMCHS_CMD_BCE                           (1 << 1)
#define MMCHS_CMD_ACEN                          (1 << 2)
#define MMCHS_CMD_DDIR                          (1 << 4)
#define MMCHS_CMD_MSBS                          (1 << 5)
#define MMCHS_CMD_RSP_TYPE                      ((rsp) << 16)
#define MMCHS_CMD_CCCE                          (1 << 19)
#define MMCHS_CMD_CICE                          (1 << 20)
#define MMCHS_CMD_DP                            (1 << 21)
#define MMCHS_CMD_TYPE(cmd)                     ((cmd) << 22)
#define MMCHS_INDX(indx)                        ((indx) << 24)

// MMCHS_CAPA register fields

#define MMCHS_CAPA_TCF(tcf)                     ((tcf) << 0)
#define MMCHS_CAPA_TCU                          (1 << 7)
#define MMCHS_CAPA_BCF(bcf)                     ((bcf) << 8)
#define MMCHS_CAPA_MBL(mbl)                     ((mbl) << 16)
#define MMCHS_CAPA_HSS                          (1 << 21)
#define MMCHS_CAPA_DS                           (1 << 22)
#define MMCHS_CAPA_SRS                          (1 << 23)
#define MMCHS_CAPA_VS33                         (1 << 24)
#define MMCHS_CAPA_VS30                         (1 << 25)
#define MMCHS_CAPA_VS18                         (1 << 26)

// MMCHS_CON register fields

#define MMCHS_CON_OD                            (1 << 0)
#define MMCHS_CON_INIT                          (1 << 1)
#define MMCHS_CON_HR                            (1 << 2)
#define MMCHS_CON_STR                           (1 << 3)
#define MMCHS_CON_MODE                          (1 << 4)
#define MMCHS_CON_DW8                           (1 << 5)
#define MMCHS_CON_MIT                           (1 << 6)
#define MMCHS_CON_CDP                           (1 << MMCHS_CON_CDP_SHIFT)
#define MMCHS_CON_WPP                           (1 << MMCHS_CON_WPP_SHIFT)
#define MMCHS_CON_DVAL(v)                       (v << 9)
#define MMCHS_CON_CTPL                          (1 << 11)
#define MMCHS_CON_CEATA                         (1 << 12)
#define MMCHS_CON_OBIP                          (1 << 13)
#define MMCHS_CON_OBIE                          (1 << 14)
#define MMCHS_CON_PADEN                         (1 << 15)
#define MMCHS_CON_CLKEXTFREE                    (1 << 16)

#endif

