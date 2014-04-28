;
; Copyright (c) Microsoft Corporation.  All rights reserved.
;
;
; Use of this sample source code is subject to the terms of the Microsoft
; license agreement under which you licensed this sample source code. If
; you did not accept the terms of the license agreement, you are not
; authorized to use this sample source code. For the terms of the license,
; please see the license agreement between you and Microsoft or, if applicable,
; see the LICENSE.RTF on your install media or the root of your tools installation.
; THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
;
; Copyright (c) 2007, 2008 BSQUARE Corporation. All rights reserved.

;
;==============================================================================
;             Texas Instruments OMAP(TM) Platform Software
; (c) Copyright Texas Instruments, Incorporated. All Rights Reserved.
;
; Use of this software is controlled by the terms and conditions found
; in the license agreement under which this software has been supplied.
;
;==============================================================================
;
;------------------------------------------------------------------------------
;
;  File:  memory_cfg.inc
;
;  This file is used to define g_oalAddressTable. This table is passed to
;  KernelStart to estabilish physical to virtual memory mapping. This table
;  is used also in memory OAL module to map between physical and virtual
;  memory addresses via OALPAtoVA/OALVAtoPA functions.
;
;  The config.bib file defines image memory layout ant it contains virtual
;  cached memory addresses which must be synchronized with g_oalAddressTable.
;  With each table change make sure that those constant are still valid.
;
;------------------------------------------------------------------------------
;  Export Definition
        EXPORT  g_oalAddressTable[DATA]
        EXPORT  g_oalEbootAddressTable[DATA]
;
; !!! IF YOU MODIFY THE TABLE YOU MUST MODIFY 
; !!! "THE MEMORY_REGISTER_ENTRY _pRegEntries" in the
; !!! \platform\common\src\soc\COMMON_TI_V1\AM33X\CEDDK\MAP\map.c
; 
;----------------------- CE7 Device Table ------------------------------------------------ 
g_oalCE7DeviceTable
        DCD     0x8F000000, 0x8F0000,  0x1000000,  0x00C00      ; 16MB uncached memory for device buffers 
        DCD     0x90000000, 0x480000,  0x0400000,  0x00C00      ; L4S
        DCD     0x90400000, 0x4A0000,  0x0400000,  0x00C00      ; L4F
        DCD     0x90800000, 0x44C000,  0x0400000,  0x00C00      ; L4WKUP
        DCD     0x90C00000, 0x47C000,  0x0400000,  0x00C00      ; L4FW
        DCD     0x91000000, 0x403000,  0x0100000,  0x00C00      ; OCMC RAM
        DCD     0x91100000, 0x080000,  0x1000000,  0x00C00      ; NAND Flash SC0  
        DCD     0x92100000, 0x490000,  0x0100000,  0x00C00      ; TPCC  
        DCD     0x92200000, 0x498000,  0x0300000,  0x00C00      ; TPTC
        DCD     0x92500000, 0x460000,  0x0800000,  0x00C00      ; McASP  
        DCD     0x92D00000, 0x474000,  0x0400000,  0x00C00      ; USB/CPPI
        DCD     0x93100000, 0x478000,  0x0100000,  0x00C00      ; MMCHS2
        DCD     0x93200000, 0x530000,  0x0100000,  0x00C00      ; SHA
        DCD     0x93300000, 0x534000,  0x0100000,  0x00C00      ; AES0
        DCD     0x93400000, 0x54C000,  0x0400000,  0x00C00      ; ADC_TSC
        DCD     0x93800000, 0x560000,  0x1000000,  0x00C00      ; SGX530
        DCD     0x94800000, 0x402000,  0x0100000,  0x00C00      ; SRAM
        DCD     0x94900000, 0x4C0000,  0x0100000,  0x00C00      ; EMIF0
        
        
;========================================================================================
; SDRAM memory allocation for DSS and etc. 
; If you modify that memory allocation, update OEMRamTable accordingly !!!
;----------------------------------------------------------------------------------------              
;
; Total                    
;--------------------------------------------------------------------------------------
;       DCD     0x94800000, NEXT    , 0x1000000, 0x00C00    ; Placeholder 
        DCD     0x00000000, 0x000000, 0x0000000, 0x00000    ; end of table


;======================================================================================
;  Table format: cached address, physical address, size

g_oalAddressTable
        DCD     CE_NEW_MAPPING_TABLE, g_oalCE7DeviceTable,0  ; Flag to use new CE7 mapping scheme
        DCD     0x80000000, 0x80000000, 240        ; SDRAM
        DCD     0x00000000, 0x00000000,   0        ; end of table
 
g_oalEbootAddressTable
        DCD     0x80000000, 0x80000000,256; 0x10000000 SDRAM
        DCD     0x90000000, 0x48000000,  4; L4S
        DCD     0x90400000, 0x4A000000,  4; L4F
        DCD     0x90800000, 0x44C00000,  4; L4WKUP
        DCD     0x90C00000, 0x47C00000,  4; L4FW
        DCD     0x91000000, 0x40300000,  1; OCMC RAM
        DCD     0x91100000, 0x08000000, 16; NAND Flash SC0  
        DCD     0x92100000, 0x49000000,  1; TPCC  
        DCD     0x92200000, 0x49800000,  3; TPTC
        DCD     0x92500000, 0x46000000,  8; McASP  
        DCD     0x92D00000, 0x47400000,  4; USB/CPPI
        DCD     0x93100000, 0x47800000,  1; MMCHS2
       
        DCD     0x93200000, 0x53000000,  1; SHA
        DCD     0x93300000, 0x53400000,  1; AES0
        DCD     0x93400000, 0x54C00000,  4; ADC_TSC
        DCD     0x93800000, 0x56000000, 16; SGX530
        DCD     0x94800000, 0x40200000,  1; SRAM
        DCD     0x94900000, 0x4C000000,  1; EMIF0
        DCD     0x94A00000, 0x50000000,  1; GPMC
;----------------------------------------------------------------------------------------              
        DCD     0x00000000, 0x00000000,  0              ; end of table

;------------------------------------------------------------------------------

        END
