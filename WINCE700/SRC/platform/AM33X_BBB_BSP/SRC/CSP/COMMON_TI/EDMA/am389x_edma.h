//
// Copyright (c) MPC Data Limited 2007. All rights reserved.
//
//------------------------------------------------------------------------------
//
//  File:  am389x_edma.h
//
//  AM389X EDMA Peripheral Register Definitions.
//  

#ifndef __AM389X_EDMA_H
#define __AM389X_EDMA_H

//------------------------------------------------------------------------------

// EDMA3 Transfer Controller Registers (Per Destination FIFO)
typedef struct  __EDMATCDSTFIFOREGS__ {
    volatile UINT32    DFOPT;
    volatile UINT32    DFSRC;
    volatile UINT32    DFCNT;
    volatile UINT32    DFDST;
    volatile UINT32    DFBIDX;
    volatile UINT32    DFMPPRXY;
    volatile UINT8     RSVD0[40];
} AM389X_EDMATC_DSTFIFO_REGS, *PEDMATCDSTFIFOREGS;

// EDMA3 Transfer Controller Registers
typedef struct  __EDMATCREGS__ {
    volatile UINT32    REV;
    volatile UINT32    TCCFG;
    volatile UINT8     RSVD0[248];
    volatile UINT32    TPTCSTAT;
    volatile UINT32    INTSTAT;
    volatile UINT32    INTEN;
    volatile UINT32    INTCLR;
    volatile UINT32    INTCMD;
    volatile UINT8     RSVD1[12];
    volatile UINT32    ERRSTAT;
    volatile UINT32    ERREN;
    volatile UINT32    ERRCLR;
    volatile UINT32    ERRDET;
    volatile UINT32    ERRCMD;
    volatile UINT8     RSVD2[12];
    volatile UINT32    RDRATE;
    volatile UINT8     RSVD3[188];
    volatile UINT32    POPT;
    volatile UINT32    PSRC;
    volatile UINT32    PCNT;
    volatile UINT32    PDST;
    volatile UINT32    PBIDX;
    volatile UINT32    PMPPRXY;
    volatile UINT8     RSVD4[40];
    volatile UINT32    SAOPT;
    volatile UINT32    SASRC;
    volatile UINT32    SACNT;
    volatile UINT32    SADST;
    volatile UINT32    SABIDX;
    volatile UINT32    SAMPPRXY;
    volatile UINT32    SACNTRLD;
    volatile UINT32    SASRCBREF;
    volatile UINT32    SADSTBREF;
    volatile UINT8     RSVD5[28];
    volatile UINT32    DFCNTRLD;
    volatile UINT32    DFSRCBREF;
    volatile UINT32    DFDSTBREF;
    volatile UINT8     RSVD6[116];
    AM389X_EDMATC_DSTFIFO_REGS DFIREG[4];
} 
AM389X_EDMATC_REGS, *PEDMATCREGS;

// EDMA Channel Controller Registers (DMA Access Regions)
typedef struct  __EDMACCDRAREGS__ {
    volatile UINT32         DRAE;
    volatile UINT32         DRAEH;
} 
AM389X_EDMACC_DRA_REGS, *PEDMACCDRAREGS;

// EDMA Channel Controller Registers (Event Que Entries)
typedef struct  __EDMACCEQEREGS__ {
    volatile UINT32         EVT_ENTRY;
} 
AM389X_EDMACC_EQE_REGS, PEDMACCEQEREGS;

// EDMA Channel Controller Registers (Shadow Regions, padded to 512B)
typedef struct  __EDMACCSRREGS__ {
    volatile UINT32         ER;
    volatile UINT32         ERH;
    volatile UINT32         ECR;
    volatile UINT32         ECRH;
    volatile UINT32         ESR;
    volatile UINT32         ESRH;
    volatile UINT32         CER;
    volatile UINT32         CERH;
    volatile UINT32         EER;
    volatile UINT32         EERH;
    volatile UINT32         EECR;
    volatile UINT32         EECRH;
    volatile UINT32         EESR;
    volatile UINT32         EESRH;
    volatile UINT32         SER;
    volatile UINT32         SERH;
    volatile UINT32         SECR;
    volatile UINT32         SECRH;
    volatile UINT8          RSVD0[8];
    volatile UINT32         IER;
    volatile UINT32         IERH;
    volatile UINT32         IECR;
    volatile UINT32         IECRH;
    volatile UINT32         IESR;
    volatile UINT32         IESRH;
    volatile UINT32         IPR;
    volatile UINT32         IPRH;
    volatile UINT32         ICR;
    volatile UINT32         ICRH;
    volatile UINT32         IEVAL;
    volatile UINT8          RSVD1[4];
    volatile UINT32         QER;
    volatile UINT32         QEER;
    volatile UINT32         QEECR;
    volatile UINT32         QEESR;
    volatile UINT32         QSER;
    volatile UINT32         QSECR;
    volatile UINT8          RSVD2[360];
} 
AM389X_EDMACC_SR_REGS, *PEDMACCSRREGS;


// EDMA Channel Controller Registers (Parameter RAM Entries)
typedef struct  __EDMACCPREREGS__ {
    volatile UINT32         OPT;
    volatile UINT32         SRC;
    volatile UINT32         A_B_CNT;
    volatile UINT32         DST;
    volatile UINT32         SRC_DST_BIDX;
    volatile UINT32         LINK_BCNTRLD;
    volatile UINT32         SRC_DST_CIDX;
    volatile UINT32         CCNT;
} 
AM389X_EDMACC_PRE_REGS, *PEDMACCPREREGS;

// EDMA Channel Controller Registers
typedef struct  __EDMACCREGS__ {
    volatile UINT32         REV;					// 0000  
    volatile UINT32         CCCFG;					// 0004
    volatile UINT8          RSVD0[248];
    volatile UINT32         DCHMAP[64];				// 0100-01FC
    volatile UINT32         QCHMAP[8];				// 0200-021C
    volatile UINT8          RSVD1[32];
    volatile UINT32         DMAQNUM[8];				// 0240-025C
    volatile UINT32         QDMAQNUM;				// 0260
    volatile UINT8          RSVD2[32];
    volatile UINT32         QUEPRI;					// 0284
    volatile UINT8          RSVD3[120];
    volatile UINT32         EMR;					// 0300
    volatile UINT32         EMRH;					// 0304
    volatile UINT32         EMCR;					// 0308
    volatile UINT32         EMCRH;					// 030C
    volatile UINT32         QEMR;					// 0310
    volatile UINT32         QEMCR;					// 0314
    volatile UINT32         CCERR;					// 0318
    volatile UINT32         CCERRCLR;				// 031C
    volatile UINT32         EEVAL;					// 0320
    volatile UINT8          RSVD4[28];
    AM389X_EDMACC_DRA_REGS  DRA[8];					// 0340,0344 - 0378,037C
    volatile UINT32         QRAE[8];				// 0380-039C
    volatile UINT8          RSVD5[96];
    AM389X_EDMACC_EQE_REGS  QUEEVTENTRY[4][16];		// 0400-04FC
    volatile UINT8          RSVD6[256];
    volatile UINT32         QSTAT[4];				// 0600-060C
    volatile UINT8          RSVD7[16];
    volatile UINT32         QWMTHRA;				// 0620
    volatile UINT8          RSVD8[28];
    volatile UINT32         CCSTAT;					// 0640
    volatile UINT8          RSVD9[444];
    volatile UINT32         MPFAR;					// 0800
    volatile UINT32         MPFSR;					// 0804
    volatile UINT32         MPFCR;					// 0808
    volatile UINT32         MPPAG;					// 080C
    volatile UINT32         MPPA[8];				// 0810-082C
    volatile UINT8          RSVD10[2000];
    volatile UINT32         ER;						// 1000
    volatile UINT32         ERH;					// 1004
    volatile UINT32         ECR;					// 1008
    volatile UINT32         ECRH;					// 100C
    volatile UINT32         ESR;					// 1010
    volatile UINT32         ESRH;					// 1014
    volatile UINT32         CER;					// 1018
    volatile UINT32         CERH;					// 101C
    volatile UINT32         EER;					// 1020
    volatile UINT32         EERH;					// 1024
    volatile UINT32         EECR;					// 1028
    volatile UINT32         EECRH;					// 102C
    volatile UINT32         EESR;					// 1030
    volatile UINT32         EESRH;					// 1034
    volatile UINT32         SER;					// 1038
    volatile UINT32         SERH;					// 103C
    volatile UINT32         SECR;					// 1040
    volatile UINT32         SECRH;					// 1044
    volatile UINT8          RSVD11[8];
    volatile UINT32         IER;					// 1050
    volatile UINT32         IERH;					// 1054
    volatile UINT32         IECR;					// 1058
    volatile UINT32         IECRH;					// 105C
    volatile UINT32         IESR;					// 1060
    volatile UINT32         IESRH;					// 1064
    volatile UINT32         IPR;					// 1068
    volatile UINT32         IPRH;					// 106C
    volatile UINT32         ICR;					// 1070
    volatile UINT32         ICRH;					// 1074
    volatile UINT32         IEVAL;					// 1078
    volatile UINT8          RSVD12[4];
    volatile UINT32         QER;					// 1080
    volatile UINT32         QEER;					// 1084
    volatile UINT32         QEECR;					// 1088
    volatile UINT32         QEESR;					// 108C
    volatile UINT32         QSER;					// 1090
    volatile UINT32         QSECR;					// 1094
    volatile UINT8          RSVD13[3944];
    AM389X_EDMACC_SR_REGS   SHADOW[8];				// 2000, 2200, 2400, 2600, 2800, 2A00, 2C00, 2E00
    volatile UINT8          RSVD14[4096];
    AM389X_EDMACC_PRE_REGS  PARAMENTRY[512];		// 4000
} 
AM389X_EDMACC_REGS, *PEDMACCREGS;


#endif
