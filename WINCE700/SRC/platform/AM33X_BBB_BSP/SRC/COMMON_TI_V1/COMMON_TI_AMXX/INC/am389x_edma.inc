;
; Copyright (c) MPC Data Limited 2007. All Rights Reserved.
;
;------------------------------------------------------------------------------
;
;  Header:  am389x_edma.inc
;
;  This header file is comprised of component header files that define
;  the register layout of each System on Chip (SoC) component.
;------------------------------------------------------------------------------

EDMA_ER_OFFSET                  EQU 0x00
EDMA_ERH_OFFSET                 EQU 0x04
EDMA_ECR_OFFSET                 EQU 0x08
EDMA_ECRH_OFFSET                EQU 0x0C
EDMA_ESR_OFFSET                 EQU 0x10
EDMA_ESRH_OFFSET                EQU 0x14
EDMA_CER_OFFSET                 EQU 0x18
EDMA_CERH_OFFSET                EQU 0x1C
EDMA_EER_OFFSET                 EQU 0x20
EDMA_EERH_OFFSET                EQU 0x24
EDMA_EECR_OFFSET                EQU 0x28
EDMA_EECRH_OFFSET               EQU 0x2C
EDMA_EESR_OFFSET                EQU 0x30
EDMA_EESRH_OFFSET               EQU 0x34
EDMA_SER_OFFSET                 EQU 0x38
EDMA_SERH_OFFSET                EQU 0x3C
EDMA_SECR_OFFSET                EQU 0x40
EDMA_SECRH_OFFSET               EQU 0x44
EDMA_IER_OFFSET                 EQU 0x50
EDMA_IERH_OFFSET                EQU 0x54
EDMA_IECR_OFFSET                EQU 0x58
EDMA_IECRH_OFFSET               EQU 0x5C
EDMA_IESR_OFFSET                EQU 0x60
EDMA_IESRH_OFFSET               EQU 0x64
EDMA_IPR_OFFSET                 EQU 0x68
EDMA_IPRH_OFFSET                EQU 0x6C
EDMA_ICR_OFFSET                 EQU 0x70
EDMA_ICRH_OFFSET                EQU 0x74
EDMA_IEVAL_OFFSET               EQU 0x78
EDMA_QER_OFFSET                 EQU 0x80
EDMA_QEER_OFFSET                EQU 0x84
EDMA_QEECR_OFFSET               EQU 0x88
EDMA_QEESR_OFFSET               EQU 0x8C
EDMA_QSER_OFFSET                EQU 0x90
EDMA_QSECR_OFFSET               EQU 0x94

EDMA_SHADOW0_REG                EQU (AM389X_EDMA_CC_REGS_PA + 0x2000)

EDMA_ER_REG                     EQU (EDMA_SHADOW0_REG + 0x00)
EDMA_ERH_REG                    EQU (EDMA_SHADOW0_REG + 0x04)
EDMA_ECR_REG                    EQU (EDMA_SHADOW0_REG + 0x08)
EDMA_ECRH_REG                   EQU (EDMA_SHADOW0_REG + 0x0C)
EDMA_ESR_REG                    EQU (EDMA_SHADOW0_REG + 0x10)
EDMA_ESRH_REG                   EQU (EDMA_SHADOW0_REG + 0x14)
EDMA_CER_REG                    EQU (EDMA_SHADOW0_REG + 0x18)
EDMA_CERH_REG                   EQU (EDMA_SHADOW0_REG + 0x1C)
EDMA_EER_REG                    EQU (EDMA_SHADOW0_REG + 0x20)
EDMA_EERH_REG                   EQU (EDMA_SHADOW0_REG + 0x24)
EDMA_EECR_REG                   EQU (EDMA_SHADOW0_REG + 0x28)
EDMA_EECRH_REG                  EQU (EDMA_SHADOW0_REG + 0x2C)
EDMA_EESR_REG                   EQU (EDMA_SHADOW0_REG + 0x30)
EDMA_EESRH_REG                  EQU (EDMA_SHADOW0_REG + 0x34)
EDMA_SER_REG                    EQU (EDMA_SHADOW0_REG + 0x38)
EDMA_SERH_REG                   EQU (EDMA_SHADOW0_REG + 0x3C)
EDMA_SECR_REG                   EQU (EDMA_SHADOW0_REG + 0x40)
EDMA_SECRH_REG                  EQU (EDMA_SHADOW0_REG + 0x44)
EDMA_IER_REG                    EQU (EDMA_SHADOW0_REG + 0x50)
EDMA_IERH_REG                   EQU (EDMA_SHADOW0_REG + 0x54)
EDMA_IECR_REG                   EQU (EDMA_SHADOW0_REG + 0x58)
EDMA_IECRH_REG                  EQU (EDMA_SHADOW0_REG + 0x5C)
EDMA_IESR_REG                   EQU (EDMA_SHADOW0_REG + 0x60)
EDMA_IESRH_REG                  EQU (EDMA_SHADOW0_REG + 0x64)
EDMA_IPR_REG                    EQU (EDMA_SHADOW0_REG + 0x68)
EDMA_IPRH_REG                   EQU (EDMA_SHADOW0_REG + 0x6C)
EDMA_ICR_REG                    EQU (EDMA_SHADOW0_REG + 0x70)
EDMA_ICRH_REG                   EQU (EDMA_SHADOW0_REG + 0x74)
EDMA_IEVAL_REG                  EQU (EDMA_SHADOW0_REG + 0x78)
EDMA_QER_REG                    EQU (EDMA_SHADOW0_REG + 0x80)
EDMA_QEER_REG                   EQU (EDMA_SHADOW0_REG + 0x84)
EDMA_QEECR_REG                  EQU (EDMA_SHADOW0_REG + 0x88)
EDMA_QEESR_REG                  EQU (EDMA_SHADOW0_REG + 0x8C)
EDMA_QSER_REG                   EQU (EDMA_SHADOW0_REG + 0x90)
EDMA_QSECR_REG                  EQU (EDMA_SHADOW0_REG + 0x94)


    END
