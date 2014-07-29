/*
*  File: am33x_sdhc_regs.h
*/
#ifndef __AM33X_SDHC_REGS_H
#define __AM33X_SDHC_REGS_H

#define MMCSLOT_1   1
#define MMCSLOT_2   2
#define MMCSLOT_3   3

//
//  MMC/SD/SDIO Registers
//
typedef volatile struct
{
    UINT32 unused0[68];			// 0x0000 - 0x010C
    UINT32 MMCHS_SYSCONFIG;		// 0x0110
    UINT32 MMCHS_SYSSTATUS;		// 0x0114
    UINT32 unused1[3];			// 0x0118 - 0x0120
    UINT32 MMCHS_CSRE;			// 0x0124
    UINT32 MMCHS_SYSTEST;		// 0x0128
    UINT32 MMCHS_CON;			// 0x012C
    UINT32 MMCHS_PWCNT;			// 0x0130
    UINT32 unused2[51];			// 0x0134 - 0x00FC
	UINT32 MMCHS_SDMASA;		// 0x0200
    UINT32 MMCHS_BLK;			// 0x0204
    UINT32 MMCHS_ARG;			// 0x0208
    UINT32 MMCHS_CMD;			// 0x020C
    UINT32 MMCHS_RSP10;			// 0x0210
    UINT32 MMCHS_RSP32;			// 0x0214
    UINT32 MMCHS_RSP54;			// 0x0218
    UINT32 MMCHS_RSP76;			// 0x021C
    UINT32 MMCHS_DATA;			// 0x0220
    UINT32 MMCHS_PSTATE;		// 0x0224
    UINT32 MMCHS_HCTL;			// 0x0228
    UINT32 MMCHS_SYSCTL;		// 0x022C
    UINT32 MMCHS_STAT;			// 0x0230
    UINT32 MMCHS_IE;			// 0x0234
    UINT32 MMCHS_ISE;			// 0x0238
    UINT32 MMCHS_AC12;			// 0x023C
    UINT32 MMCHS_CAPA;			// 0x0240
    UINT32 unused4[1];			// 0x0244
    UINT32 MMCHS_CUR_CAPA;		// 0x0248
    UINT32 unused5[1];			// 0x024C
    UINT32 MMCHS_FE;			// 0x0250
    UINT32 MMCHS_ADMAES;		// 0x0254
    UINT32 MMCHS_ADMASAL;		// 0x0258
    UINT32 MMCHS_ADMASAH;		// 0x025C
	UINT32 unused6[39];			// 0x0260 - 0x01F8
    UINT32 MMCHS_REV;			// 0x02FC
} AM33X_MMCHS_REGS, *PAM33X_MMCHS_REGS;


// MMCHS_SYSCONFIG register fields
#define MMCHS_SYSCONFIG_AUTOIDLE                (1 << 0)
#define MMCHS_SYSCONFIG_SOFTRESET               (1 << 1)
#define MMCHS_SYSCONFIG_ENAWAKEUP               (1 << 2)
#define MMCHS_SYSCONFIG_SIDLEMODE(mode)         ((mode) << 3)
#define MMCHS_SYSCONFIG_CLOCKACTIVITY(act)      ((act) << 8)

#define SIDLE_FORCE                             (0)
#define SIDLE_IGNORE                            (1)
#define SIDLE_SMART                             (2)

#define CLOCKACTIVITY_AUTOOFF					(0 << 8)
#define CLOCKACTIVITY_F_ON						(1 << 8)
#define CLOCKACTIVITY_I_ON						(2 << 8)
#define CLOCKACTIVITY_IF_ON						(3 << 8)

// MMCHS_SYSSTATUS register fields
#define MMCHS_SYSSTATUS_RESETDONE               (1 << 0)


// MMCHS_CON register fields
#define MMCHS_CON_OD                            (1 << 0)
#define MMCHS_CON_INIT                          (1 << 1)
#define MMCHS_CON_HR                            (1 << 2)
#define MMCHS_CON_STR                           (1 << 3)
#define MMCHS_CON_MODE                          (1 << 4)
#define MMCHS_CON_DW8                           (1 << 5)
#define MMCHS_CON_MIT                           (1 << 6)
#define MMCHS_CON_CDP                           (1 << 7)
#define MMCHS_CON_WPP                           (1 << 8)
#define MMCHS_CON_DVAL(v)                       (v << 9)
#define MMCHS_CON_CTPL                          (1 << 11)
#define MMCHS_CON_CEATA                         (1 << 12)
#define MMCHS_CON_PADEN                         (1 << 15)
#define MMCHS_CON_CLKEXTFREE                    (1 << 16)
#define MMCHS_CON_BOOT_ACK	                    (1 << 17)
#define MMCHS_CON_BOOT_CF0	                    (1 << 18)
#define MMCHS_CON_DDR		                    (1 << 19)
#define MMCHS_CON_DMA_MNS	                    (1 << 20)
#define MMCHS_CON_SDMA_LNE	                    (1 << 21)


// MMCHS_BLK register fields
#define MMCHS_BLK_BLEN(len)                     ((len) << 0)
#define MMCHS_BLK_NBLK(num)                     ((num) << 16)


// MMCHS_CMD register fields
#define MMCHS_CMD_DE                            (1 << 0)
#define MMCHS_CMD_BCE                           (1 << 1)
#define MMCHS_CMD_ACEN                          (1 << 2)
#define MMCHS_CMD_DDIR                          (1 << 4)
#define MMCHS_CMD_MSBS                          (1 << 5)
#define MMCHS_CMD_RSP_TYPE(rsp)                 ((rsp) << 16)
#define MMCHS_CMD_CCCE                          (1 << 19)
#define MMCHS_CMD_CICE                          (1 << 20)
#define MMCHS_CMD_DP                            (1 << 21)
#define MMCHS_CMD_TYPE(cmd)                     ((cmd) << 22)
#define MMCHS_INDX(indx)                        ((indx) << 24)

typedef enum
{
    SDHC_CMD_NORMAL = 0,
    SDHC_CMD_SUSPEND = 1,
    SDHC_CMD_RESUME = 2,
    SDHC_CMD_ABORT = 3,
}SDHCCmdType;

typedef enum
{
    SDHC_RSPLEN_0 = 0,
    SDHC_RSPLEN_136 = 1,
    SDHC_RSPLEN_48 = 2,
    SDHC_RSPLEN_48B = 3,
}SDHCRspType;


// MMCHS_PSTATE register fields
#define MMCHS_PSTATE_CMDI                    	(1 << 0)
#define MMCHS_PSTATE_DATI                    	(1 << 1)
#define MMCHS_PSTATE_DLA                     	(1 << 2)
#define MMCHS_PSTATE_WTA                     	(1 << 8)
#define MMCHS_PSTATE_RTA                     	(1 << 9)
#define MMCHS_PSTATE_BWE                     	(1 << 10)
#define MMCHS_PSTATE_BRE                     	(1 << 11)
#define MMCHS_PSTATE_CINS                    	(1 << 16)
#define MMCHS_PSTATE_CSS                     	(1 << 17)
#define MMCHS_PSTATE_CDPL                    	(1 << 18)
#define MMCHS_PSTATE_WP                      	(1 << 19)
#define MMCHS_PSTATE_DLEV(bits)                	((bits) << 20)
#define MMCHS_PSTATE_CLEV                    	(1 << 24)


// MMCHS_HCTL register fields
#define MMCHS_HCTL_DTW                          (1 << 1)
#define MMCHS_HCTL_HSPE                         (1 << 2)
#define MMCHS_HCTL_DMAS(mode)                   ((mode) << 3)
#define MMCHS_HCTL_CDTL                         (1 << 6)
#define MMCHS_HCTL_CDSS                         (1 << 7)
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

#define MMCHS_HCTL_DMAS_32BIT_ADMA2				(3 << 3)


// MMCHS_SYSCTL register fields
#define MMCHS_SYSCTL_ICE                        (1 << 0)
#define MMCHS_SYSCTL_ICS                        (1 << 1)
#define MMCHS_SYSCTL_CEN                        (1 << 2)
#define MMCHS_SYSCTL_CLKD(clkd)                 ((clkd) << 6)
#define MMCHS_SYSCTL_DTO(dto)                   ((dto) << 16)
#define MMCHS_SYSCTL_SRA                        (1 << 24)
#define MMCHS_SYSCTL_SRC                        (1 << 25)
#define MMCHS_SYSCTL_SRD                        (1 << 26)

#define MMCHS_SYSCTL_DTO_MASK                   (0x000F0000)
#define MMCHS_SYSCTL_CLKD_MASK                  (0x0000FFC0)
#define MMCHS_SYSCTL_DTO_MAX_VAL				0xE   // data timeout will occur after SDCLK * 2^27 ticks


// MMCHS_STAT register fields
#define MMCHS_STAT_CC                           (1 << 0)
#define MMCHS_STAT_TC                           (1 << 1)
#define MMCHS_STAT_BGE                          (1 << 2)
#define MMCHS_STAT_DMA                          (1 << 3)
#define MMCHS_STAT_BWR                          (1 << 4)
#define MMCHS_STAT_BRR                          (1 << 5)
#define MMCHS_STAT_CINS                         (1 << 6)
#define MMCHS_STAT_CREM                         (1 << 7)
#define MMCHS_STAT_CIRQ                         (1 << 8)
#define MMCHS_STAT_OBI                          (1 << 9)
#define MMCHS_STAT_BSR                          (1 << 10)
#define MMCHS_STAT_ERRI                         (1 << 15)
#define MMCHS_STAT_CTO                          (1 << 16)
#define MMCHS_STAT_CCRC                         (1 << 17)
#define MMCHS_STAT_CEB                          (1 << 18)
#define MMCHS_STAT_CIE                          (1 << 19)
#define MMCHS_STAT_DTO                          (1 << 20)
#define MMCHS_STAT_DCRC                         (1 << 21)
#define MMCHS_STAT_DEB                          (1 << 22)
#define MMCHS_STAT_ACE                          (1 << 24)
#define MMCHS_STAT_ADMAE                        (1 << 25)
#define MMCHS_STAT_CERR                         (1 << 28)
#define MMCHS_STAT_BADA                         (1 << 29)

#define MMCHS_CMD_ERROR_BITS					(MMCHS_STAT_CIE | MMCHS_STAT_CEB | MMCHS_STAT_CCRC | MMCHS_STAT_CTO)
#define MMCHS_DAT_ERROR_BITS					(MMCHS_STAT_DEB | MMCHS_STAT_DCRC | MMCHS_STAT_DTO)
//TODO:#define MMCHS_ERROR_BITS						(0x11000000 | MMCHS_DAT_ERROR_BITS | MMCHS_CMD_ERROR_BITS) 
#define MMCHS_ERROR_BITS						(0x11000000 | MMCHS_DAT_ERROR_BITS | MMCHS_CMD_ERROR_BITS) 


// MMCHS_IE register fields
#define MMCHS_IE_CC                             (1 << 0)
#define MMCHS_IE_TC                             (1 << 1)
#define MMCHS_IE_BGE                            (1 << 2)
#define MMCHS_IE_DMA                            (1 << 3)
#define MMCHS_IE_BWR                            (1 << 4)
#define MMCHS_IE_BRR                            (1 << 5)
#define MMCHS_IE_CINS                           (1 << 6)
#define MMCHS_IE_CREM                           (1 << 7)
#define MMCHS_IE_CIRQ                           (1 << 8)
#define MMCHS_IE_OBI                            (1 << 9)
#define MMCHS_IE_BSR                            (1 << 10)
#define MMCHS_IE_NULL                           (1 << 15)
#define MMCHS_IE_CTO                            (1 << 16)
#define MMCHS_IE_CCRC                           (1 << 17)
#define MMCHS_IE_CEB                            (1 << 18)
#define MMCHS_IE_CIE                            (1 << 19)
#define MMCHS_IE_DTO                            (1 << 20)
#define MMCHS_IE_DCRC                           (1 << 21)
#define MMCHS_IE_DEB                            (1 << 22)
#define MMCHS_IE_ACE                            (1 << 24)
#define MMCHS_IE_ADMA                           (1 << 25)
#define MMCHS_IE_CERR                           (1 << 28)
#define MMCHS_IE_BADA                           (1 << 29)


// MMCHS_ISE register fields
#define MMCHS_ISE_CC                            (1 << 0)
#define MMCHS_ISE_TC                            (1 << 1)
#define MMCHS_ISE_BGE                           (1 << 2)
#define MMCHS_ISE_DMA                           (1 << 3)
#define MMCHS_ISE_BWR                           (1 << 4)
#define MMCHS_ISE_BRR                           (1 << 5)
#define MMCHS_ISE_CINS                          (1 << 6)
#define MMCHS_ISE_CREM                          (1 << 7)
#define MMCHS_ISE_CIRQ                          (1 << 8)
#define MMCHS_ISE_OBI                           (1 << 9)
#define MMCHS_ISE_BSR                           (1 << 10)
#define MMCHS_ISE_NULL                          (1 << 15)
#define MMCHS_ISE_CTO                           (1 << 16)
#define MMCHS_ISE_CCRC                          (1 << 17)
#define MMCHS_ISE_CEB                           (1 << 18)
#define MMCHS_ISE_CIE                           (1 << 19)
#define MMCHS_ISE_DTO                           (1 << 20)
#define MMCHS_ISE_DCRC                          (1 << 21)
#define MMCHS_ISE_DEB                           (1 << 22)
#define MMCHS_ISE_ACE                           (1 << 24)
#define MMCHS_ISE_ADMA                          (1 << 25)
#define MMCHS_ISE_CERR                          (1 << 28)
#define MMCHS_ISE_BADA                          (1 << 29)


// MMCHS_AC12 register fields
#define MMCHS_AC12_ACNE                         (1 << 0)
#define MMCHS_AC12_ACTO                         (1 << 1)
#define MMCHS_AC12_ACCE                         (1 << 2)
#define MMCHS_AC12_ACEB                         (1 << 3)
#define MMCHS_AC12_ACIE                         (1 << 4)
#define MMCHS_AC12_CNI                          (1 << 7)


// MMCHS_CAPA register fields
#define MMCHS_CAPA_TCF(tcf)                     ((tcf) << 0)
#define MMCHS_CAPA_TCU                          (1 << 7)
#define MMCHS_CAPA_BCF(bcf)                     ((bcf) << 8)
#define MMCHS_CAPA_MBL(mbl)                     ((mbl) << 16)
#define MMCHS_CAPA_AD2S                         (1 << 19)
#define MMCHS_CAPA_HSS                          (1 << 21)
#define MMCHS_CAPA_DS                           (1 << 22)
#define MMCHS_CAPA_SRS                          (1 << 23)
#define MMCHS_CAPA_VS33                         (1 << 24)
#define MMCHS_CAPA_VS30                         (1 << 25)
#define MMCHS_CAPA_VS18                         (1 << 26)
#define MMCHS_CAPA_BUS_64BIT                    (1 << 28)


// MMCHS_FE register fields
#define MMCHS_FE_ACNE                           (1 << 0)
#define MMCHS_FE_ACTO                           (1 << 1)
#define MMCHS_FE_ACCE                           (1 << 2)
#define MMCHS_FE_ACEB                           (1 << 3)
#define MMCHS_FE_ACIE                           (1 << 4)
#define MMCHS_FE_CNI                            (1 << 7)
#define MMCHS_FE_CTO                            (1 << 16)
#define MMCHS_FE_CCRC                           (1 << 17)
#define MMCHS_FE_CEB                            (1 << 18)
#define MMCHS_FE_CIE                            (1 << 19)
#define MMCHS_FE_DTO                            (1 << 20)
#define MMCHS_FE_DCRC                           (1 << 21)
#define MMCHS_FE_DEB                            (1 << 22)
#define MMCHS_FE_ACE                            (1 << 24)
#define MMCHS_FE_ADMA                           (1 << 25)
#define MMCHS_FE_CERR                           (1 << 28)
#define MMCHS_FE_BADA                           (1 << 29)


#endif

