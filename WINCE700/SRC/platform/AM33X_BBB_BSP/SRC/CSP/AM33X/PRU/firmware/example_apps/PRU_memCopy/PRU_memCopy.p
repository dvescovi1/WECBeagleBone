//******************************************************************************
//+--------------------------------------------------------------------------+**
//|                            ****                                          |**
//|                            ****                                          |**
//|                            ******o***                                    |**
//|                      ********_///_****                                   |**
//|                      ***** /_//_/ ****                                   |**
//|                       ** ** (__/ ****                                    |**
//|                           *********                                      |**
//|                            ****                                          |**
//|                            ***                                           |**
//|                                                                          |**
//|         Copyright (c) 1998-2010 Texas Instruments Incorporated           |**
//|                        ALL RIGHTS RESERVED                               |**
//|                                                                          |**
//| Permission is hereby granted to licensees of Texas Instruments           |**
//| Incorporated (TI) products to use this computer program for the sole     |**
//| purpose of implementing a licensee product based on TI products.         |**
//| No other rights to reproduce, use, or disseminate this computer          |**
//| program, whether in part or in whole, are granted.                       |**
//|                                                                          |**
//| TI makes no representation or warranties with respect to the             |**
//| performance of this computer program, AND specifically disclaims         |**
//| any responsibility for any damages, special or consequential,            |**
//| connected with the use of this program.                                  |**
//|                                                                          |**
//+--------------------------------------------------------------------------+**
//*****************************************************************************/
// file:   PRU_memCopy.p 
//
// brief:  PRU Example to optimized memory to memory copy.
//
//
//  (C) Copyright 2010, Texas Instruments, Inc
//
//  author     M. Watkins
//
//  version    0.1     Created

//=============================================================================
//  Case 1: Source AND destination addresses are aligned. 
//  Case 2: Source AND destination addresses are not aligned, offsets of source
//          AND destination addresses from NEXT aligned addresses are same. 
//  Case 3: Source AND destination addresses are not aligned, AND offsets of 
//          source AND destination addresses from NEXT aligned addresses 
//          are not same. 
//=============================================================================

.origin 0
.entrypoint MEM_COPY

#include "PRU_memCopy.hp"

MEM_COPY:
    LDI paramMemLocation.w0, 0x0000
    LDI paramMemLocation.w2, 0x0000
    LBBO global.size, paramMemLocation, 0, 4 // size
    LBBO global.src, paramMemLocation, 4, 4 // source addr
    LBBO global.dst, paramMemLocation, 8, 4 // destination addr

    AND offset.b1, global.src, 0x03 // # source offset  
    AND offset.b2, global.dst, 0x03 // # destination offset  
    QBEQ NEXT, offset.b1, offset.b2 // memory addr not aligned but offsets in source AND destination same

//--------------------------------------
//case 3: 
LOOP1: 
    LBBO temp.b0, global.src, 0, 1 // single byte transfers
    SBBO temp.b0, global.dst, 0, 1
    ADD global.src,global.src, 1         // global.src points to NEXT location
    ADD global.dst,global.dst, 1         // global.dst points to NEXT location
    SUB global.size, global.size, 1        // size count reduces by 1
    QBNE LOOP1, global.size, 0
    JMP DONE
  
NEXT: 
    QBEQ  CASE1, offset.b2, 0 // source offset = destination offset = 0 (both addr aligned)

//---------------------------------------------------        
//case2: 

// single byte transfers to adjust offset
LOOP2: 
    LBBO temp.b0, global.src, 0, 1
    SBBO temp.b0, global.dst, 0, 1
    ADD global.src,global.src, 1         // global.src points to NEXT location
    ADD global.dst,global.dst, 1         // global.dst points to NEXT location
    SUB offset.b2, offset.b2, 1
    SUB global.size, global.size, 1
    QBNE LOOP2, offset.b2, 0
    QBNE LOOP3, global.size, 0
    JMP DONE
 
// 4 byte transfers
LOOP3:     
    QBGT LOOP4, global.size, 4
    LBBO temp, global.src, 0, 4
    SBBO temp, global.dst, 0, 4
    ADD global.src,global.src, 4         // global.src points to NEXT location
    ADD global.dst,global.dst, 4         // global.dst points to NEXT location
    SUB global.size, global.size, 4        // size count reduces by 1       
    QBNE LOOP3, global.size, 0
    JMP DONE
 
// single byte transfers
LOOP4: 
    LBBO temp.b0, global.src, 0, 1
    SBBO temp.b0, global.dst, 0, 1
    ADD global.src,global.src, 1         // global.src points to NEXT location
    ADD global.dst,global.dst, 1         // global.dst points to NEXT location
    SUB global.size, global.size, 1
    QBNE LOOP4, global.size, 0
    JMP DONE 

//----------------------------------------
CASE1: 
    AND singleByteTransfers, global.size, 0x03 // # of single byte transfers  
    SUB global.size, global.size, singleByteTransfers

// 4 byte transfers
LOOP5:     
    LBBO temp, global.src, 0, 4
    SBBO temp, global.dst, 0, 4
    ADD global.src,global.src, 4         // global.src points to NEXT location
    ADD global.dst,global.dst, 4         // global.dst points to NEXT location
    SUB global.size, global.size, 4        // size count reduces by 1       
    QBNE LOOP5, global.size, 0
    QBNE LOOP6, singleByteTransfers,0
    JMP DONE

// single byte transfers
LOOP6: 
    LBBO temp.b0, global.src, 0, 1
    SBBO temp.b0, global.dst, 0, 1
    ADD global.src,global.src, 1         // global.src points to NEXT location
    ADD global.dst,global.dst, 1         // global.dst points to NEXT location
    SUB singleByteTransfers, singleByteTransfers, 1
    QBNE LOOP6, singleByteTransfers, 0
    JMP DONE
          
DONE:
    // send notification to Host for program completion
    MOV R31.b0, #PRU0_ARM_INTERRUPT
    HALT
