/*
===============================================================================
*             Texas Instruments OMAP(TM) Platform Software
* (c) Copyright Texas Instruments, Incorporated. All Rights Reserved.
*
* Use of this software is controlled by the terms and conditions found
* in the license agreement under which this software has been supplied.
*
===============================================================================
*/
//
//  This file contains OMAP specific CPU version code.
//
#ifndef __OMAP_CPUVER_H
#define __OMAP_CPUVER_H

#if __cplusplus
extern "C" {
#endif

// The following constants are used to distinguish between ES2.0 and ES2.1 CPU 
// revisions.  This mechanism is necessary because the ID fuses are incorrectly
// set to ES2.0 in some ES2.1 devices.
#define PUBLIC_ROM_CRC_PA       0x14020
#define PUBLIC_ROM_CRC_ES2_0    0x5a540331
#define PUBLIC_ROM_CRC_ES2_1    0x6880d8d6

#define CPU_FAMILY_SHIFT     8
#define CHIP_ID_SHIFT            16

#define CPU_REVISION_MASK   0xFF
#define CPU_FAMILY_MASK       0xFF
#define CHIP_ID_MASK             0xFFFF
#define CHIP_FEATURE_MASK  0x7C00

#define CPU_ID(x)                     ((x>>CHIP_ID_SHIFT) & CHIP_ID_MASK)
#define CPU_FAMILY(x)             ((x>>CPU_FAMILY_SHIFT) & CPU_FAMILY_MASK)
#define CPU_REVISION(x)          ((x) & CPU_REVISION_MASK)

/*
 * Define CPU families
 * (CAUTION: Be careful if changing the following definitions since modules
 * such as PowerVR also rely on these)
 */
 typedef enum {
    CPU_FAMILY_OMAP35XX=0,      /* OMAP35xx devices */
    CPU_FAMILY_DM37XX,          /* OMAP37xx devices */
    CPU_FAMILY_AM35XX,          /* AM3517 devices   */
    CPU_FAMILY_AM389X,          /* AM389X devices   */
    CPU_FAMILY_AM387X,          /* AM387X devices   */
    CPU_FAMILY_AM33X,           /* AM33X devices    */
    CPU_FAMILY_MAX_ID
}CPU_FAMILY_tag;

#define VALID_CPU_FAMILY(x)    (x<CPU_FAMILY_MAX_ID)

/*
 * Define CPU Revision ID
 */
#define CPU_FAMILY_35XX_REVISION_ES_1_0     0
#define CPU_FAMILY_35XX_REVISION_ES_2_0     1
#define CPU_FAMILY_35XX_REVISION_ES_2_1     2
#define CPU_FAMILY_35XX_REVISION_ES_2_0_CRC     3
#define CPU_FAMILY_35XX_REVISION_ES_2_1_CRC     4
#define CPU_FAMILY_35XX_REVISION_ES_3_0     5
#define CPU_FAMILY_35XX_REVISION_ES_3_1     6
#define CPU_FAMILY_37XX_REVISION_ES_1_0     0x10
#define CPU_FAMILY_37XX_REVISION_ES_1_1     0x11
#define CPU_FAMILY_37XX_REVISION_ES_1_2     0x12

#define CPU_FAMILY_35XX_REVISION_ES_FUTURE  0xFE
#define CPU_FAMILY_37XX_REVISION_ES_FUTURE  0xFF



// Note: all revision numbers for all CPU families must be unique!
#define CPU_FAMILY_OMAP35XX_REVISION_ES_1_0     ((CPU_FAMILY_OMAP35XX << CPU_FAMILY_SHIFT) | CPU_FAMILY_35XX_REVISION_ES_1_0)
#define CPU_FAMILY_OMAP35XX_REVISION_ES_2_0     ((CPU_FAMILY_OMAP35XX << CPU_FAMILY_SHIFT) | CPU_FAMILY_35XX_REVISION_ES_2_0)
#define CPU_FAMILY_OMAP35XX_REVISION_ES_2_1     ((CPU_FAMILY_OMAP35XX << CPU_FAMILY_SHIFT) | CPU_FAMILY_35XX_REVISION_ES_2_1)
#define CPU_FAMILY_OMAP35XX_REVISION_ES_3_0     ((CPU_FAMILY_OMAP35XX << CPU_FAMILY_SHIFT) | CPU_FAMILY_35XX_REVISION_ES_3_0)
#define CPU_FAMILY_OMAP35XX_REVISION_ES_3_1     ((CPU_FAMILY_OMAP35XX << CPU_FAMILY_SHIFT) | CPU_FAMILY_35XX_REVISION_ES_3_1)
#define CPU_FAMILY_OMAP35XX_REVISION_ES_2_0_CRC     ((CPU_FAMILY_OMAP35XX << CPU_FAMILY_SHIFT) | CPU_FAMILY_35XX_REVISION_ES_2_0_CRC)
#define CPU_FAMILY_OMAP35XX_REVISION_ES_2_1_CRC     ((CPU_FAMILY_OMAP35XX << CPU_FAMILY_SHIFT) | CPU_FAMILY_35XX_REVISION_ES_2_1_CRC)
#define CPU_FAMILY_OMAP35XX_REVISION_ES_FUTURE     ((CPU_FAMILY_OMAP35XX << CPU_FAMILY_SHIFT) | CPU_FAMILY_35XX_REVISION_ES_FUTURE)


#define CPU_FAMILY_DM37XX_REVISION_ES_1_0     ((CPU_FAMILY_DM37XX << CPU_FAMILY_SHIFT) | CPU_FAMILY_37XX_REVISION_ES_1_0)
#define CPU_FAMILY_DM37XX_REVISION_ES_1_1     ((CPU_FAMILY_DM37XX << CPU_FAMILY_SHIFT) | CPU_FAMILY_37XX_REVISION_ES_1_1)
#define CPU_FAMILY_DM37XX_REVISION_ES_1_2     ((CPU_FAMILY_DM37XX << CPU_FAMILY_SHIFT) | CPU_FAMILY_37XX_REVISION_ES_1_2)
#define CPU_FAMILY_DM37XX_REVISION_ES_FUTURE     ((CPU_FAMILY_DM37XX << CPU_FAMILY_SHIFT) | CPU_FAMILY_37XX_REVISION_ES_FUTURE)


#define CPU_REVISION_UNKNOWN                (DWORD)(-1)

//------------------------------------------------------------------------------
/*
 * Hawkeye values
 */
#define HAWKEYE_OMAP35XX        0xB7AE    /* 3530, 3525, 3515 */
#define HAWKEYE_AM35XX            0x0000    /* 3517 */
#define HAWKEYE_DM37XX            0xB891    /* 3730, 3705*/

#define HAWKEYE_SHIFT           12


/*
 * Chip_id
 */
#define CPU_OMAP3530            0x3530  
#define CPU_OMAP3525            0x3525  
#define CPU_OMAP3515            0x3515  
#define CPU_OMAP3503            0x3503

#define CPU_DM3730            0x3730  
#define CPU_DM3725            0x3725  
#define CPU_DM3715            0x3715  
#define CPU_DM3703            0x3703

#define CPU_AM3517        0x3517

typedef struct {
    UINT16    chip_id;
    UINT16    hawkeye;    /* 16 bits */
    UINT16    family;
    UINT16    feature;
}chip_info_tag;

#define OMAP_35XX_FEATURE_SGX_SHIFT 13
#define OMAP_35XX_FEATURE_SGX_MASK (0x3)
#define OMAP_35XX_FEATURE_SGX(x) ((x>>OMAP_35XX_FEATURE_SGX_SHIFT) & OMAP_35XX_FEATURE_SGX_MASK)

#define OMAP_35XX_FEATURE_IVA_SHIFT 12
#define OMAP_35XX_FEATURE_IVA_MASK (0x1)
#define OMAP_35XX_FEATURE_IVA(x) ((x>>OMAP_35XX_FEATURE_IVA_SHIFT) & OMAP_35XX_FEATURE_IVA_MASK)

/* Function prototype */
extern UINT32 Get_CPUVersion(void);
extern UINT32 Get_CPUMaxSpeed(UINT32 cpu_family);



/* AM335X */
#define NO_OF_DAUGHTER_BOARDS	3

#define AM33X_BOARDID_GP_BOARD      0
#define AM33X_BOARDID_IA_BOARD      1
#define AM33X_BOARDID_IPP_BOARD     2
#define AM33X_BOARDID_BASE_BOARD    3
#define AM33X_BOARDID_BBONE_BOARD   4
#define AM33X_BOARDID_BBONEB_BOARD  5


#define PROFILE_NONE	0x0
#define PROFILE_0	    (1 << 0)
#define PROFILE_1	    (1 << 1)
#define PROFILE_2	    (1 << 2)
#define PROFILE_3	    (1 << 3)
#define PROFILE_4	    (1 << 4)
#define PROFILE_5	    (1 << 5)
#define PROFILE_6	    (1 << 6)
#define PROFILE_7	    (1 << 7)
#define PROFILE_ALL	    0xFF

#define AM335X_BOARD_VERSION_1_0_A      0x0001
#define AM335X_BOARD_VERSION_1_0_B      0x0002
#define AM335X_BOARD_VERSION_1_1_A      0x0010

#define AM335X_BONE_BOARD_VER_A1        0x1001
#define AM335X_BONE_BOARD_VER_A2        0x1002
#define AM335X_BONE_BOARD_VER_A3        0x1003
#define AM335X_BONE_BOARD_VER_A4        0x1004
#define AM335X_BONE_BOARD_VER_A5        0x1005
#define AM335X_BONE_BOARD_VER_A6        0x1006



/* not yet implemented */
extern UINT32 Get_BoardID(void);
extern UINT32 Get_BoardProfile(void);


#if __cplusplus
}
#endif

#endif
