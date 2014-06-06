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
//  Header: omap_hecc_scc_regs.h
//

#ifndef  __OMAP_HECC_SCC_REGS_H
#define  __OMAP_HECC_SCC_REGS_H

#ifndef HECC
#ifndef SCC
#error A CAN controller model (HECC or SCC) must be selected
#endif
#endif

#ifdef HECC
#define NB_MAILBOXES    (32)
#endif
#ifdef SCC
#define NB_MAILBOXES    (16)
#endif


typedef struct {
    REG32 CANMID;   // Message identifier
    REG32 CANMCF;   // Message control field
    REG32 CANMDL;   // Message data low word
    REG32 CANMDH;   // Message data high word
}OMAP_HECC_SCC_MAILBOX;

typedef struct {
    REG32 CANME;   // Mailbox enable
    REG32 CANMD;   // Mailbox direction
    REG32 CANTRS;  // Transmit request set
    REG32 CANTRR;  // Transmit request reset
    REG32 CANTA;   // Transmission acknowledge
    REG32 CANAA;   // Abort acknowledge
    REG32 CANRMP;  // Receive message pending
    REG32 CANRML;  // Receive message lost
    REG32 CANRFP;  // Remote frame pending
#ifdef SCC
    REG32 CANGAM;  // Global acceptance mask
#else
    REG32 reserved1;
#endif
    REG32 CANMC;    // Master control
    REG32 CANBTC;   // Bit-timing configuration
    REG32 CANES;    // Error and status
    REG32 CANTEC;   // Transmit error counter
    REG32 CANREC;   // Receive error counter
    REG32 CANGIF0;  // Global interrupt flag 0
    REG32 CANGIM;   // Global interrupt mask
    REG32 CANGIF1;  // Global interrupt flag 1
    REG32 CANMIM;   // Mailbox interrupt mask
    REG32 CANMIL;   // Mailbox interrupt level
    REG32 CANOPC;   // Overwrite protection control
    REG32 CANTIOC;  // Transmit I/O control
    REG32 CANRIOC;  // Receive I/O control
#ifdef HECC
    REG32 CANLNT;   // Local network time
    REG32 CANTOC;   // Time-out control
    REG32 CANTOS;   // Time-out status
#else
    REG32 reserved1[3];
#endif
    UINT32 reserved2[2022];
    OMAP_HECC_SCC_MAILBOX MailBoxes[NB_MAILBOXES];
#ifdef HECC
    UINT32 reserved3[0x380];
#endif
#ifdef SCC
    UINT32 reserved3[0x3C0];
#endif
#ifdef SCC
    REG32 CANLAM0;  // Local acceptance mask 0
    REG32 CANLAM3;  // Local acceptance mask 3
#endif
#ifdef HECC
    REG32 CANLAM[32];   // Local acceptance mask
    REG32 CANMOTS[32];  // Message time stamp    
    REG32 CANMOTO[32];  // Message time-out
#endif
} OMAP_HECC_SCC_REGS;



#define CANMC_LNTC    (1<<15)   //Local Network Time Clear Bit
#define CANMC_LNTM    (1<<14)   //Local Network Time MSB Clear Bit
#define CANMC_SCM     (1<<13)   //SCC-Compatibility Mode
#define CANMC_CCR     (1<<12)   //Change Configuration Request
#define CANMC_PDR     (1<<11)   //Power-Down-Mode Request
#define CANMC_DBO     (1<<10)   //Data Byte Order
#define CANMC_WUBA    (1<<9)    //Wake Up on Bus Activity
#define CANMC_CDR     (1<<8)    //Change Data Request
#define CANMC_ABO     (1<<7)    //Auto Bus On
#define CANMC_STM     (1<<6)    //Self-Test Mode
#define CANMC_SRES    (1<<5)    //Software Reset



#define CANBTC_ERM      (1<<10) //Edge Resynchronization
#define CANBTC_SAM      (1<<7)  //Sample point
    


#define CANES_FE    (1<<24) //Form Error Flag
#define CANES_BE    (1<<23) //Bit Error Flag
#define CANES_SA1   (1<<22) //Stuck at dominant error
#define CANES_CRCE  (1<<21) //CRC Error
#define CANES_SE    (1<<20) //Stuff Error
#define CANES_ACKE  (1<<19) //Acknowledge Error
#define CANES_BO    (1<<18) //Bus-Off State
#define CANES_EP    (1<<17) //Error Passive State
#define CANES_EW    (1<<16) //Warning Status
#define CANES_SMA   (1<<5)  //Suspend Mode
#define CANES_CCE   (1<<4)  //Change Configuration enable
#define CANES_PDA   (1<<3)  //Power Down Mode Acknowledge
#define CANES_RM    (1<<1)  //The CAN protocol kernel is receiving a message
#define CANES_TM    (1<<0)  //The CAN protocol kernel is transmitting a message

#define CANGIx_MAIM   (1<<17)      //Message Alarm Interrupt
#define CANGIx_TCOIM  (1<<16)      //Local Network Time Counter Overflow Interrupt
#define CANGIF_GMIFx  (1<<15)      //Global Mailbox Interrupt Flag
#define CANGIx_AAIM   (1<<14)      //Abort Acknowledge Interrupt
#define CANGIx_WDIM   (1<<13)      //Write Denied Interrupt
#define CANGIx_WUIM   (1<<12)      //Wake-Up Interrupt
#define CANGIx_RMLIM  (1<<11)      //Receive Message Lost Interrupt 
#define CANGIx_BOIM   (1<<10)      //Bus Off Interrupt 
#define CANGIx_EPIM   (1<<9)       //Error Passive Interrupt 
#define CANGIx_WLIM   (1<<8)       //Warning Level Interrupt 
#define CANGIM_SIL    (1<<2)       //System Interrupt Level. Define the interrupt level for TCOIF, WDIF, WUIF, BOIF, EPIF, AAIF, RMLIF and WLIF.
                                            //0 : All global interrupts are mapped to the HECC0INT/SCC0INT interrupt line.
                                            //1 : All system interrupts are mapped to the HECC1INT/SCC1INT interrupt line.
#define CANGIM_I1EN   (1<<1)       //Interrupt Line 1 Enable
#define CANGIM_I0EN   (1<<0)       //Interrupt Line 0 Enable

#define CANTIOC_TXDIR   (1<<2)
#define CANTIOC_TXFUNC  (1<<3)

#define CANRIOC_RXFUNC  (1<<3)

#define CANMID_EXTENDED     (1<<31)
#define CANMID_AME          (1<<30)
#define CANMID_AMM          (1<<29)
#define CANMID_IDMASK       (0x1FFFFFFF)

#define CANLAM_LAMI      (1<<31)

#define CANMCF_RTR          (1<<4)
                                            
#endif //__OMAP_HECC_SCC_REGS_H