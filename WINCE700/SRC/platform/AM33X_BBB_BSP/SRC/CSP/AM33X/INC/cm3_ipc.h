
/*
 * \file     IPC.h
 *
 * \brief    This file contains the function prototypes for inter-processor 
			 communication between A8 and M3.
 */

/* Copyright (C) 2010 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 */
 
#ifndef      __CM3_IPC_H__
#define      __CM3_IPC_H__
#ifdef __cplusplus
extern "C" {
#endif


/********************** MACROS ***************************/

/********************* COMMANDS **************************/
#define PM_CMD_NO_PM            (0x0)
/*	Initiates force_sleep on interconnect clocks.
 *	Turns off MPU and PER power domains
 *	Programs the RTC alarm register for deasserting pmic_pwr_enable
*/
#define PM_CMD_RTC_MODE			(0x1)

/*	Programs the RTC alarm register for deasserting pmic_pwr_enable	*/
#define PM_CMD_RTC_FAST_MODE	(0x2)

/*	Initates force_sleep on interconnect clocks
 *	Turns off the MPU and PER power domains
 *	Configures the system for disabling MOSC when CM3 executes WFI
*/
#define PM_CMD_DS0_MODE			(0x3)

/*	Initates force_sleep on interconnect clocks
 *	Turns off the MPU power domains
 *	Configures the system for disabling MOSC when CM3 executes WFI
*/
#define PM_CMD_DS1_MODE			(0x5)

/*	Configures the system for disabling MOSC when CM3 executes WFI	*/
#define PM_CMD_DS2_MODE			(0x7)

/******************** COMMAND STATUS *********************/

/* In init phase this denotes that CM3 was initialized successfully. 
When other commands are to be executed, this indicates completion of command */
#define PM_CMD_PASS			(0x0)

/* Early indication of command being carried out */
#define PM_IN_PROGRESS		(0x3)

/* In init phase 0x2 denotes CM3 could not initialize properly. 
When other tasks are to be done, this indicates some error in carrying out the task. */
#define PM_CMD_FAIL			(0x1)

/* CM3 INTC will catch the next WFI of A8 and continue with the pre-defined sequence */
#define PM_WAIT4OK			(0x2)

/**************** Memory config values **********************/

#define PM_MOSC_STATE_OFF				(0x0)
#define PM_MOSC_STATE_ON				(0x1)

#define PM_MPU_POWERSTATE_OFF   		(POWERSTATE_OFF)
#define PM_MPU_POWERSTATE_RET   		(POWERSTATE_RETENTION)
#define PM_MPU_POWERSTATE_ON   			(POWERSTATE_ON)
	
	
#define PM_MPU_RAM_RETSTATE_ON			(MPURAMMEMRETSTATE_MEMRET_DOMAINRET >> \
										MPURAMMEMRETSTATE_SHIFT)
#define PM_MPU_RAM_RETSTATE_OFF			!(PM_MPU_RAM_RETSTATE_ON)
	
	
#define PM_MPU_L1_RETSTATE_ON			(MPUL1MEMRETSTATE_MEMRET_DOMAINRET >> \
										MPUL1MEMRETSTATE_SHIFT)
#define PM_MPU_L1_RETSTATE_OFF			!(PM_MPU_L1_RETSTATE_ON)
	
	
#define PM_MPU_L2_RETSTATE_ON			(MPUL2MEMRETSTATE_MEMRET_DOMAINRET >> \
										MPUL2MEMRETSTATE_SHIFT)
#define PM_MPU_L2_RETSTATE_OFF			!(PM_MPU_L2_RETSTATE_ON)
	
	
#define PM_MPU_RAM_ONSTATE_OFF			(0x0)
#define PM_MPU_RAM_ONSTATE_ON			(MPURAMMEMONSTATE_MEMON_DOMAINON >> MPURAMMEMONSTATE_SHIFT)
	
	
#define PM_PER_POWERSTATE_OFF   		(POWERSTATE_OFF)
#define PM_PER_POWERSTATE_RET   		(POWERSTATE_RETENTION)
#define PM_PER_POWERSTATE_ON   			(POWERSTATE_ON)


#define PM_PER_ICSS_RAM_RETSTATE_RET	(ICSSMEMRETSTATE_MEMRET_DOMAINRET>>ICSSMEMRETSTATE_SHIFT)
#define PM_PER_ICSS_RAM_RETSTATE_OFF	!(PM_PER_ICSS_RAM_RETSTATE_RET)


#define	PM_PER_MEM_RETSTATE_RET			(PERMEMRETSTATE_MEMRET_DOMAINRET>>PERMEMRETSTATE_SHIFT)
#define	PM_PER_MEM_RETSTATE_OFF			!(PM_PER_MEM_RETSTATE_RET)


#define PM_PER_OCMC_RAM_RETSTATE_RET	(RAMMEMRETSTATE_MEMRET_DOMAINRET>>RAMMEMRETSTATE_SHIFT)
#define PM_PER_OCMC_RAM_RETSTATE_OFF	!(PM_PER_OCMC_RAM_RETSTATE_RET)


//#define PM_PER_ICSS_RAM_ONSTATE_RET		(PRM_PER_PM_PER_PWRSTCTRL_ICSS_MEM_ONSTATE_RET)
//#define PM_PER_ICSS_RAM_ONSTATE_OFF		(PRM_PER_PM_PER_PWRSTCTRL_ICSS_MEM_ONSTATE_OFF)
#define PM_PER_ICSS_RAM_ONSTATE_ON		(ICSSMEMONSTATE_MEMON_DOMAINON>>ICSSMEMONSTATE_SHIFT)


//#define PM_PER_MEM_ONSTATE_RET			(PRM_PER_PM_PER_PWRSTCTRL_PER_MEM_ONSTATE_RET)
//#define PM_PER_MEM_ONSTATE_OFF			(PRM_PER_PM_PER_PWRSTCTRL_PER_MEM_ONSTATE_OFF)
#define PM_PER_MEM_ONSTATE_ON			(PERMEMONSTATE_MEMON_DOMAINON>>PERMEMONSTATE_SHIFT)


#define PM_PER_OCMC_RAM_ONSTATE_RET		(RAMMEMONSTATE_MEMRET_DOMAINON>>RAMMEMONSTATE_SHIFT)
#define PM_PER_OCMC_RAM_ONSTATE_OFF		(RAMMEMONSTATE_MEMOFF_DOMAINON>>RAMMEMONSTATE_SHIFT)
#define PM_PER_OCMC_RAM_ONSTATE_ON		(RAMMEMONSTATE_MEMON_DOMAINON>>RAMMEMONSTATE_SHIFT)



/**************** Wake Sources values **********************/

#define WAKE_SOURCES_ALL			(0x1FFF)


/********************** Structure definitions   ***************************/

/*	deep sleep data */
typedef struct
{
	/*	Address to where the control should jump on wake up on A8	*/
	unsigned int resumeAddr:32;

	/* MOSC to be kept on (1) or off (0) */
	unsigned int moscState :1;
	/* Count of how many OSC clocks needs to be seen before exiting deep sleep
		mode. Default = 0x6A75 */
	unsigned int deepSleepCount :16;
	
	/* If vdd_mpu is to be lowered, vdd_mpu in mV */
	unsigned int vddMpuVal :15;
	
	/* Powerstate of PD_MPU */
	unsigned int pdMpuState :2;
	/* State of Sabertooth RAM memory when power domain is in retention */
	unsigned int pdMpuRamRetState :1;
	/* State of L1 memory when power domain is in retention */
	unsigned int pdMpul1RetState :1;	
	/* State of L2 memory when power domain is in retention */
	unsigned int pdMpul2RetState :1;	
	/* State of Sabertooth RAM memory when power domain is ON */
	unsigned int pdMpuRamOnState :2;	
	
	/* Powerstate of PD_PER */
	unsigned int pdPerState :2;	 	
	/* State of ICSS memory when power domain is in retention */
	unsigned int pdPerIcssMemRetState :1;	
	/* State of other memories when power domain is in retention */
	unsigned int pdPerMemRetState :1; 	
	/* State of OCMC memory when power domain is in retention */	
	unsigned int pdPerOcmcRetState :1; 	
	/* State of ICSS memory when power domain is ON */
	unsigned int pdPerIcssMemOnState :2;
	/* State of other memories when power domain is ON */ 	
	unsigned int pdPerMemOnState :2; 	
	/* State of OCMC memory when power domain is ON */
	unsigned int pdPerOcmcOnState :2; 	

	/* Wake sources */
	/* USB, I2C0, RTC_Timer, RTC_Alarm, Timer0, Timer1, UART0, GPIO0_Wake0, \
		GPIO0_Wake1, MPU, WDT0, WDT1, ADTSC*/
	unsigned int wakeSources :13;		
	
	unsigned int reserved :1;
	
	/*	Command id to uniquely identify the intented deep sleep state	*/
	unsigned int cmdID:16;
	
	/* Delay for RTC alarm timeout. Default = 2secs */
	unsigned char rtcTimeoutVal :4;	
	
}deepSleepDataBits;

/*	deep sleep data \
	4 bytes - resume address \
	4 bytes - deep sleep data \
	4 bytes - deep sleep data \
	2 bytes - command id \	
	1 byte  - RTC timeout value \
*/
typedef struct
{
	unsigned int word0;
	unsigned int word1;
	unsigned int word2;
	unsigned short short1;	
	char byte1;
}readDeepSleepData;

typedef union
{
	deepSleepDataBits dsDataBits;
	readDeepSleepData readDsData;	
}deepSleepData;


BOOL PrcmCM3FillDSData(deepSleepData * dsData, UINT32 cmdID);
void PrcmCM3ConfigIPCRegs(deepSleepData *pmDsDataVar);
void PrcmCM3WaitForTxevent(void);
void PrcmCM3ConfigDeepSleep(DWORD suspendMode, deepSleepData * dsData);
void PrcmCM3DumpIPCRegs();



#ifdef __cplusplus
}
#endif

#endif

