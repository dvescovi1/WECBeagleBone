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
//===================================================================
//
//   Module Name:   XLDR/EBOOT
//
//   File Name:      mmccard.h
//
//   Description:   data structures and global variables used by MMC code
//                   data structures common to all MMC cards
//
//===================================================================

#ifndef _MMCDISK_H_
#define _MMCDISK_H_

#ifdef __cplusplus
extern "C" {
#endif
		
// Device states
#define STATE_INITING   1
#define STATE_CLOSED    2
#define STATE_OPENED    3
#define STATE_DEAD      4    // Power down
#define STATE_REMOVED   5    // Power down

// Define MMC states
#define MMC_STATE_IDLE      0
#define MMC_STATE_READY     1
#define MMC_STATE_IDENT     2
#define MMC_STATE_STBY      3
#define MMC_STATE_TRAN      4
#define MMC_STATE_DATA      5
#define MMC_STATE_RCV       6
#define MMC_STATE_PRG       7
#define MMC_STATE_DIS       8

#define MMCREAD_SUCCESS         0
#define MMCREAD_FAILURE         1
#define MMCBIT_SET              2
#define MMCBIT_CLEARED          3
#define MMC_WP_NOLOCKSET        4
#define MMC_WP_PERM_LOCKED      5
#define MMC_WP_TEMP_LOCKED      6
#define MMC_WP_PWR_UP_LOCKED    7


#define MMC_WP_NO_WP	    0x0
#define MMC_WP_TEMP_WP	    0x1
#define MMC_WP_PWR_ON_WP	0x2
#define MMC_WP_PERM_WP	    0x3

#define US_PWR_WP_EN 	    (1 << 0)
#define US_PERM_WP_EN	    (1 << 2)
#define US_PWR_WP_DIS	    (1 << 3)
#define US_PERM_WP_DIS	    (1 << 4)
#define CD_PERM_WP_DIS      (1 << 6)
#define PERM_PSWD_DIS       (1 << 7)


//
// Structure to keep track of each IDE interface (each can handle 2 drives, master and slave)
//
//typedef struct {
//   int InterfaceType;             // the type of IDE interface
//   PVOID BaseAddress;             // kernel mode base address of primary registers
//   PVOID DataAddress;             // kernel mode address of data register
//   PVOID BaseAddressAlt;          // kernel mode base address of alternate registers
//   UINT32 RegisterStride;         // number of bytes between registers
//   HANDLE hInterfaceMutex;        // IDE interface MUTEX
//   DWORD dwIntId;                 // SYSINTR number
//   HANDLE hInterfaceEvent;        // handle to event signaled on interrupt
//   volatile PUCHAR vpATAReg;      // pointer to primary registers mapped into driver space
//   volatile PUCHAR vpATADataReg;  // pointer to data register mapped into driver space
//   volatile PUCHAR vpATARegAlt;   // pointer to alternate registers mapped into driver space
//} INTERFACE_INFO;

//
// Structure to keep track of a disk
//
typedef struct _DISK {
	struct _DISK * d_next;
	DWORD Interface;                     // selects which MMC interface this drive is connected to
	DWORD d_DiskCardState;               // current state of drive
	DWORD d_MMCState;                    // current state of MMC card
	DWORD d_CardType;                    // type of card
	DWORD d_RelAddress;                  // card relative address
	DISK_INFO d_DiskInfo;                // for DISK_IOCTL_GET/SETINFOs
	DWORD MaxClkFreq;                    // Maximum clock frequency for interface
	BOOL d_Supports4Bit;
	BOOL d_MMC_HC;
} DISK, * PDISK;

#define CARDTYPE_MMC        1
#define CARDTYPE_SD         2
#define CARDTYPE_SDHC       3

//#define MMC_FLAG_WRITEPROTECT         (1<<0)


/* SD/MMC command definitions */

#define     GO_IDLE_STATE               0
#define     SEND_OP_COND                1
#define     ALL_SEND_CID                2
#define     SET_RELATIVE_ADDR           3
#define     SEND_RELATIVE_ADDRESS       3   // not same number as above -- why didn't they just use a different command number !!
#define     SET_DSR                     4
#define     MMC_SWITCH                  6 
#define     SELECT_DESELECT_CARD        7
#define     SEND_CSD                    9
#define     SEND_CID                    10
#define     READ_DAT_UNTIL_STOP         11
#define     STOP_TRANSMISSION           12
#define     SEND_STATUS                 13
#define     GO_INACTIVE_STATE           15
#define     SET_BLOCKLEN                16
#define     READ_SINGLE_BLOCK           17
#define     READ_MULTIPLE_BLOCK         18
#define     WRITE_DAT_UNTIL_STOP        20
#define     WRITE_BLOCK                 24
#define     WRITE_MULTIPLE_BLOCK        25
#define     PROGRAM_CID                 26
#define     PROGRAM_CSD                 27
#define     SET_WRITE_PROT              28
#define     CLR_WRITE_PROT              29
#define     SEND_WRITE_PROT             30
#define     SEND_WRITE_PROT_TYPE        31
#define     TAG_SECTOR_START            32
#define     TAG_SECTOR_END              33
#define     UNTAG_SECTOR                34
#define     TAG_ERASE_GROUP_START       35
#define     TAG_ERASE_GROUP_END         36
#define     UNTAG_ERASE_GROUP           37
#define     ERASE                       38
#define     LOCK_UNLOCK                 42
#define     APP_CMD                     55
#define     GEN_CMD                     56

/* SD application specific commands -- must follow APP_CMD */
#define     SET_BUS_WIDTH               6
#define     SD_STATUS                   13
#define     SEND_NUM_WR_BLOCKS          22
#define     SET_WR_BLK_ERASE_COUNT      23
#define     SD_SEND_OP_CODE             41
#define     SET_CLR_CARD_DETECT         42
#define     SEND_SCR                    51

/* SD Security commands -- must follow APP_CMD */
#define     GET_MKB                     43
#define     GET_MID                     44
#define     SET_CER_RN1                 45
#define     GET_CER_RN2                 46
#define     SET_CER_RS2                 47
#define     GET_CER_RS2                 48
#define     SECURE_READ_MULTI_BLOCK     18
#define     SECURE_WRITE_MULTI_BLOCK    25
#define     SECURE_ERASE                38
#define     CHANGE_SECURE_AREA          49
#define     SECURE_WRITE_MKB            26

/* SD 2.0 (SDHC) commands, arg 11:8 = VHS (supply voltage), 7:0 = check pattern, response R7 */
#define     SD_SEND_IF_COND             8

#define     MMC_SEND_EXT_CSD            8

struct mmc_card_cid
{
	unsigned char mid;           // 127:120 Manufacturer ID
	unsigned short oid;          // 119:104 OEM/Application ID
	unsigned char pnm[8];        // 103:56 Product Name
	unsigned char prv;           // 55:48 Product revision (bcd coded)
	unsigned int psn;            // 47:16 32 bit binary serial number
	unsigned char mdt;           // 15:8 Manufacturing date (bcd coded)
	unsigned char crc;           // 7:0 (includes bit 0 which is always "1")
};

struct sd_card_cid
{
	unsigned char sdmid;         // 127:120 Mfr ID (gads, what nonsense!)
	unsigned short sdoid;        // see above...
	unsigned char sdpnm[6];      // ugh...
	unsigned char sdprv;         // bcd coded (I think...)
	unsigned int sdpsn;          // 55:24
	unsigned int sdmdt:12;       // 19:8 (rocket scientists...)
	unsigned char sdcrc;         // includes the always set bit 0
};

struct mmc_card_csd
{
	unsigned int csd_struct:2;       // 127:126 CSD Structure
	unsigned int spec_vers:4;        // 125:122 MultiMediaCard Specification version
									 // 121:120 (reserved)
	unsigned char tacc;              // 119:112 Data read access, time-1
	unsigned char nsac;              // 111:104 Data read access, time-2, in clk cycles (nsac*100)
	unsigned char tr_speed;          // 103:96 Max. data transfer rate
	unsigned int ccc:12;             // 95:84 Card command classes
	unsigned int rd_bl_len:4;        // 83:80 Max read data block length (csd code val)
	unsigned int rd_bl_part:1;       // 79 Partial blocks for read allowed
	unsigned int wr_bl_msalign:1;    // 78 Write block misalignment
	unsigned int rd_bl_msalign:1;    // 77 Read block Misalignment
	unsigned int dsr_imp:1;          // 76 DSR implemented
									 // 75:74 (reserved)
	unsigned int c_size:12;          // 73:62 Device size (C_SIZE)
	unsigned int vdd_r_min:3;        // 61:59 Max read current @ vdd min (x10mA)
	unsigned int vdd_r_max:3;        // 58:56 Max read current @ vdd max
	unsigned int vdd_w_min:3;        // 55:53 Max write current @ vdd min
	unsigned int vdd_w_max:3;        // 52:50 Max write current @ vdd max
	unsigned int c_size_mult:3;      // 49:47 Device size multiplier (C_SIZE_MULT)
	unsigned int sector_size:5;      // 46:42 Erase sector size
	unsigned int erase_grp_size:5;   // 41:37 Erase group size
	unsigned int wp_grp_size:5;      // 36:32 Write protect group size
	unsigned int wp_grp_en:1;        // 31 Write protect group enable
	unsigned int default_ecc:2;      // 30:29 manufacturer's default ECC
	unsigned int r2w_factor:3;       // 28:26 Read to Write speed factor
	unsigned int wr_blk_len:4;       // 25:22 Max Write data block length
	unsigned int wr_blk_part:1;      // 21 Partial blocks for write allowed?
									 // 20:16 (reserved)
	unsigned int file_fmt_grp:1;     // 15 indicates file format of selected group
	unsigned int copy:1;             // 14 copy flag (OTP)
	unsigned int perm_wr_prot:1;     // 13 permanent write protect?
	unsigned int tmp_wr_prot:1;      // 12 temporary write protect?
	unsigned int file_fmt:2;         // 11:10 file format of card
	unsigned int ecc:2;              // 9:8 ECC code
	unsigned char crc;               // 7:0, includes bit 0 which is always set to '1'
};

struct sd_card_csd
{
	unsigned int sdcsd_struct:2;     // 127:126 CSD Structure
								     // 125:120 (reserved)
	unsigned char sdtacc;            // 119:112 Data read access, time-1
	unsigned char sdnsac;            // 111:104 Data read access, time-2, in clk cycles (nsac*100)
	unsigned char sdtr_speed;        // 103:96 Max. data transfer rate
	unsigned int sdccc:12;           // 95:84 Card command classes
	unsigned int sdrd_bl_len:4;      // 83:80 Max read data block length (csd code val)
	unsigned int sdrd_bl_part:1;     // 79 Partial blocks for read allowed
	unsigned int sdwr_bl_msalign:1;  // 78 Write block misalignment
	unsigned int sdrd_bl_msalign:1;  // 77 Read block Misalignment
	unsigned int sddsr_imp:1;        // 76 DSR implemented
									 // 75:74 (reserved)
	unsigned int sdc_size:12;        // 73:62 Device size (C_SIZE)
	unsigned int sdvdd_r_min:3;      // 61:59 Max read current @ vdd min (x10mA), not valid for SDHC
	unsigned int sdvdd_r_max:3;      // 58:56 Max read current @ vdd max, not valid for SDHC
	unsigned int sdvdd_w_min:3;      // 55:53 Max write current @ vdd min, not valid for SDHC
	unsigned int sdvdd_w_max:3;      // 52:50 Max write current @ vdd max, not valid for SDHC
	unsigned int sdc_size_mult:3;    // 49:47 Device size multiplier (C_SIZE_MULT), not valid for SDHC
	unsigned int sderase_bk_en:1;    // 46 Erase single block enable, not valid for SDHC
	unsigned int sdsector_size:7;    // 45:39 Erase sector size
	unsigned int sderase_grp_size:7; // 38:32 write protect group size
	unsigned int sdwp_grp_en:1;      // 31 Write protect group enable
									 // 30:29 preserve mmc compatibility (heh...)
	unsigned int sdr2w_factor:3;     // 28:26 Read to Write speed factor
	unsigned int sdwr_blk_len:4;     // 25:22 Max Write data block length
	unsigned int sdwr_blk_part:1;    // 21 Partial blocks for write allowed?
	unsigned int sdreserved2:5;      // 20:16 (reserved)
	unsigned int sdfile_fmt_grp:1;   // 15 indicates file format of selected group
	unsigned int sdcopy:1;           // 14 copy flag (OTP)
	unsigned int sdperm_wr_prot:1;   // 13 permanent write protect?
	unsigned int sdtmp_wr_prot:1;    // 12 temporary write protect?
	unsigned int sdfile_fmt:2;       // 11:10 file format of card
									 // 9:8 (reserved)
	unsigned char sdcrc;             // 7:0, includes bit 0 which is always set to '1'

	unsigned int sdhcc_size;         // 69:48 Device size (C_SIZE) for SDHC
};

// MMC Extended CSD
struct mmc_card_extcsd
{
    UCHAR reserved1[134];       // Bytes 0:133 are reserved
    UCHAR Sec_Bad_Blk_Mgmt;     // 134  Bad Block Management mode
    UCHAR reserved2;            // 135
    UCHAR Enh_Start_Addr[4];    // 136:139 Enhanced User Data Start Address
    UCHAR Ehn_Size_Mult[3];     // 140:142 Enhanced User Data Area Size
    UCHAR Gp_Size_Mult[12];     // 143:154 General Purpose Partition Size
    UCHAR Part_Setting_Cmplt;   // 155  Partition Settings
    UCHAR Part_Attrib;          // 156  Partition Attributes
    UCHAR Max_Enh_Size_Mult[3]; // 157:159 Max Enhanced Area Size
    UCHAR Part_Support;         // 160  Partitioning Support
    UCHAR reserved3;            // 161
    UCHAR Rst_N_Function;       // 162  H/W Reset Function
    UCHAR reserved4[5];         // 163:167
    UCHAR Rpmb_Size_Mult;       // 168  RPMB Size
    UCHAR Fw_Config;            // 169  FW Configuration
    UCHAR reserved5;            // 170
    UCHAR User_Wp;              // 171  User Area Write Protection Register
    UCHAR reserved6;            // 172
    UCHAR Boot_Wp;              // 173  Boot Area Write Protection Register
    UCHAR reserved7;            // 174
    UCHAR Erase_Grp_Def;        // 175  High density Erase group def.
    UCHAR reserved8;            // 176
    UCHAR Boot_Bus_Wdth;        // 177  Boot bus width
    UCHAR reserved9;            // 178
    UCHAR Boot_Cfg;             // 179  Boot configuration
    UCHAR reserved10;           // 180
    UCHAR Erased_Mem_Cont;      // 181  Erase Memory Content
    UCHAR reserved11;           // 182
    UCHAR Bus_Width_Mde;        // 183  Bus Width Mode
    UCHAR reserved12;           // 184
    UCHAR Hs_Timing;            // 185  High Speed interface timing
    UCHAR reserved13;           // 186
    UCHAR Pwr_Class;            // 187  Power Class
    UCHAR reserved14;           // 188
    UCHAR Cmd_Set_Rev;          // 189  Command set revision
    UCHAR reserved15;           // 190
    UCHAR Cmd_Set;              // 191  Command set
    UCHAR Ext_Csd_Rev;          // 192  Extended CSD revision
    UCHAR reserved16;           // 193
    UCHAR Csd_Struct;           // 194  CSD Structure Version
    UCHAR reserved17;           // 195
    UCHAR Card_Type;            // 196  Card Type
    UCHAR reserved18[3];        // 197:199
    UCHAR Pwr_Cl_52_195;        // 200  Power class for 52MHz @ 1.95v
    UCHAR Pwr_Cl_26_195;        // 201  Power class for 26MHz @ 1.95v
    UCHAR Pwr_Cl_52_360;        // 202  Power class for 52MHz @ 3.6v
    UCHAR Pwr_Cl_26_360;        // 203  Power class for 26MHz @ 3.6v
    UCHAR reserved19;           // 204
    UCHAR Min_Perf_R_4_26;      // 205  Min Read Perf for 4bit @ 26MHz
    UCHAR Min_Perf_W_4_26;      // 206  Min Write Perf for 4bit @ 26MHz
    UCHAR Min_Perf_R_8_26;      // 207  Min Read Perf for 8bit @ 26MHz
    UCHAR Min_Perf_W_8_26;      // 208  Min Write Perf for 8bit @ 26MHz
    UCHAR Min_Perf_R_8_52;      // 209  Min Read Perf for 8bit @ 52MHz
    UCHAR Min_Perf_W_8_52;      // 210  Min Write Perf for 8bit @ 52MHz
    UCHAR reserved20;           // 211
    UCHAR Sec_Cnt[4];           // 212:215 Sector Count
    UCHAR reserved21;           // 216
    UCHAR S_A_Timeout;          // 217  Sleep/awake timeout
    UCHAR reserved22;           // 218
    UCHAR S_C_Vccq;             // 219  Sleep current (VccQ)
    UCHAR S_C_Vcc;              // 220  Sleep current (Vcc)
    UCHAR Hc_Wp_Grp_Sze;        // 221  High capacity write protect group size
    UCHAR Rel_Wr_Sec_C;         // 222  Reliable write sector count
    UCHAR Erase_Timeout_Mult;   // 223  High capacity erase timeout
    UCHAR Hc_Erase_Grp_Sze;     // 224  High capacity erase unit size
    UCHAR Acc_Sze;              // 225  Access size
    UCHAR Boot_Sze_Mult;        // 226  Boot Partition size
    UCHAR reserved23;           // 227
    UCHAR Boot_Info;            // 228  Boot information
    UCHAR reserved24[275];      // 229:503
    UCHAR S_Cmd_Set;            // 504  Supported Command sets
    UCHAR reserved25[7];        // 505:511
};

typedef struct mmc_card_extcsd * pmmc_card_extcsd;

struct MMC_command
{
	unsigned int card_type;         // CARDTYPE_MMC, CARDTYPE_SD or CARDTYPE_SDHC
	unsigned char command;
	unsigned int argument;
	unsigned int num_blocks;
	unsigned int block_len;
	unsigned int status;
	unsigned int ocr;
	unsigned int relative_address;   
	unsigned char response[17];      // raw data from low level. Warning: machine dependent
	unsigned char crc;
	unsigned char * pBuffer;

	union                            // since we currently support only one card at a time
	{                                // this can save some space.
		struct mmc_card_cid mmc_cid;
		struct sd_card_cid sd_cid;
	} cid;
	union
	{
		struct mmc_card_csd mmc_csd;
		struct sd_card_csd sd_csd;
	} csd;
   
   struct mmc_card_extcsd mmc_extcsd;
};

/*
 * macros to parse MMC status register
 * requires 32bit status register
 */

#define MMC_STATUS_READY(status)         (status & (1<<8))
#define MMC_STATUS_STATE(status)         ((status >> 9) & 0xf)
#define MMC_STATUS_CMD_ERROR(status)     (status & ((1<<31)|(1<<30)|(1<<29)|(1<<28)|(1<<27)|(1<<26)|(1<<23)|(1<<22)|(1<<20)|(1<<19)|(1<<16)|(1<<13)))
#define MMC_STATUS_POLL_ERROR(status)    (status & ((1<<30)|(1<<26)|(1<<20)|(1<<19)|(1<<18)|(1<<17)|(1<<15)))

/* state encoding in MMC card status register */
#define MMC_STATUS_STATE_IDLE       0
#define MMC_STATUS_STATE_READY      1
#define MMC_STATUS_STATE_IDENT      2
#define MMC_STATUS_STATE_STBY       3
#define MMC_STATUS_STATE_TRAN       4
#define MMC_STATUS_STATE_DATA       5
#define MMC_STATUS_STATE_RCV        6
#define MMC_STATUS_STATE_PRG        7
#define MMC_STATUS_STATE_DIS        8


/*
 * macro that can be used to instantiate a response table
 */

#define MMC_RESPONSE_NONE   0
#define MMC_RESPONSE_R1     1
#define MMC_RESPONSE_R2     2
#define MMC_RESPONSE_R3     3
		
#define RESPONSE_TABLE \
{ \
	MMC_RESPONSE_NONE,       /* CMD0 */      \
	MMC_RESPONSE_R3,         /* CMD1 */      \
	MMC_RESPONSE_R2,         /* CMD2 */      \
	MMC_RESPONSE_R1,         /* CMD3 */      \
	MMC_RESPONSE_NONE,       /* CMD4 */      \
	MMC_RESPONSE_NONE,       /* 5 - res */   \
	MMC_RESPONSE_NONE,       /* 6 - res */   \
	MMC_RESPONSE_R1,         /* CMD7 */      \
	MMC_RESPONSE_R3,         /* CMD8 - SD 2.0 only, 6 byte response */   \
	MMC_RESPONSE_R2,         /* CMD9 */       \
	MMC_RESPONSE_R2,         /* CMD10 */      \
	MMC_RESPONSE_R1,         /* CMD11 */      \
	MMC_RESPONSE_R1,         /* CMD12 */      \
	MMC_RESPONSE_R1,         /* CMD13 */      \
	MMC_RESPONSE_NONE,       /* 14 - res */   \
	MMC_RESPONSE_NONE,       /* CMD15 */      \
	MMC_RESPONSE_R1,         /* CMD16 */      \
	MMC_RESPONSE_R1,         /* CMD17 */      \
	MMC_RESPONSE_R1,         /* CMD18 */      \
	MMC_RESPONSE_NONE,       /* 19 - res */   \
	MMC_RESPONSE_R1,         /* CMD20 */      \
	MMC_RESPONSE_NONE,       /* 21 - res */   \
	MMC_RESPONSE_NONE,       /* 22 - res */   \
	MMC_RESPONSE_NONE,       /* 23 - res */   \
	MMC_RESPONSE_R1,         /* CMD24 */      \
	MMC_RESPONSE_R1,         /* CMD25 */      \
	MMC_RESPONSE_R1,         /* CMD26 */      \
	MMC_RESPONSE_R1,         /* CMD27 */      \
	MMC_RESPONSE_R1,         /* CMD28 */      \
	MMC_RESPONSE_R1,         /* CDM29 */      \
	MMC_RESPONSE_R1,         /* CMD30 */      \
   MMC_RESPONSE_R1,         /* CMD31 */      \
	MMC_RESPONSE_R1,         /* CMD32 */      \
	MMC_RESPONSE_R1,         /* CMD33 */      \
	MMC_RESPONSE_R1,         /* CMD34 */      \
	MMC_RESPONSE_R1,         /* CMD35 */      \
	MMC_RESPONSE_R1,         /* CMD36 */      \
	MMC_RESPONSE_R1,         /* CMD37 */      \
	MMC_RESPONSE_R1,         /* CMD38 */      \
	MMC_RESPONSE_NONE,       /* 39 - not supported */   \
	MMC_RESPONSE_NONE,       /* 40 - not supported */   \
	MMC_RESPONSE_NONE,       /* 41 - not supported */   \
	MMC_RESPONSE_R1,         /* CMD42 */   \
	MMC_RESPONSE_NONE,       /* 43 - not supported */   \
	MMC_RESPONSE_NONE,       /* 44 - not supported */   \
	MMC_RESPONSE_NONE,       /* 45 - not supported */   \
	MMC_RESPONSE_NONE,       /* 46 - not supported */   \
	MMC_RESPONSE_NONE,       /* 47 - not supported */   \
	MMC_RESPONSE_NONE,       /* 48 - not supported */   \
	MMC_RESPONSE_NONE,       /* 49 - not supported */   \
	MMC_RESPONSE_NONE,       /* 50 - not supported */   \
	MMC_RESPONSE_NONE,       /* 51 - not supported */   \
	MMC_RESPONSE_NONE,       /* 52 - not supported */   \
	MMC_RESPONSE_NONE,       /* 53 - not supported */   \
	MMC_RESPONSE_NONE,       /* 54 - not supported */   \
	MMC_RESPONSE_R1,         /* CMD55 */   \
	MMC_RESPONSE_R1          /* CMD56 */   \
}

// Useful for working with SD/SDIO cards:
#define ALT_RESPONSE_TABLE \
{ \
	MMC_RESPONSE_NONE,       /* 0 (res) */   \
	MMC_RESPONSE_NONE,       /* 1 (res) */   \
	MMC_RESPONSE_NONE,       /* 2 (res) */   \
	MMC_RESPONSE_NONE,       /* 3 (res) */   \
	MMC_RESPONSE_NONE,       /* 4 (res) */   \
	MMC_RESPONSE_NONE,       /* 5 (res) */   \
	MMC_RESPONSE_R1,         /* ACMD6 (SET BUS WIDTH) */   \
	MMC_RESPONSE_NONE,       /* 7 (res) */   \
	MMC_RESPONSE_NONE,       /* 8 (res) */   \
	MMC_RESPONSE_NONE,       /* 9 (res) */   \
	MMC_RESPONSE_NONE,       /* 10 (res) */   \
	MMC_RESPONSE_NONE,       /* 11 (res) */   \
	MMC_RESPONSE_NONE,       /* 12 (res) */   \
	MMC_RESPONSE_R1,         /* ACMD13 (SD STATUS) */ \
	MMC_RESPONSE_NONE,       /* 14 (res) */   \
	MMC_RESPONSE_NONE,       /* 15 (res) */   \
	MMC_RESPONSE_NONE,       /* 16 (res) */   \
	MMC_RESPONSE_NONE,       /* 17 (res) */   \
	MMC_RESPONSE_R1,         /* ACMD18 (secure read multi block) */   \
	MMC_RESPONSE_NONE,       /* 19 (res) */   \
	MMC_RESPONSE_NONE,       /* 20 (res) */   \
	MMC_RESPONSE_NONE,       /* 21 (res) */   \
	MMC_RESPONSE_R1,         /* ACMD22 (send num wr blocks) */   \
	MMC_RESPONSE_R1,         /* ACMD23 (set wr block erase count) */   \
	MMC_RESPONSE_NONE,       /* 24 (res) */      \
	MMC_RESPONSE_R1,         /* ACMD25 ( secure write multi block) */ \
	MMC_RESPONSE_R1,         /* ACMD26 ( secure write mkb) */ \
	MMC_RESPONSE_NONE,       /* 27 (res) */   \
	MMC_RESPONSE_NONE,       /* 28 (res) */   \
	MMC_RESPONSE_NONE,       /* 29 (res) */   \
	MMC_RESPONSE_NONE,       /* 30 (res) */   \
	MMC_RESPONSE_NONE,       /* 31 (res) */   \
	MMC_RESPONSE_NONE,       /* 32 (res) */   \
	MMC_RESPONSE_NONE,       /* 33 (res) */   \
	MMC_RESPONSE_NONE,       /* 34 (res) */   \
	MMC_RESPONSE_NONE,       /* 35 (res) */  \
	MMC_RESPONSE_NONE,       /* 36 (res) */   \
	MMC_RESPONSE_NONE,       /* 37 (res) */   \
	MMC_RESPONSE_R1,         /* ACMD38 (secure erase) */   \
	MMC_RESPONSE_NONE,       /* 39 (res) */   \
	MMC_RESPONSE_NONE,       /* 40 (res) */   \
	MMC_RESPONSE_R3,         /* ACMD41 (sd send op code */   \
	MMC_RESPONSE_R1,         /* ACMD42 (set clr card detect pu resistor*/ \
	MMC_RESPONSE_R1,         /* ACMD43 (get mkb) */   \
	MMC_RESPONSE_R1,         /* ACMD44 (get mid) */   \
	MMC_RESPONSE_R1,         /* ACMD45 (set cer RN1) */   \
	MMC_RESPONSE_R1,         /* ACMD46 (get_cer_RN2) */   \
	MMC_RESPONSE_R1,         /* ACMD47 (set cer_rs2) */   \
	MMC_RESPONSE_R1,         /* ACMD48 (get cer_rs2) */   \
	MMC_RESPONSE_NONE,       /* ACMD49 (change secure area) */   \
	MMC_RESPONSE_NONE,       /* 50 (res) */   \
	MMC_RESPONSE_R1,         /* ACMD51 (send scr) */   \
	MMC_RESPONSE_NONE,       /* 52 (padding to make table */   \
	MMC_RESPONSE_NONE,       /* 53 (same length as base table */   \
	MMC_RESPONSE_NONE,       /* 54 */   \
	MMC_RESPONSE_NONE,       /* 55 */   \
	MMC_RESPONSE_NONE        /* 56 */   \
}

#define MMC_TC_COMMAND  0
#define MMC_TC_WRITE    1
#define MMC_TC_READ     2

#define TRANSFER_CLASS_TABLE \
{ \
	MMC_TC_COMMAND,      /* CMD0 */      \
	MMC_TC_COMMAND,      /* CMD1 */      \
	MMC_TC_COMMAND,      /* CMD2 */      \
	MMC_TC_COMMAND,      /* CMD3 */      \
	MMC_TC_COMMAND,      /* CMD4 */      \
	MMC_TC_COMMAND,      /* 5 - res */   \
	MMC_TC_COMMAND,      /* 6 - res */   \
	MMC_TC_COMMAND,      /* CMD7 */      \
	MMC_TC_COMMAND,      /* CMD8 - SD 2.0 only */   \
	MMC_TC_COMMAND,      /* CMD9 */       \
	MMC_TC_COMMAND,      /* CMD10 */      \
	MMC_TC_COMMAND,      /* CMD11 */      \
	MMC_TC_COMMAND,      /* CMD12 */      \
	MMC_TC_COMMAND,      /* CMD13 */      \
	MMC_TC_COMMAND,      /* 14 - res */   \
	MMC_TC_COMMAND,      /* CMD15 */      \
	MMC_TC_COMMAND,      /* CMD16 */      \
	MMC_TC_READ,         /* CMD17 */      \
	MMC_TC_READ,         /* CMD18 */      \
	MMC_TC_COMMAND,      /* 19 - res */   \
	MMC_TC_COMMAND,      /* CMD20 */      \
	MMC_TC_COMMAND,      /* 21 - res */   \
	MMC_TC_COMMAND,      /* 22 - res */   \
	MMC_TC_COMMAND,      /* 23 - res */   \
	MMC_TC_WRITE,        /* CMD24 */      \
	MMC_TC_WRITE,        /* CMD25 */      \
	MMC_TC_COMMAND,      /* CMD26 */      \
	MMC_TC_COMMAND,      /* CMD27 */      \
	MMC_TC_COMMAND,      /* CMD28 */      \
	MMC_TC_COMMAND,      /* CDM29 */      \
	MMC_TC_COMMAND,      /* CMD30 */      \
	MMC_TC_COMMAND,      /* CMD31 */      \
	MMC_TC_COMMAND,      /* CMD32 */      \
	MMC_TC_COMMAND,      /* CMD33 */      \
	MMC_TC_COMMAND,      /* CMD34 */      \
	MMC_TC_COMMAND,      /* CMD35 */      \
	MMC_TC_COMMAND,      /* CMD36 */      \
	MMC_TC_COMMAND,      /* CMD37 */      \
	MMC_TC_COMMAND,      /* CMD38 */      \
	MMC_TC_COMMAND,      /* 39 - not supported */   \
	MMC_TC_COMMAND,      /* 40 - not supported */   \
	MMC_TC_COMMAND,      /* 41 - not supported */   \
	MMC_TC_COMMAND,      /* CMD42 */   \
	MMC_TC_COMMAND,      /* 43 - not supported */   \
	MMC_TC_COMMAND,      /* 44 - not supported */   \
	MMC_TC_COMMAND,      /* 45 - not supported */   \
	MMC_TC_COMMAND,      /* 46 - not supported */   \
	MMC_TC_COMMAND,      /* 47 - not supported */   \
	MMC_TC_COMMAND,      /* 48 - not supported */   \
	MMC_TC_COMMAND,      /* 49 - not supported */   \
	MMC_TC_COMMAND,      /* 50 - not supported */   \
	MMC_TC_COMMAND,      /* 51 - not supported */   \
	MMC_TC_COMMAND,      /* 52 - not supported */   \
	MMC_TC_COMMAND,      /* 53 - not supported */   \
	MMC_TC_COMMAND,      /* 54 - not supported */   \
	MMC_TC_COMMAND,      /* CMD55 */   \
	MMC_TC_COMMAND       /* CMD56 is read or write TC depending on RW bit */   \
}

// Useful for working with SD/SDIO cards:
#define ALT_TRANSFER_CLASS_TABLE \
{ \
	MMC_TC_COMMAND,      /* 0 (res) */   \
	MMC_TC_COMMAND,      /* 1 (res) */   \
	MMC_TC_COMMAND,      /* 2 (res) */   \
	MMC_TC_COMMAND,      /* 3 (res) */   \
	MMC_TC_COMMAND,      /* 4 (res) */   \
	MMC_TC_COMMAND,      /* 5 (res) */   \
	MMC_TC_COMMAND,      /* ACMD6 (SET BUS WIDTH) */   \
	MMC_TC_COMMAND,      /* 7 (res) */   \
	MMC_TC_COMMAND,      /* 8 (res) */   \
	MMC_TC_COMMAND,      /* 9 (res) */   \
	MMC_TC_COMMAND,      /* 10 (res) */   \
	MMC_TC_COMMAND,      /* 11 (res) */   \
	MMC_TC_COMMAND,      /* 12 (res) */   \
	MMC_TC_COMMAND,      /* ACMD13 (SD STATUS) */ \
	MMC_TC_COMMAND,      /* 14 (res) */   \
	MMC_TC_COMMAND,      /* 15 (res) */   \
	MMC_TC_COMMAND,      /* 16 (res) */   \
	MMC_TC_COMMAND,      /* 17 (res) */   \
	MMC_TC_COMMAND,      /* ACMD18 (secure read multi block) */   \
	MMC_TC_COMMAND,      /* 19 (res) */   \
	MMC_TC_COMMAND,      /* 20 (res) */   \
	MMC_TC_COMMAND,      /* 21 (res) */   \
	MMC_TC_COMMAND,      /* ACMD22 (send num wr blocks) */   \
	MMC_TC_COMMAND,      /* ACMD23 (set wr block erase count) */   \
	MMC_TC_COMMAND,      /* 24 (res) */      \
	MMC_TC_COMMAND,      /* ACMD25 ( secure write multi block) */ \
	MMC_TC_COMMAND,      /* ACMD26 ( secure write mkb) */ \
	MMC_TC_COMMAND,      /* 27 (res) */   \
	MMC_TC_COMMAND,      /* 28 (res) */   \
	MMC_TC_COMMAND,      /* 29 (res) */   \
	MMC_TC_COMMAND,      /* 30 (res) */   \
	MMC_TC_COMMAND,      /* 31 (res) */   \
	MMC_TC_COMMAND,      /* 32 (res) */   \
	MMC_TC_COMMAND,      /* 33 (res) */   \
	MMC_TC_COMMAND,      /* 34 (res) */   \
	MMC_TC_COMMAND,      /* 35 (res) */  \
	MMC_TC_COMMAND,      /* 36 (res) */   \
	MMC_TC_COMMAND,      /* 37 (res) */   \
	MMC_TC_COMMAND,      /* ACMD38 (secure erase) */   \
	MMC_TC_COMMAND,      /* 39 (res) */   \
	MMC_TC_COMMAND,      /* 40 (res) */   \
	MMC_TC_COMMAND,      /* ACMD41 (sd send op code */   \
	MMC_TC_COMMAND,      /* ACMD42 (set clr card detect pu resistor*/ \
	MMC_TC_COMMAND,      /* ACMD43 (get mkb) */   \
	MMC_TC_COMMAND,      /* ACMD44 (get mid) */   \
	MMC_TC_COMMAND,      /* ACMD45 (set cer RN1) */   \
	MMC_TC_COMMAND,      /* ACMD46 (get_cer_RN2) */   \
	MMC_TC_COMMAND,      /* ACMD47 (set cer_rs2) */   \
	MMC_TC_COMMAND,      /* ACMD48 (get cer_rs2) */   \
	MMC_TC_COMMAND,      /* ACMD49 (change secure area) */   \
	MMC_TC_COMMAND,      /* 50 (res) */   \
	/*MMC_TC_COMMAND*/ MMC_TC_READ,     /* ACMD51 (send scr) */ \
	MMC_TC_COMMAND,      /* 52 (padding to make table */   \
	MMC_TC_COMMAND,      /* 53 (same length as base table */   \
	MMC_TC_COMMAND,      /* 54 */   \
	MMC_TC_COMMAND,      /* 55 */   \
	MMC_TC_COMMAND       /* 56 */   \
}

#define MMC_OCR_BUSY(ocr)      (!(ocr & (1 << 31)))

#ifdef __cplusplus
}
#endif

#endif // _MMCDISK_H_

