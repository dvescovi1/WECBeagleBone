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
//  Header:  am33x_pwmss.h
//
#ifndef __AM33X_PWMSS_H
#define __AM33X_PWMSS_H


//------------------------------------------------------------------------------
// PWMSS
typedef volatile struct {
    UINT32 IDVER;				// 0   
    UINT32 SYSCONFIG;			// 4
    UINT32 CLKCONFIG;			// 8
    UINT32 CLKSTATUS;			// C
} AM33X_PWMSS_REGS;


#define AM33X_ECAP_OFFSET		0x100
#define AM33X_EQEP_OFFSET		0x180
#define	AM33x_EPWM_OFFSET		0x200


// ePWM
typedef volatile struct {
    UINT16 TBCTL;				// 0   
    UINT16 TBSTS;				// 2
    UINT16 TBPHSHR;				// 4
    UINT16 TBPHS;   
    UINT16 TBCNT;
	UINT16 TBPRD;				// A
	UINT16 zzzReserved01;		// 0x0C
	UINT16 CMPCTL;				// 0x0E
	UINT16 CMPAHR;				// 0x10
	UINT16 CMPA;	
	UINT16 CMPB;				// 0x14
	UINT16 AQCTLA;	
    UINT16 AQCTLB;  
    UINT16 AQSFRC;  
    UINT16 AQCSFRC;				// 0x1C
    UINT16 DBCTL;   
    UINT16 DBRED;				// 0x20
    UINT16 DBFED;
    UINT16 TZSEL;   
	UINT16 zzzReserved02;
    UINT16 TZCTL;   
    UINT16 TZEINT;				// 0x2A
	UINT16 TZFLG;
	UINT16 TZCLR;
	UINT16 TZFRC;				// 0x30
	UINT16 ETSEL;
	UINT16 ETPS;
	UINT16 ETFLG;
	UINT16 ETCLR;				// 0x38
	UINT16 ETFRC;
	UINT16 PCCTL;				// 0x3C
	UINT16 zzzReserved03[0x81];
	UINT16 HRCNFG;				// 0xC0
} AM33X_EPWM_REGS;

// eCAP
typedef volatile struct {
    UINT32 TSCTR;				// 0   
    UINT32 CTRPHS;				// 4
    UINT32 CAP1;				// 8
    UINT32 CAP2;				// C
    UINT32 CAP3;				// 0x10
    UINT32 CAP4;				// 0x14
	UINT32 zzzReserved1[4];		// 0x18
	UINT16 ECCTL1;				// 0x28
	UINT16 ECCTL2;				// 0x2A
	UINT16 ECEINT;				// 0x2C
	UINT16 ECFLG;				// 0x2E
	UINT16 ECCLR;				// 0x30
	UINT16 ECFRC;				// 0x32
	UINT32 zzzReserved2[10];
	UINT32 REVID;				// 0x5C
} AM33X_ECAP_REGS;


//------------------------------------------------------------------------------

// TBCNT MODE bits
#define EPWM_TBCNT_CTRLMODE_MASK             	(3 << 0)
#define EPWM_TBCNT_CTRLMODE_COUNT_UP         	(0 << 0)
#define EPWM_TBCNT_CTRLMODE_COUNT_DOWN       	(1 << 0)
#define EPWM_TBCNT_CTRLMODE_UPDOWN           	(2 << 0)
#define EPWM_TBCNT_CTRLMODE_FREEZE           	(3 << 0)

#define EPWM_TBCNT_PHSEN_MASK                	(1 << 2)
#define EPWM_TBCNT_PHSEN_DONT_LOAD           	(0 << 2)
#define EPWM_TBCNT_PHSEN_LOAD                	(1 << 2)

#define EPWM_TBCNT_PRDLD_MASK                	(1 << 3)
#define EPWM_TBCNT_PRDLD_SHADOW              	(0 << 3)
#define EPWM_TBCNT_PRDLD_DONT_SHADOW         	(1 << 3)

#define EPWM_TBCNT_SYNCOSEL_MASK             	(3 << 4)
#define EPWM_TBCNT_SYNCOSEL_EPWMXSYNC        	(0 << 4) // EPWMxSYNC:
#define EPWM_TBCNT_SYNCOSEL_CTR_ZERO         	(1 << 4) // CTR = zero: Time-base counter equal to zero (TBCNT = 0000h)
#define EPWM_TBCNT_SYNCOSEL_CTR_CMPB         	(2 << 4) // CTR = CMPB : Time-base counter equal to counter-compare B (TBCNT = CMPB)
#define EPWM_TBCNT_SYNCOSEL_DISABLE          	(3 << 4) // Disable EPWMxSYNCO signal

#define EPWM_TBCNT_SWFSYNC_MASK              	(1 << 6)
#define EPWM_TBCNT_SWFSYNC_ON                	(1 << 6)

#define EPWM_TBCNT_HSPCLKDIV_MASK            	(7 << 7)
#define EPWM_TBCNT_HSPCLKDIV_DIV_1           	(0 << 7)
#define EPWM_TBCNT_HSPCLKDIV_DIV_2           	(1 << 7)
#define EPWM_TBCNT_HSPCLKDIV_DIV_4           	(2 << 7)
#define EPWM_TBCNT_HSPCLKDIV_DIV_6           	(3 << 7)
#define EPWM_TBCNT_HSPCLKDIV_DIV_8           	(4 << 7)
#define EPWM_TBCNT_HSPCLKDIV_DIV_10          	(5 << 7)
#define EPWM_TBCNT_HSPCLKDIV_DIV_12          	(6 << 7)
#define EPWM_TBCNT_HSPCLKDIV_DIV_14          	(7 << 7)

#define EPWM_TBCNT_CLKDIV_MASK               	(7 << 10)
#define EPWM_TBCNT_CLKDIV_DIV_1              	(0 << 10)
#define EPWM_TBCNT_CLKDIV_DIV_2              	(1 << 10)
#define EPWM_TBCNT_CLKDIV_DIV_4              	(2 << 10)
#define EPWM_TBCNT_CLKDIV_DIV_8              	(3 << 10)
#define EPWM_TBCNT_CLKDIV_DIV_16             	(4 << 10)
#define EPWM_TBCNT_CLKDIV_DIV_32             	(5 << 10)
#define EPWM_TBCNT_CLKDIV_DIV_64             	(6 << 10)
#define EPWM_TBCNT_CLKDIV_DIV_128            	(7 << 10)

#define EPWM_TBCNT_PHSDIR_MASK               	(1 << 13)
#define EPWM_TBCNT_PHSDIR_DOWN               	(0 << 13)
#define EPWM_TBCNT_PHSDIR_UP                 	(1 << 13)

#define EPWM_TBCNT_FREE_SOFT_MASK            	(3 << 14)
#define EPWM_TBCNT_FREE_SOFT_STOP_NEXT_CNT   	(0 << 14)
#define EPWM_TBCNT_FREE_SOFT_STOP_WHOLE_CYC  	(1 << 14)
#define EPWM_TBCNT_FREE_SOFT_FREERUN         	(2 << 14)


// ETSEL bits
#define EPWM_ETSEL_INTSEL_MASK               	(7 << 0)
#define EPWM_ETSEL_INTSEL_TBCNT_ZERO         	(1 << 0) // Enable event time-base counter equal to zero. (TBCNT = 0000h)
#define EPWM_ETSEL_INTSEL_TBCNT_EQ_TBPRD     	(2 << 0) // Enable event time-base counter equal to period (TBCNT = TBPRD)
#define EPWM_ETSEL_INTSEL_TBCNT_EQ_CMPA_INC  	(4 << 0) // Enable event time-base counter equal to CMPA when the timer is incrementing.
#define EPWM_ETSEL_INTSEL_TBCNT_EQ_CMPA_DEC  	(5 << 0) // Enable event time-base counter equal to CMPA when the timer is decrementing.
#define EPWM_ETSEL_INTSEL_TBCNT_EQ_CMPB_INC  	(6 << 0) // Enable event: time-base counter equal to CMPB when the timer is incrementing.
#define EPWM_ETSEL_INTSEL_TBCNT_EQ_CMPB_DEC  	(7 << 0) // Enable event: time-base counter equal to CMPB when the timer is decrementing.

#define EPWM_ETSEL_INTEN_MASK                	(1 << 3)
#define EPWM_ETSEL_INTEN_DISABLE             	(0 << 3)
#define EPWM_ETSEL_INTEN_ENABLE              	(1 << 3)


// ETPS bits
#define EPWM_ETPS_INTPRD_MASK                	(3 << 0)
#define EPWM_ETPS_INTPRD_DISABLE             	(0 << 0)
#define EPWM_ETPS_INTPRD_1ST_EVENT           	(1 << 0)
#define EPWM_ETPS_INTPRD_2ND_EVENT           	(2 << 0)
#define EPWM_ETPS_INTPRD_3RD_EVENT           	(3 << 0)


// AQCTLx bits
#define EPWM_AQCTL_ZRO_MASK        			(3 << 0)
#define EPWM_AQCTL_ZRO_DISABLED    			(0 << 0)
#define EPWM_AQCTL_ZRO_CLEAR       			(1 << 0)
#define EPWM_AQCTL_ZRO_SET         			(2 << 0)
#define EPWM_AQCTL_ZRO_TOGGLE      			(3 << 0)

#define EPWM_AQCTL_PRD_MASK        			(3 << 2)
#define EPWM_AQCTL_PRD_DISABLED    			(0 << 2)
#define EPWM_AQCTL_PRD_CLEAR       			(1 << 2)
#define EPWM_AQCTL_PRD_SET         			(2 << 2)
#define EPWM_AQCTL_PRD_TOGGLE      			(3 << 2)

#define EPWM_AQCTL_CAU_MASK        			(3 << 4)
#define EPWM_AQCTL_CAU_DISABLED    			(0 << 4)
#define EPWM_AQCTL_CAU_CLEAR       			(1 << 4)
#define EPWM_AQCTL_CAU_SET         			(2 << 4)
#define EPWM_AQCTL_CAU_TOGGLE      			(3 << 4)

#define EPWM_AQCTL_CAD_MASK        			(3 << 6)
#define EPWM_AQCTL_CAD_DISABLED    			(0 << 6)
#define EPWM_AQCTL_CAD_CLEAR       			(1 << 6)
#define EPWM_AQCTL_CAD_SET         			(2 << 6)
#define EPWM_AQCTL_CAD_TOGGLE      			(3 << 6)

#define EPWM_AQCTL_CBU_MASK        			(3 << 8)
#define EPWM_AQCTL_CBU_DISABLED    			(0 << 8)
#define EPWM_AQCTL_CBU_CLEAR       			(1 << 8)
#define EPWM_AQCTL_CBU_SET         			(2 << 8)
#define EPWM_AQCTL_CBU_TOGGLE      			(3 << 8)

#define EPWM_AQCTL_CBD_MASK        			(3 << 10)
#define EPWM_AQCTL_CBD_DISABLED    			(0 << 10)
#define EPWM_AQCTL_CBD_CLEAR       			(1 << 10)
#define EPWM_AQCTL_CBD_SET         			(2 << 10)
#define EPWM_AQCTL_CBD_TOGGLE      			(3 << 10)


//------------------------------------------------------------------------------

// ECCTL1 bits
#define ECAP_ECCTL1_CAP1POL			         	(1 << 0)
#define ECAP_ECCTL1_CTRRST1			         	(1 << 1)
#define ECAP_ECCTL1_CAP2POL			         	(1 << 2)
#define ECAP_ECCTL1_CTRRST2			         	(1 << 3)
#define ECAP_ECCTL1_CAP3POL			         	(1 << 4)
#define ECAP_ECCTL1_CTRRST3			         	(1 << 5)
#define ECAP_ECCTL1_CAP4POL			         	(1 << 6)
#define ECAP_ECCTL1_CTRRST4			         	(1 << 7)
#define ECAP_ECCTL1_CAPLDEN			         	(1 << 8)

#define ECAP_ECCTL1_PRESCALE_MASK	         	(0x1f << 9)
#define ECAP_ECCTL1_PRESCALE_DIV_1              (0 << 9)
#define ECAP_ECCTL1_PRESCALE_DIV_2             	(1 << 9)
#define ECAP_ECCTL1_PRESCALE_DIV_4             	(2 << 9)
#define ECAP_ECCTL1_PRESCALE_DIV_6             	(3 << 9)
#define ECAP_ECCTL1_PRESCALE_DIV_8             	(4 << 9)
#define ECAP_ECCTL1_PRESCALE_DIV_10            	(5 << 9)
#define ECAP_ECCTL1_PRESCALE_DIV_60             (0x1e << 9)
#define ECAP_ECCTL1_PRESCALE_DIV_62            	(0x1f << 9)

#define ECAP_ECCTL1_FREE_SOFT_MASK	         	(3 << 14)

// ECCTL2 bits
#define ECAP_ECCTL2_CONT_ONESHT			        (1 << 0)
#define ECAP_ECCTL2_STOP_WRAP_MASK		        (3 << 1)
#define ECAP_ECCTL2_STOP_WRAP_EV1		        (0 << 1)
#define ECAP_ECCTL2_STOP_WRAP_EV2		        (1 << 1)
#define ECAP_ECCTL2_STOP_WRAP_EV3		        (2 << 1)
#define ECAP_ECCTL2_STOP_WRAP_EV4		        (3 << 1)
#define ECAP_ECCTL2_REARM				        (1 << 3)
#define ECAP_ECCTL2_TSCTRSTOP			        (1 << 4)
#define ECAP_ECCTL2_SYNCI_EN			        (1 << 5)
#define ECAP_ECCTL2_SYNCO_SEL_MASK		        (3 << 6)
#define ECAP_ECCTL2_SYNCO_SEL_IN_OUT	        (0 << 6)
#define ECAP_ECCTL2_SYNCO_SEL_PRDEQ		        (1 << 6)
#define ECAP_ECCTL2_SYNCO_SEL_DISABLE	        (2 << 6)
#define ECAP_ECCTL2_SYNCO_SEL_DISABLE2	        (3 << 6)
#define ECAP_ECCTL2_SWSYNC				        (1 << 8)
#define ECAP_ECCTL2_CAP_APWM			        (1 << 9)
#define ECAP_ECCTL2_APWMPOL				        (1 << 10)

// ECEINT,ECFLG,ECCLR,ECFRC bits
#define ECAP_ECEINT_CEVT1				        (1 << 1)
#define ECAP_ECEINT_CEVT2				        (1 << 2)
#define ECAP_ECEINT_CEVT3				        (1 << 3)
#define ECAP_ECEINT_CEVT4				        (1 << 4)
#define ECAP_ECEINT_CNTOVF				        (1 << 5)
#define ECAP_ECEINT_PRDEQ				        (1 << 6)
#define ECAP_ECEINT_CMPEQ				        (1 << 7)


//------------------------------------------------------------------------------

#endif // __AM33X_PWMSS_H

