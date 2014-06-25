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

//------------------------------------------------------------------------------
//
//  File:  omap_uart_regs.h
//
//  This header file is comprised of UART module register details defined as
//  structures and macros for configuring and controlling UART module

#ifndef __OMAP_UART_REGS_H
#define __OMAP_UART_REGS_H
//------------------------------------------------------------------------------

#pragma warning(push)
#pragma warning(disable: 4201)
typedef struct {

   union {                          //offset 0x0   
      REG8 DLL;                    //config mode A&B, read and write
      REG8 RHR;                    //operational mode read
      REG8 THR;                    //operational mode write      
   };   
   REG8 RESERVED_1[3];

   union {                          //offset 0x4   
      REG8 DLH;                    //config mode A&B, read and write
      REG8 IER;                    //operational mode read and write      
   };   
   REG8  RESERVED_2[3];
   
   union {                          //offset 0x8   
      REG8 IIR;                    //config mode A read, oper mode read
      REG8 FCR;                    //config mode A write,oper mode write
      REG8 EFR;                    //config mode B read and write      
   };
   REG8  RESERVED_3[3];
   
   REG8 LCR;                       //offset 0xC
   REG8  RESERVED_4[3];
   
   union {                          //offset 0x10   
      REG8 MCR;                    //config mode A read & write, op mode read & write
      REG8 XON1_ADDR1;             //config mode B read & write      
   };
   REG8  RESERVED_5[3];
   
   union {                          //offset 0x14   
      REG8 LSR;                    //config mode A read, oper mode read
      REG8 XON2_ADDR2;             //config mode B read & write\      
   };
   REG8  RESERVED_6[3];
   
   union {                          //offset 0x18   
      REG8 MSR;                    //config mode A read & oper mode read
      REG8 TCR;                    //all modes
      REG8 XOFF1;                  //config mode B read & write\      
   };
   REG8  RESERVED_7[3];
   
   union {                          //offset 0x1C   
      REG8 TLR;                    //both config mode & oper mode
      REG8 XOFF2;                  //config mode B read & write
      REG8 SPR;                    //config mode A, oper mode read &write      
   };
   REG8  RESERVED_8[3];
   
   REG8 MDR1;                      //offset 0x20   
   REG8  RESERVED_9[3];
   
   REG8 MDR2;                      //offset 0x24
   REG8  RESERVED_10[3];
   
   union {                          //offset 0x28   
      REG8 SFLSR;                  //config mode,oper mode read
      REG8 TXFLL;                  //config mode,oper mode write      
   };
   REG8  RESERVED_11[3];
   
   union {                          //offset 0x2C   
      REG8 RESUME;                 //config mode,oper mode read
      REG8 TXFLH;                  //config mode,oper mode write      
   };   
   REG8  RESERVED_12[3];
   
   union {                          //offset 0x30   
      REG8 SFREGL;                 //config mode,oper mode read
      REG8 RXFLL;                  //config mode,oper mode write      
   };
   REG8  RESERVED_13[3];
   
   union {                          //offset 0x34   
      REG8 SFREGH;                 //config mode,oper mode read
      REG8 RXFLH;                  //config mode,oper mode write      
   };
   REG8  RESERVED_14[3];
   
   union {                          //offset 0x38   
      REG8 UASR;                   //config mode read only
      REG8 BLR;                    //oper mode
      REG8 RESERVED_WRITE_0x30;      
   };
   REG8  RESERVED_15[3];
   
   union {                          //offset 0x3C   
      REG8 ACR;                    //oper mode read & write
      REG8 RESERVED_CONFIG_0x3C;
   };
   REG8  RESERVED_16[3];
   
   REG8 SCR;                       //offset 0x40   
   REG8 RESERVED_17[3];
   
   union {                          //offset 0x44   
      REG8 SSR;                    //read only
      REG8 RESERVED_WRITE_0x44;      
   };
   REG8  RESERVED_18[3];
   
   union {                          //offset 0x48   
      REG8 EBLR;                   //oper mode
      REG8 RESERVED_CONFIG_0x48;      
   };
   REG8  RESERVED_19[7];
   
   union {                          //offset 0x50   
      REG8 MVR;                    //read only
      REG8 RESERVED_WRITE_0x50;      
   };
   REG8  RESERVED_20[3];
   
   REG8 SYSC;                      //offset 0x54
   REG8  RESERVED_21[3];
   
   union {                          //offset 0x58   
      REG8 SYSS;
      REG8 RESERVED_WRITE_0x58;      
   };   
   REG8  RESERVED_22[3];
   
   REG8 WER;                       //offset 0x5C
   REG8 RESERVED_23[3];
   
   REG8 CFPS;                      //offset 0x60
   
} OMAP_UART_REGS;
#pragma warning(pop)

#define UART_LCR_MODE_CONFIG_A            0x80
#define UART_LCR_MODE_CONFIG_B            0xBF
#define UART_LCR_MODE_OPERATIONAL         0x00

#define UART_LCR_DLAB                     (1 << 7)
#define UART_LCR_DIV_EN                   (1 << 7)
#define UART_LCR_BREAK_EN                 (1 << 6)
#define UART_LCR_PARITY_TYPE_2            (1 << 5)
#define UART_LCR_PARITY_TYPE_1            (1 << 4)
#define UART_LCR_PARITY_FORCE_0           (3 << 4)
#define UART_LCR_PARITY_FORCE_1           (2 << 4)
#define UART_LCR_PARITY_EVEN              (1 << 4)
#define UART_LCR_PARITY_ODD               (0 << 4)
#define UART_LCR_PARITY_EN                (1 << 3)
#define UART_LCR_NB_STOP                  (1 << 2)
#define UART_LCR_CHAR_LENGTH_8BIT         (3 << 0)
#define UART_LCR_CHAR_LENGTH_7BIT         (2 << 0)
#define UART_LCR_CHAR_LENGTH_6BIT         (1 << 0)
#define UART_LCR_CHAR_LENGTH_5BIT         (0 << 0)

#define UART_LSR_RX_ERROR                 0x1E

#define UART_LSR_RX_FIFO_STS              (1 << 7)
#define UART_LSR_TX_SR_E                  (1 << 6)
#define UART_LSR_TX_FIFO_E                (1 << 5)
#define UART_LSR_RX_BI                    (1 << 4)
#define UART_LSR_RX_FE                    (1 << 3)
#define UART_LSR_RX_PE                    (1 << 2)
#define UART_LSR_RX_OE                    (1 << 1)
#define UART_LSR_RX_FIFO_E                (1 << 0)

#define UART_LSR_IRDA_THR_EMPTY           (1 << 7)
#define UART_LSR_IRDA_STS_FIFO_FULL       (1 << 6)
#define UART_LSR_IRDA_RX_LAST_BYTE        (1 << 5)
#define UART_LSR_IRDA_FRAME_TOO_LONG      (1 << 4)
#define UART_LSR_IRDA_ABORT               (1 << 3)
#define UART_LSR_IRDA_CRC                 (1 << 2)
#define UART_LSR_IRDA_STS_FIFO_E          (1 << 1)
#define UART_LSR_IRDA_RX_FIFO_E           (1 << 0)

#define UART_MCR_TCR_TLR                  (1 << 6)
#define UART_MCR_XON_EN                   (1 << 5)
#define UART_MCR_LOOPBACK_EN              (1 << 4)
#define UART_MCR_CD_STS_CH                (1 << 3)
#define UART_MCR_RI_STS_CH                (1 << 2)
#define UART_MCR_RTS                      (1 << 1)
#define UART_MCR_DTR                      (1 << 0)

#define UART_TCR_RX_FIFO_TRIG_START_0     (0 << 4)
#define UART_TCR_RX_FIFO_TRIG_START_4     (1 << 4)
#define UART_TCR_RX_FIFO_TRIG_START_8     (2 << 4)
#define UART_TCR_RX_FIFO_TRIG_START_12    (3 << 4)
#define UART_TCR_RX_FIFO_TRIG_START_16    (4 << 4)
#define UART_TCR_RX_FIFO_TRIG_START_20    (5 << 4)
#define UART_TCR_RX_FIFO_TRIG_START_24    (6 << 4)
#define UART_TCR_RX_FIFO_TRIG_START_28    (7 << 4)
#define UART_TCR_RX_FIFO_TRIG_START_32    (8 << 4)
#define UART_TCR_RX_FIFO_TRIG_START_36    (9 << 4)
#define UART_TCR_RX_FIFO_TRIG_START_40   (10 << 4)
#define UART_TCR_RX_FIFO_TRIG_START_44   (11 << 4)
#define UART_TCR_RX_FIFO_TRIG_START_48   (12 << 4)
#define UART_TCR_RX_FIFO_TRIG_START_52   (13 << 4)
#define UART_TCR_RX_FIFO_TRIG_START_56   (14 << 4)
#define UART_TCR_RX_FIFO_TRIG_START_60   (15 << 4)
#define UART_TCR_RX_FIFO_TRIG_HALT_0      (0 << 0)
#define UART_TCR_RX_FIFO_TRIG_HALT_4      (1 << 0)
#define UART_TCR_RX_FIFO_TRIG_HALT_8      (2 << 0)
#define UART_TCR_RX_FIFO_TRIG_HALT_12     (3 << 0)
#define UART_TCR_RX_FIFO_TRIG_HALT_16     (4 << 0)
#define UART_TCR_RX_FIFO_TRIG_HALT_20     (5 << 0)
#define UART_TCR_RX_FIFO_TRIG_HALT_24     (6 << 0)
#define UART_TCR_RX_FIFO_TRIG_HALT_28     (7 << 0)
#define UART_TCR_RX_FIFO_TRIG_HALT_32     (8 << 0)
#define UART_TCR_RX_FIFO_TRIG_HALT_36     (9 << 0)
#define UART_TCR_RX_FIFO_TRIG_HALT_40    (10 << 0)
#define UART_TCR_RX_FIFO_TRIG_HALT_44    (11 << 0)
#define UART_TCR_RX_FIFO_TRIG_HALT_48    (12 << 0)
#define UART_TCR_RX_FIFO_TRIG_HALT_52    (13 << 0)
#define UART_TCR_RX_FIFO_TRIG_HALT_56    (14 << 0)
#define UART_TCR_RX_FIFO_TRIG_HALT_60    (15 << 0)

#define UART_TLR_RX_FIFO_TRIG_DMA_0       (0 << 4)
#define UART_TLR_RX_FIFO_TRIG_DMA_4       (1 << 4)
#define UART_TLR_RX_FIFO_TRIG_DMA_8       (2 << 4)
#define UART_TLR_RX_FIFO_TRIG_DMA_12      (3 << 4)
#define UART_TLR_RX_FIFO_TRIG_DMA_16      (4 << 4)
#define UART_TLR_RX_FIFO_TRIG_DMA_20      (5 << 4)
#define UART_TLR_RX_FIFO_TRIG_DMA_24      (6 << 4)
#define UART_TLR_RX_FIFO_TRIG_DMA_28      (7 << 4)
#define UART_TLR_RX_FIFO_TRIG_DMA_32      (8 << 4)
#define UART_TLR_RX_FIFO_TRIG_DMA_36      (9 << 4)
#define UART_TLR_RX_FIFO_TRIG_DMA_40     (10 << 4)
#define UART_TLR_RX_FIFO_TRIG_DMA_44     (11 << 4)
#define UART_TLR_RX_FIFO_TRIG_DMA_48     (12 << 4)
#define UART_TLR_RX_FIFO_TRIG_DMA_52     (13 << 4)
#define UART_TLR_RX_FIFO_TRIG_DMA_56     (14 << 4)
#define UART_TLR_RX_FIFO_TRIG_DMA_60     (15 << 4)
#define UART_TLR_TX_FIFO_TRIG_DMA_0       (0 << 0)
#define UART_TLR_TX_FIFO_TRIG_DMA_4       (1 << 0)
#define UART_TLR_TX_FIFO_TRIG_DMA_8       (2 << 0)
#define UART_TLR_TX_FIFO_TRIG_DMA_12      (3 << 0)
#define UART_TLR_TX_FIFO_TRIG_DMA_16      (4 << 0)
#define UART_TLR_TX_FIFO_TRIG_DMA_20      (5 << 0)
#define UART_TLR_TX_FIFO_TRIG_DMA_24      (6 << 0)
#define UART_TLR_TX_FIFO_TRIG_DMA_28      (7 << 0)
#define UART_TLR_TX_FIFO_TRIG_DMA_32      (8 << 0)
#define UART_TLR_TX_FIFO_TRIG_DMA_36      (9 << 0)
#define UART_TLR_TX_FIFO_TRIG_DMA_40     (10 << 0)
#define UART_TLR_TX_FIFO_TRIG_DMA_44     (11 << 0)
#define UART_TLR_TX_FIFO_TRIG_DMA_48     (12 << 0)
#define UART_TLR_TX_FIFO_TRIG_DMA_52     (13 << 0)
#define UART_TLR_TX_FIFO_TRIG_DMA_56     (14 << 0)
#define UART_TLR_TX_FIFO_TRIG_DMA_60     (15 << 0)

#define UART_MSR_NCD                      (1 << 7)
#define UART_MSR_NRI                      (1 << 6)
#define UART_MSR_NDSR                     (1 << 5)
#define UART_MSR_NCTS                     (1 << 4)
#define UART_MSR_DCD                      (1 << 3)
#define UART_MSR_RI                       (1 << 2)
#define UART_MSR_DSR                      (1 << 1)
#define UART_MSR_CTS                      (1 << 0)

#define UART_IER_CST                      (1 << 7)
#define UART_IER_RTS                      (1 << 6)
#define UART_IER_XOFF                     (1 << 5)
#define UART_IER_SLEEP_MODE               (1 << 4)
#define UART_IER_MODEM                    (1 << 3)
#define UART_IER_LINE                     (1 << 2)
#define UART_IER_THR                      (1 << 1)
#define UART_IER_RHR                      (1 << 0)

#define UART_IER_IRDA_RHR                 (1 << 0)
#define UART_IER_IRDA_THR                 (1 << 1)
#define UART_IER_IRDA_RX_LAST_BYTE        (1 << 2)
#define UART_IER_IRDA_RX_OE               (1 << 3)
#define UART_IER_IRDA_STS_FIFO            (1 << 4)
#define UART_IER_IRDA_TX_STATUS           (1 << 5)
#define UART_IER_IRDA_LINE_STATUS         (1 << 6)
#define UART_IER_IRDA_EOF                 (1 << 7)

#define UART_IIR_MODEM                    (0 << 1)
#define UART_IIR_THR                      (1 << 1)
#define UART_IIR_RHR                      (2 << 1)
#define UART_IIR_LINE                     (3 << 1)
#define UART_IIR_TO                       (6 << 1)
#define UART_IIR_XOFF                     (8 << 1)
#define UART_IIR_CTSRTS                  (10 << 1)
//#define UART_IIR_HW                      (16 << 1) // NOT DEFINED IN 35xx
#define UART_IIR_IT_PENDING               (1 << 0)

#define UART_IIR_IRDA_RHR                 (1 << 0)
#define UART_IIR_IRDA_THR                 (1 << 1)
#define UART_IIR_IRDA_RX_LAST_BYTE        (1 << 2)
#define UART_IIR_IRDA_RX_OE               (1 << 3)
#define UART_IIR_IRDA_STS_FIFO            (1 << 4)
#define UART_IIR_IRDA_TX_STATUS           (1 << 5)
#define UART_IIR_IRDA_LINE_STATUS         (1 << 6)
#define UART_IIR_IRDA_EOF                 (1 << 7)

#define UART_FCR_RX_FIFO_LSB_1            (1 << 6)
#define UART_FCR_RX_FIFO_LSB_2            (2 << 6)
#define UART_FCR_RX_FIFO_LSB_3            (3 << 6)
#define UART_FCR_TX_FIFO_LSB_1            (1 << 4)
#define UART_FCR_TX_FIFO_LSB_2            (2 << 4)
#define UART_FCR_TX_FIFO_LSB_3            (3 << 4)

#define UART_FCR_RX_FIFO_TRIG_8           (0 << 6)
#define UART_FCR_RX_FIFO_TRIG_16          (1 << 6)
#define UART_FCR_RX_FIFO_TRIG_56          (2 << 6)
#define UART_FCR_RX_FIFO_TRIG_60          (3 << 6)
#define UART_FCR_TX_FIFO_TRIG_8           (0 << 4)
#define UART_FCR_TX_FIFO_TRIG_16          (1 << 4)
#define UART_FCR_TX_FIFO_TRIG_32          (2 << 4)
#define UART_FCR_TX_FIFO_TRIG_56          (3 << 4)
#define UART_FCR_DMA_MODE                 (1 << 3)
#define UART_FCR_TX_FIFO_CLEAR            (1 << 2)
#define UART_FCR_RX_FIFO_CLEAR            (1 << 1)
#define UART_FCR_FIFO_EN                  (1 << 0)

#define UART_SCR_RX_TRIG_GRANU1           (1 << 7)
#define UART_SCR_TX_TRIG_GRANU1           (1 << 6)
#define UART_SCR_DSR_IT                   (1 << 5)
#define UART_SCR_RX_CTS_DSR_WAKE_UP_ENABLE (1 << 4)
#define UART_SCR_TX_EMPTY_CTL             (1 << 3)
#define UART_SCR_DMA_MODE_2_MODE0         (0 << 1)
#define UART_SCR_DMA_MODE_2_MODE1         (1 << 1)
#define UART_SCR_DMA_MODE_2_MODE2         (2 << 1)
#define UART_SCR_DMA_MODE_2_MODE3         (3 << 1)
#define UART_SCR_DMA_MODE_CTL             (1 << 0)

#define UART_SSR_TX_FIFO_FULL             (1 << 0)

#define UART_SYSC_IDLE_FORCE              (0 << 3)
#define UART_SYSC_IDLE_DISABLED           (1 << 3)
#define UART_SYSC_IDLE_SMART              (2 << 3)
#define UART_SYSC_WAKEUP_ENABLE           (1 << 2)
#define UART_SYSC_RST                     (1 << 1)
#define UART_SYSC_AUTOIDLE                (1 << 0)

#define UART_SYSS_RST_DONE                (1 << 0)

#define UART_MDR1_UART16                  (0 << 0)
#define UART_MDR1_SIR                     (1 << 0)
#define UART_MDR1_UART16AUTO              (2 << 0)
#define UART_MDR1_UART13                  (3 << 0)
#define UART_MDR1_MIR                     (4 << 0)
#define UART_MDR1_FIR                     (5 << 0)
#define UART_MDR1_DISABLE                 (7 << 0)

#define UART_MDR1_FRAME_END_MODE          (1 << 7)
#define UART_MDR1_SIP_MODE                (1 << 6)
#define UART_MDR1_SCT                     (1 << 5)
#define UART_MDR1_SET_TXIR                (1 << 4)
#define UART_MDR1_IR_SLEEP                (1 << 3)

#define UART_MDR2_STS_FIFO_TRIG           (0x6)
#define UART_MDR2_STS_FIFO_TRIG_1_ENTRY   (0 << 1)
#define UART_MDR2_STS_FIFO_TRIG_4_ENTRY   (1 << 1)
#define UART_MDR2_STS_FIFO_TRIG_7_ENTRY   (2 << 1)
#define UART_MDR2_STS_FIFO_TRIG_8_ENTRY   (3 << 1)
#define UART_MDR2_TX_UNDERRUN             (1 << 0)

#define UART_BLR_STS_FIFO_RESET           (1 << 7)

// ACREG register bit masks
#define UART_ACR_EOT                      (1 << 0)   // end of transmission bits
#define UART_ACR_ABORT                    (1 << 1)   // frame abort
#define UART_ACR_SCTX                     (1 << 2)   // start frame tx
#define UART_ACR_SEND_SIP                 (1 << 3)   // Triggers SIP when set to 1
#define UART_ACR_DIS_TX_UNDERRUN          (1 << 4)   // Disable TX Underrun
#define UART_ACR_DIS_IR_RX                (1 << 5)   // Disable RXIR input
#define UART_ACR_SD_MOD                   (1 << 6)   // SD_MODE control
#define UART_ACR_PULSE_TYPE               (1 << 7)   // SIR pulse width 3/16 or 1.6us

#define UART_EFR_AUTO_CTS_EN              (1 << 7)
#define UART_EFR_AUTO_RTS_EN              (1 << 6)
#define UART_EFR_SPECIAL_CHAR_DETECT      (1 << 5)
#define UART_EFR_ENHANCED_EN              (1 << 4)
#define UART_EFR_SW_FLOW_CONTROL_TX_NONE        (0 << 2)
#define UART_EFR_SW_FLOW_CONTROL_TX_XONOFF1     (2 << 2)
#define UART_EFR_SW_FLOW_CONTROL_TX_XONOFF2     (1 << 2)
#define UART_EFR_SW_FLOW_CONTROL_TX_XONOFF12    (3 << 2)
#define UART_EFR_SW_FLOW_CONTROL_RX_NONE        (0 << 0)
#define UART_EFR_SW_FLOW_CONTROL_RX_XONOFF1     (2 << 0)
#define UART_EFR_SW_FLOW_CONTROL_RX_XONOFF2     (1 << 0)
#define UART_EFR_SW_FLOW_CONTROL_RX_XONOFF12    (3 << 0)

#define UART_SFLSR_RX_OE                  (1 << 4)
#define UART_SFLSR_FRAME_TOO_LONG         (1 << 3)
#define UART_SFLSR_ABORT                  (1 << 2)
#define UART_SFLSR_CRC                    (1 << 1)
                                                    
#define IRDA_BOF                          (0xC0)    // Don't use 0xFF,
#define IRDA_EXTRA_BOF                    (0xC0)    // it can break HP printers 
#define IRDA_EOF                          (0xC1)
#define IRDA_ESC                          (0x7D)
#define IRDA_ESC_XOR                      (0x20)

typedef enum _IRDA_DECODE_PROCESS_STATE
{
  STATE_READY = 0,
  STATE_BOF,
  STATE_IN_ESC,
  STATE_RX,
  STATE_SHUTDOWN,
  STATE_COMPLETE
}IRDA_DECODE_PROCESS_STATE;

//------------------------------------------------------------------------------

#endif // __OMAP_UART_REGS_H
