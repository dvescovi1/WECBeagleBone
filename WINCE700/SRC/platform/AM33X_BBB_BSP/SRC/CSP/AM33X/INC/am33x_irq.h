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
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//
/*
================================================================================
* (c) Copyright Texas Instruments, Incorporated. All Rights Reserved.
*
* Use of this software is controlled by the terms and conditions found
* in the license agreement under which this software has been supplied.
*
================================================================================
*/
//
//  File:  am33x_irq.h
//
//  This file defines names for IRQ. This names have no other role than make
//  code more readable. For SoC where device - IRQ mapping is defined by
//  silicon and it can't be changed by software.
//
#ifndef __AM33X_IRQ_H
#define __AM33X_IRQ_H


//------------------------------------------------------------------------------
//
//  Global: Flag for fake OEMIdle busy loop

extern volatile BOOL fInterruptFlag;


//------------------------------------------------------------------------------

#define AM33X_IRQ_MAXIMUM                256
#define AM33X_IRQ_PER_SYSINTR            4
#define AM33X_GPIO_BANK_COUNT            4

//------------------------------------------------------------------------------
//
//  Physical IRQs on the AM389X
//

#define IRQ_EMUINT                      0   //
#define IRQ_COMMTX                      1   //
#define IRQ_COMMRX                      2   //
#define IRQ_BENCH                       3   //
#define IRQ_ELM                         4   //
#define IRQ_SSM_WFI                     5   //
#define IRQ_SSM                         6   //
#define IRQ_NMI                         7   // External source 
#define IRQ_SEC_EVNT                    8   // 
#define IRQ_L3DEBUG                     9   //
#define IRQ_L3APPINT                    10  // 
#define IRQ_PRCMINT                     11  
#define IRQ_EDMACOMPINIT                12  //
#define IRQ_EDMAMPERR                   13  //
#define IRQ_EDMAERRINT                  14  //
#define IRQ_WDTINT0                     15  //
#define IRQ_ADC_TSC_GENINT              16  //
#define IRQ_USBSSINT                    17  //
#define IRQ_USBINT0                     18  //
#define IRQ_USBINT1                     19  //
#define IRQ_PRUSS1_EVTOUT0              20  //
#define IRQ_PRUSS1_EVTOUT1              21  //
#define IRQ_PRUSS1_EVTOUT2              22  //
#define IRQ_PRUSS1_EVTOUT3              23  //
#define IRQ_PRUSS1_EVTOUT4              24  //
#define IRQ_PRUSS1_EVTOUT5              25  //
#define IRQ_PRUSS1_EVTOUT6              26  //
#define IRQ_PRUSS1_EVTOUT7              27  //
#define IRQ_SDINT1			            28  //
#define IRQ_SDINT2			            29  //
#define IRQ_I2CINT2                     30  //
#define IRQ_ECAP0                       31  //
#define IRQ_GPIO2A                      32  //
#define IRQ_GPIO2B                      33  //
#define IRQ_USBWAKEUP                   34  //
#define IRQ_PCIWAKEUP                   35  //
#define IRQ_LCDCINT                     36  //
#define IRQ_GFXINT                      37  //
#define IRQ_2DHWA                       38  //
#define IRQ_EPWM2           			39
#define IRQ_3PGSWRXTHR0                 40  //
#define IRQ_3PGSWRXINT0                 41  //
#define IRQ_3PGSWTXINT0                 42  //
#define IRQ_3PGSWMISC0                  43  //
#define IRQ_UART3INT                    44  //
#define IRQ_UART4INT                    45  //
#define IRQ_UART5INT                    46  //
#define IRQ_ECAP1                       47
#define IRQ_PCIINT0                     48  //
#define IRQ_PCIINT1                     49  //
#define IRQ_PCIINT2                     50  //
#define IRQ_PCIINT3                     51  //
#define IRQ_DCAN0_INT0                  52  //
#define IRQ_DCAN0_INT1                  53  //
#define IRQ_DCAN0_PARITY                54  //
#define IRQ_DCAN1_INT0                  55  //
#define IRQ_DCAN1_INT1                  56  //
#define IRQ_DCAN1_PARITY                57  //
#define IRQ_EPWM0_TZ                    58
#define IRQ_EPWM1_TZ                    59
#define IRQ_EPWM2_TZ                    60
#define IRQ_ECAP2                       61
#define IRQ_GPIO3A                      62  //
#define IRQ_GPIO3B                      63  //
#define IRQ_SDINT0                      64  //
#define IRQ_SPI0INT                     65  //
#define IRQ_TIMER0                      66  //
#define IRQ_TIMER1MS                    67  //
#define IRQ_TIMER2                      68  //
#define IRQ_TIMER3                      69  //
#define IRQ_I2CINT0                     70  //
#define IRQ_I2CINT1                     71  //
#define IRQ_UART0INT                    72  //
#define IRQ_UART1INT                    73  //
#define IRQ_UART2INT                    74  //
#define IRQ_RTCINT                      75  //
#define IRQ_RTCALARM                    76  //
#define IRQ_MBINT                       77  // Mailbox
#define IRQ_M3_TXEV                     78  //
#define IRQ_EQEP0                       79  //
#define IRQ_MCA0TX                      80  //
#define IRQ_MCA0RX                      81  //
#define IRQ_MCA1TX                      82  //
#define IRQ_MCA1RX                      83  //
#define IRQ_MCA2TX                      84  //
#define IRQ_MCA2RX                      85  //
#define IRQ_EPWM0                       86  //
#define IRQ_EPWM1                       87  //
#define IRQ_EQEP1                       88  //
#define IRQ_EQEP2                       89  //
#define IRQ_DMA_INT_PIN2                90
#define IRQ_WDT1                        91  //
#define IRQ_TIMER4                      92  //
#define IRQ_TIMER5                      93  //
#define IRQ_TIMER6                      94  //
#define IRQ_TIMER7                      95  // 
#define IRQ_GPIO0A                      96  //
#define IRQ_GPIO0B                      97  //
#define IRQ_GPIO1A                      98  //
#define IRQ_GPIO1B                      99  //
#define IRQ_GPMC                       100  // 
#define IRQ_DDRERR0                    101  //
#define IRQ_AES0_S                     102  //
#define IRQ_AES0_P                     103  //
#define IRQ_AES1_S                     104  //
#define IRQ_AES1_P                     105  //
#define IRQ_DES_S                      106  //
#define IRQ_DES_P                      107  // 
#define IRQ_SHA_S                      108  //
#define IRQ_SHA_P                      109  //
#define IRQ_FPKA_SINTREQUEST           110  //
#define IRQ_RNG                        111  //
#define IRQ_TCERR0                     112  //
#define IRQ_TCERR1                     113  //
#define IRQ_TCERR2                     114  //
#define IRQ_ADC_TSC_PEND               115  //
#define IRQ_CDMA0                      116
#define IRQ_CDMA1                      117
#define IRQ_CDMA2                      118
#define IRQ_CDMA3                      119
#define IRQ_SMRFLX_ARM                 120  //
#define IRQ_SMRFLX_CORE                121  //
#define IRQ_SYSMMU                     122  //
#define IRQ_DMA_INT_PIN0               123  //
#define IRQ_DMA_INT_PIN1               124  //
#define IRQ_SPI1INT                    125  //
#define IRQ_SPI2INT                    126  //
#define IRQ_SPI3INT                    127  //
#define MAX_IRQ_COUNT                  128  // Total number of IRQs 


//------------------------------------------------------------------------------
//
//  Software IRQs for kernel/driver interaction
//

#define IRQ_SW_RTC_QUERY                256         // Query to OAL RTC
#define IRQ_SW_RESERVED_1               257
#define IRQ_SW_RESERVED_2               258
#define IRQ_SW_RESERVED_MAX             259


//------------------------------------------------------------------------------
//
//  GPIO virtual IRQs for flattening out the 2 level GPIO interrupts
//

//GPIO Bank 1
#define IRQ_GPIO_0                      128         // GPIO1 bit 0
#define IRQ_GPIO_1                      129         // GPIO1 bit 1
#define IRQ_GPIO_2                      130         // GPIO1 bit 2
#define IRQ_GPIO_3                      131         // GPIO1 bit 3
#define IRQ_GPIO_4                      132         // GPIO1 bit 4
#define IRQ_GPIO_5                      133         // GPIO1 bit 5
#define IRQ_GPIO_6                      134         // GPIO1 bit 6
#define IRQ_GPIO_7                      135         // GPIO1 bit 7
#define IRQ_GPIO_8                      136         // GPIO1 bit 8
#define IRQ_GPIO_9                      137         // GPIO1 bit 9
#define IRQ_GPIO_10                     138         // GPIO1 bit 10
#define IRQ_GPIO_11                     139         // GPIO1 bit 11
#define IRQ_GPIO_12                     140         // GPIO1 bit 12
#define IRQ_GPIO_13                     141         // GPIO1 bit 13
#define IRQ_GPIO_14                     142         // GPIO1 bit 14
#define IRQ_GPIO_15                     143         // GPIO1 bit 15
#define IRQ_GPIO_16                     144         // GPIO1 bit 16
#define IRQ_GPIO_17                     145         // GPIO1 bit 17
#define IRQ_GPIO_18                     146         // GPIO1 bit 18
#define IRQ_GPIO_19                     147         // GPIO1 bit 19
#define IRQ_GPIO_20                     148         // GPIO1 bit 20
#define IRQ_GPIO_21                     149         // GPIO1 bit 21
#define IRQ_GPIO_22                     150         // GPIO1 bit 22
#define IRQ_GPIO_23                     151         // GPIO1 bit 23
#define IRQ_GPIO_24                     152         // GPIO1 bit 24
#define IRQ_GPIO_25                     153         // GPIO1 bit 25
#define IRQ_GPIO_26                     154         // GPIO1 bit 26
#define IRQ_GPIO_27                     155         // GPIO1 bit 27
#define IRQ_GPIO_28                     156         // GPIO1 bit 28
#define IRQ_GPIO_29                     157         // GPIO1 bit 29
#define IRQ_GPIO_30                     158         // GPIO1 bit 30
#define IRQ_GPIO_31                     159         // GPIO1 bit 31

//GPIO Bank 2
#define IRQ_GPIO_32                     160         // GPIO2 bit 0
#define IRQ_GPIO_33                     161         // GPIO2 bit 1
#define IRQ_GPIO_34                     162         // GPIO2 bit 2
#define IRQ_GPIO_35                     163         // GPIO2 bit 3
#define IRQ_GPIO_36                     164         // GPIO2 bit 4
#define IRQ_GPIO_37                     165         // GPIO2 bit 5
#define IRQ_GPIO_38                     166         // GPIO2 bit 6
#define IRQ_GPIO_39                     167         // GPIO2 bit 7
#define IRQ_GPIO_40                     168         // GPIO2 bit 8
#define IRQ_GPIO_41                     169         // GPIO2 bit 9
#define IRQ_GPIO_42                     170         // GPIO2 bit 10
#define IRQ_GPIO_43                     171         // GPIO2 bit 11
#define IRQ_GPIO_44                     172         // GPIO2 bit 12
#define IRQ_GPIO_45                     173         // GPIO2 bit 13
#define IRQ_GPIO_46                     174         // GPIO2 bit 14
#define IRQ_GPIO_47                     175         // GPIO2 bit 15
#define IRQ_GPIO_48                     176         // GPIO2 bit 16
#define IRQ_GPIO_49                     177         // GPIO2 bit 17
#define IRQ_GPIO_50                     178         // GPIO2 bit 18
#define IRQ_GPIO_51                     179         // GPIO2 bit 19
#define IRQ_GPIO_52                     180         // GPIO2 bit 20
#define IRQ_GPIO_53                     181         // GPIO2 bit 21
#define IRQ_GPIO_54                     182         // GPIO2 bit 22
#define IRQ_GPIO_55                     183         // GPIO2 bit 23
#define IRQ_GPIO_56                     184         // GPIO2 bit 24
#define IRQ_GPIO_57                     185         // GPIO2 bit 25
#define IRQ_GPIO_58                     186         // GPIO2 bit 26
#define IRQ_GPIO_59                     187         // GPIO2 bit 27
#define IRQ_GPIO_60                     188         // GPIO2 bit 28
#define IRQ_GPIO_61                     189         // GPIO2 bit 29
#define IRQ_GPIO_62                     190         // GPIO2 bit 30
#define IRQ_GPIO_63                     191         // GPIO2 bit 31

//GPIO Bank 3
#define IRQ_GPIO_64                     192         // GPIO2 bit 0
#define IRQ_GPIO_65                     193         // GPIO2 bit 1
#define IRQ_GPIO_66                     194         // GPIO2 bit 2
#define IRQ_GPIO_67                     195         // GPIO2 bit 3
#define IRQ_GPIO_68                     196         // GPIO2 bit 4
#define IRQ_GPIO_69                     197         // GPIO2 bit 5
#define IRQ_GPIO_70                     198         // GPIO2 bit 6
#define IRQ_GPIO_71                     199         // GPIO2 bit 7
#define IRQ_GPIO_72                     200         // GPIO2 bit 8
#define IRQ_GPIO_73                     201         // GPIO2 bit 9
#define IRQ_GPIO_74                     202         // GPIO2 bit 10
#define IRQ_GPIO_75                     203         // GPIO2 bit 11
#define IRQ_GPIO_76                     204         // GPIO2 bit 12
#define IRQ_GPIO_77                     205         // GPIO2 bit 13
#define IRQ_GPIO_78                     206         // GPIO2 bit 14
#define IRQ_GPIO_79                     207         // GPIO2 bit 15
#define IRQ_GPIO_80                     208         // GPIO2 bit 16
#define IRQ_GPIO_81                     209         // GPIO2 bit 17
#define IRQ_GPIO_82                     210         // GPIO2 bit 18
#define IRQ_GPIO_83                     211         // GPIO2 bit 19
#define IRQ_GPIO_84                     212         // GPIO2 bit 20
#define IRQ_GPIO_85                     213         // GPIO2 bit 21
#define IRQ_GPIO_86                     214         // GPIO2 bit 22
#define IRQ_GPIO_87                     215         // GPIO2 bit 23
#define IRQ_GPIO_88                     216         // GPIO2 bit 24
#define IRQ_GPIO_89                     217         // GPIO2 bit 25
#define IRQ_GPIO_90                     218         // GPIO2 bit 26
#define IRQ_GPIO_91                     219         // GPIO2 bit 27
#define IRQ_GPIO_92                     220         // GPIO2 bit 28
#define IRQ_GPIO_93                     221         // GPIO2 bit 29
#define IRQ_GPIO_94                     222         // GPIO2 bit 30
#define IRQ_GPIO_95                     223         // GPIO2 bit 31

//GPIO Bank 4
#define IRQ_GPIO_96                     224         // GPIO4 bit 0
#define IRQ_GPIO_97                     225         // GPIO4 bit 1
#define IRQ_GPIO_98                     226         // GPIO4 bit 2
#define IRQ_GPIO_99                     227         // GPIO4 bit 3
#define IRQ_GPIO_100                    228         // GPIO4 bit 4
#define IRQ_GPIO_101                    229         // GPIO4 bit 5
#define IRQ_GPIO_102                    230         // GPIO4 bit 6
#define IRQ_GPIO_103                    231         // GPIO4 bit 7
#define IRQ_GPIO_104                    232         // GPIO4 bit 8
#define IRQ_GPIO_105                    233         // GPIO4 bit 9
#define IRQ_GPIO_106                    234         // GPIO4 bit 10
#define IRQ_GPIO_107                    235         // GPIO4 bit 11
#define IRQ_GPIO_108                    236         // GPIO4 bit 12
#define IRQ_GPIO_109                    237         // GPIO4 bit 13
#define IRQ_GPIO_110                    238         // GPIO4 bit 14
#define IRQ_GPIO_111                    239         // GPIO4 bit 15
#define IRQ_GPIO_112                    240         // GPIO4 bit 16
#define IRQ_GPIO_113                    241         // GPIO4 bit 17
#define IRQ_GPIO_114                    242         // GPIO4 bit 18
#define IRQ_GPIO_115                    243         // GPIO4 bit 19
#define IRQ_GPIO_116                    244         // GPIO4 bit 20
#define IRQ_GPIO_117                    245         // GPIO4 bit 21
#define IRQ_GPIO_118                    246         // GPIO4 bit 22
#define IRQ_GPIO_119                    247         // GPIO4 bit 23
#define IRQ_GPIO_120                    248         // GPIO4 bit 24
#define IRQ_GPIO_121                    249         // GPIO4 bit 25
#define IRQ_GPIO_122                    250         // GPIO4 bit 26
#define IRQ_GPIO_123                    251         // GPIO4 bit 27
#define IRQ_GPIO_124                    252         // GPIO4 bit 28
#define IRQ_GPIO_125                    253         // GPIO4 bit 29
#define IRQ_GPIO_126                    254         // GPIO4 bit 30
#define IRQ_GPIO_127                    255         // GPIO4 bit 31
                                        
//------------------------------------------------------------------------------


#endif // __AM33X_IRQ_H

