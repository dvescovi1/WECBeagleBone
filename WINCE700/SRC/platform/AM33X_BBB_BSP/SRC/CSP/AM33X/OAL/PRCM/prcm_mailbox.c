
#include "windows.h"
#include "oal.h"

#include "am33x.h"
#include "am33x_oal_prcm.h"

/********************** MACROS ***************************/

/*	User Id's	*/
#define MAILBOX_USER_A8 		0
#define MAILBOX_USER_PRU0 		1
#define MAILBOX_USER_PRU1 		2
#define MAILBOX_USER_CM3WKUP 	3


/*	Mailbox Queue's	*/
#define	MAILBOX_QUEUE_0		0
#define	MAILBOX_QUEUE_1		1
#define	MAILBOX_QUEUE_2		2
#define	MAILBOX_QUEUE_3		3
#define	MAILBOX_QUEUE_4		4
#define	MAILBOX_QUEUE_5		5
#define	MAILBOX_QUEUE_6		6
#define	MAILBOX_QUEUE_7		7

#define MESSAGE_VALID		0
#define MESSAGE_INVALID		1

/*************************************************************************\
 * Registers Definition
\*************************************************************************/

#define MAILBOX_REVISION   (0x0)
#define MAILBOX_SYSCONFIG   (0x10)
#define MAILBOX_MESSAGE(n)   (0x40 + (n * 4))
#define MAILBOX_FIFOSTATUS(n)   (0x80 + (n * 4))
#define MAILBOX_MESSAGESTATUS(n)   (0xc0 + (n * 4))
#define MAILBOX_IRQSTATUS_RAW(n)   (0x100 + (n * 0x10))
#define MAILBOX_IRQSTATUS_CLR(n)   (0x104 + (n * 0x10))
#define MAILBOX_IRQENABLE_SET(n)   (0x108 + (n * 0x10))
#define MAILBOX_IRQENABLE_CLR(n)   (0x10c + (n * 0x10))

/**************************************************************************\ 
 * Field Definition Macros
\**************************************************************************/

/* REVISION */
#define MAILBOX_REVISION_CUSTOM   (0x000000C0u)
#define MAILBOX_REVISION_CUSTOM_SHIFT   (0x00000006u)

#define MAILBOX_REVISION_FUNC   (0x0FFF0000u)
#define MAILBOX_REVISION_FUNC_SHIFT   (0x00000010u)

#define MAILBOX_REVISION_MAJOR   (0x00000700u)
#define MAILBOX_REVISION_MAJOR_SHIFT   (0x00000008u)

#define MAILBOX_REVISION_MINOR   (0x0000003Fu)
#define MAILBOX_REVISION_MINOR_SHIFT   (0x00000000u)

#define MAILBOX_REVISION_RES   (0x30000000u)
#define MAILBOX_REVISION_RES_SHIFT   (0x0000001Cu)

#define MAILBOX_REVISION_RTL   (0x0000F800u)
#define MAILBOX_REVISION_RTL_SHIFT   (0x0000000Bu)

#define MAILBOX_REVISION_SCHEME   (0xC0000000u)
#define MAILBOX_REVISION_SCHEME_SHIFT   (0x0000001Eu)


/* SYSCONFIG */
#define MAILBOX_SYSCONFIG_SIDLEMODE   (0x0000000Cu)
#define MAILBOX_SYSCONFIG_SIDLEMODE_SHIFT   (0x00000002u)
#define MAILBOX_SYSCONFIG_SIDLEMODE_FORCEIDLE   (0x0u)
#define MAILBOX_SYSCONFIG_SIDLEMODE_NOIDLE   (0x1u)
#define MAILBOX_SYSCONFIG_SIDLEMODE_RESERVED   (0x3u)
#define MAILBOX_SYSCONFIG_SIDLEMODE_SMARTIDLE   (0x2u)

#define MAILBOX_SYSCONFIG_SOFTRESET   (0x00000001u)
#define MAILBOX_SYSCONFIG_SOFTRESET_SHIFT   (0x00000000u)
#define MAILBOX_SYSCONFIG_SOFTRESET_NORMAL   (0x0u)
#define MAILBOX_SYSCONFIG_SOFTRESET_RESET   (0x1u)


/* MESSAGE */
#define MAILBOX_MESSAGE_MESSAGEVALUEMBM   (0xFFFFFFFFu)
#define MAILBOX_MESSAGE_MESSAGEVALUEMBM_SHIFT   (0x00000000u)

/* FIFOSTATUS_0 */
#define MAILBOX_FIFOSTATUS_FIFOFULLMBM   (0x00000001u)
#define MAILBOX_FIFOSTATUS_FIFOFULLMBM_SHIFT   (0x00000000u)
#define MAILBOX_FIFOSTATUS_FIFOFULLMBM_FULL   (0x1u)
#define MAILBOX_FIFOSTATUS_FIFOFULLMBM_NOTFULL   (0x0u)

/* MESSAGESTATUS */
#define MAILBOX_MESSAGESTATUS_NBOFMSGMBM   (0x00000007u)
#define MAILBOX_MESSAGESTATUS_NBOFMSGMBM_SHIFT   (0x00000000u)

/* IRQSTATUS_RAW */
#define MAILBOX_IRQSTATUS_RAW_NEWMSGSTATUSUUMB(m)   (0x1u << (m*2))
#define MAILBOX_IRQSTATUS_RAW_NEWMSGSTATUSUUMB_SHIFT(m)   (m*2)
#define MAILBOX_IRQSTATUS_RAW_NEWMSGSTATUSUUMB_EVENTPENDING   (0x1u)
#define MAILBOX_IRQSTATUS_RAW_NEWMSGSTATUSUUMB_NOACTION   (0x0u)
#define MAILBOX_IRQSTATUS_RAW_NEWMSGSTATUSUUMB_NOEVENT   (0x0u)
#define MAILBOX_IRQSTATUS_RAW_NEWMSGSTATUSUUMB_SETEVENT   (0x1u)

#define MAILBOX_IRQSTATUS_RAW_NOTFULLSTATUSUUMB(m)   (0x1u << ((m*2)+1))
#define MAILBOX_IRQSTATUS_RAW_NOTFULLSTATUSUUMB_SHIFT(m)   ((m*2)+1)
#define MAILBOX_IRQSTATUS_RAW_NOTFULLSTATUSUUMB_EVENTPENDING   (0x1u)
#define MAILBOX_IRQSTATUS_RAW_NOTFULLSTATUSUUMB_NOACTION   (0x0u)
#define MAILBOX_IRQSTATUS_RAW_NOTFULLSTATUSUUMB_NOEVENT   (0x0u)
#define MAILBOX_IRQSTATUS_RAW_NOTFULLSTATUSUUMB_SETEVENT   (0x1u)


/* IRQSTATUS_CLR */
#define MAILBOX_IRQSTATUS_CLR_NEWMSGSTATUSUUMB(m)   (0x1u << (m*2))
#define MAILBOX_IRQSTATUS_CLR_NEWMSGSTATUSUUMB_SHIFT(m)   (m*2)
#define MAILBOX_IRQSTATUS_CLR_NEWMSGSTATUSUUMB_EVENTPENDING   (0x1u)
#define MAILBOX_IRQSTATUS_CLR_NEWMSGSTATUSUUMB_NOACTION   (0x0u)
#define MAILBOX_IRQSTATUS_CLR_NEWMSGSTATUSUUMB_NOEVENT   (0x0u)
#define MAILBOX_IRQSTATUS_CLR_NEWMSGSTATUSUUMB_SETEVENT   (0x1u)

#define MAILBOX_IRQSTATUS_CLR_NOTFULLSTATUSUUMB(m)   (0x1u << ((m*2)+1))
#define MAILBOX_IRQSTATUS_CLR_NOTFULLSTATUSUUMB_SHIFT(m)   ((m*2)+1)
#define MAILBOX_IRQSTATUS_CLR_NOTFULLSTATUSUUMB_EVENTPENDING   (0x1u)
#define MAILBOX_IRQSTATUS_CLR_NOTFULLSTATUSUUMB_NOACTION   (0x0u)
#define MAILBOX_IRQSTATUS_CLR_NOTFULLSTATUSUUMB_NOEVENT   (0x0u)
#define MAILBOX_IRQSTATUS_CLR_NOTFULLSTATUSUUMB_SETEVENT   (0x1u)


/* IRQENABLE_SET */
#define MAILBOX_IRQENABLE_SET_NEWMSGSTATUSUUMB(m)   (0x1u << (m*2))
#define MAILBOX_IRQENABLE_SET_NEWMSGSTATUSUUMB_SHIFT(m)   (m*2)
#define MAILBOX_IRQENABLE_SET_NEWMSGSTATUSUUMB_EVENTPENDING   (0x1u)
#define MAILBOX_IRQENABLE_SET_NEWMSGSTATUSUUMB_NOACTION   (0x0u)
#define MAILBOX_IRQENABLE_SET_NEWMSGSTATUSUUMB_NOEVENT   (0x0u)
#define MAILBOX_IRQENABLE_SET_NEWMSGSTATUSUUMB_SETEVENT   (0x1u)

#define MAILBOX_IRQENABLE_SET_NOTFULLSTATUSUUMB(m)   (0x1u << ((m*2)+1))
#define MAILBOX_IRQENABLE_SET_NOTFULLSTATUSUUMB_SHIFT(m)   ((m*2)+1)
#define MAILBOX_IRQENABLE_SET_NOTFULLSTATUSUUMB_EVENTPENDING   (0x1u)
#define MAILBOX_IRQENABLE_SET_NOTFULLSTATUSUUMB_NOACTION   (0x0u)
#define MAILBOX_IRQENABLE_SET_NOTFULLSTATUSUUMB_NOEVENT   (0x0u)
#define MAILBOX_IRQENABLE_SET_NOTFULLSTATUSUUMB_SETEVENT   (0x1u)


/* IRQENABLE_CLR */
#define MAILBOX_IRQENABLE_CLR_NEWMSGSTATUSUUMB(m)   (0x1u << (m*2))
#define MAILBOX_IRQENABLE_CLR_NEWMSGSTATUSUUMB_SHIFT(m)   (m*2)
#define MAILBOX_IRQENABLE_CLR_NEWMSGSTATUSUUMB_EVENTPENDING   (0x1u)
#define MAILBOX_IRQENABLE_CLR_NEWMSGSTATUSUUMB_NOACTION   (0x0u)
#define MAILBOX_IRQENABLE_CLR_NEWMSGSTATUSUUMB_NOEVENT   (0x0u)
#define MAILBOX_IRQENABLE_CLR_NEWMSGSTATUSUUMB_SETEVENT   (0x1u)

#define MAILBOX_IRQENABLE_CLR_NOTFULLSTATUSUUMB(m)   (0x1u << ((m*2)+1))
#define MAILBOX_IRQENABLE_CLR_NOTFULLSTATUSUUMB_SHIFT(m)   ((m*2)+1)
#define MAILBOX_IRQENABLE_CLR_NOTFULLSTATUSUUMB_EVENTPENDING   (0x1u)
#define MAILBOX_IRQENABLE_CLR_NOTFULLSTATUSUUMB_NOACTION   (0x0u)
#define MAILBOX_IRQENABLE_CLR_NOTFULLSTATUSUUMB_NOEVENT   (0x0u)
#define MAILBOX_IRQENABLE_CLR_NOTFULLSTATUSUUMB_SETEVENT   (0x1u)


/**
 *  \brief   This function resets the mailbox
 *
 * \param     baseAdd	Memory address of the mailbox instance used.
 * \return 	  None
 */
 
void MBResetMailbox(volatile UINT32 baseAdd)
{
	/*	Start the soft reset sequence	*/
	OUTREG32(baseAdd + MAILBOX_SYSCONFIG,(MAILBOX_SYSCONFIG_SOFTRESET_RESET << MAILBOX_SYSCONFIG_SOFTRESET_SHIFT));
					
	/*	Wait till the reset is complete	*/
	while((INREG32(baseAdd + MAILBOX_SYSCONFIG) & (MAILBOX_SYSCONFIG_SOFTRESET << MAILBOX_SYSCONFIG_SOFTRESET_SHIFT)));
}

/**
 *  \brief   This function enables the new message interrupt for a user for given queue
 *
 * \param     baseAdd	Memory address of the mailbox instance used.
 * \param     userId	User for whom the new meaasge should be intimated
 * \param     queueId	Queue to be monitored for new message
 *
 * \return 	  None
 */
 
void MBEnableNewMsgInt(volatile UINT32 baseAdd, unsigned int userId, unsigned int queueId)
{
	OUTREG32((baseAdd + MAILBOX_IRQENABLE_SET(userId)),(MAILBOX_IRQENABLE_SET_NEWMSGSTATUSUUMB(queueId)));
}

/**
 *  \brief   This function configures the idle mode of the mailbox
 *
 * \param     baseAdd	Memory address of the mailbox instance used.
 * \param     idleMode	Idle mode to be configured. Possible values are
 *						0x0: Force-idle. An idle request is acknowledged unconditionally
 *						0x1: No-idle. An idle request is never acknowledged
 *						0x2: Smart-idle. Acknowledgement to an idle request is given based 
 *						on the internal activity of the module
 * \return 	  None
 */
 
void MBConfigIdleMode(volatile UINT32 baseAdd, unsigned int idleMode)
{
	/*	Configure idle mode	*/
	OUTREG32(baseAdd + MAILBOX_SYSCONFIG,(idleMode << MAILBOX_SYSCONFIG_SIDLEMODE_SHIFT));
}

/**
 *  \brief   This function gets the first message in the queue
 *
 * \param     baseAdd	Memory address of the mailbox instance used.
 * \param     queueId	Queue to be read
 * \param     *msgPtr	Message pointer in which the message will be returned
 *
 * \return 	  Validity	The return value indicates whether the message is valid
 */
 
unsigned int MBGetMessage(volatile UINT32 baseAdd, unsigned int queueId, unsigned int *msgPtr)
{
	/*	Check if queue is not empty	*/
	if((INREG32(baseAdd + MAILBOX_MESSAGESTATUS(queueId)) & 
		(MAILBOX_MESSAGESTATUS_NBOFMSGMBM << 
			MAILBOX_MESSAGESTATUS_NBOFMSGMBM_SHIFT)) > 0)
	{
		/*	Read message	*/
		*msgPtr = INREG32(baseAdd + MAILBOX_MESSAGE(queueId));
		return MESSAGE_VALID;
	}
	else
	{
		/*	Queue empty*/
		return MESSAGE_INVALID;
	}
}

/**
 *  \brief   This function writes message in the queue
 *
 * \param     baseAdd	Memory address of the mailbox instance used.
 * \param     queueId	Queue to be written
 * \param     msg		Message which has to be sent
 *
 * \return 	  status	The return value indicates whether the message is 
 *			  			written to the queue. Possible values are,
 *							0	-	Written successfully
 *							0	-	Queue full
 */
 
unsigned int MBSendMessage(volatile UINT32 baseAdd, unsigned int queueId, unsigned int msg)
{

	unsigned int fifoFullMask = (MAILBOX_FIFOSTATUS_FIFOFULLMBM << 
							MAILBOX_FIFOSTATUS_FIFOFULLMBM_SHIFT);

	/*	Check if queue is not full	*/
	if((INREG32(baseAdd + MAILBOX_FIFOSTATUS(queueId)) & fifoFullMask)  != fifoFullMask)
	{
		/*	Write message	*/
		OUTREG32(baseAdd + MAILBOX_MESSAGE(queueId), msg);
		return (!fifoFullMask);
	}
	else
	{	
		/*	Queue full	*/
		return (fifoFullMask);
	}
}

/**
 *  \brief   This function clears the queue not-full status
 *
 * \param     baseAdd	Memory address of the mailbox instance used.
 * \param     userId	User for whom the event should be cleared
 * \param     queueId	Queue for  which the event should be cleared
 *
 * \return 	  None
 */
 
void MBClrNewMsgStatus(volatile UINT32 baseAdd, unsigned int userId, unsigned int queueId)
{
	OUTREG32(baseAdd + MAILBOX_IRQSTATUS_CLR(userId),
                (MAILBOX_IRQSTATUS_CLR_NEWMSGSTATUSUUMB(queueId)));
}


/*
** Clear mail box messages
*/
void MBClearMailboxMsg(volatile UINT32 baseAdd)
{
	unsigned int temp;
	/*	Read the message back	*/
	MBGetMessage(baseAdd, MAILBOX_QUEUE_0, &temp); 
	/*	Clear new message status	*/
	MBClrNewMsgStatus(baseAdd, MAILBOX_USER_CM3WKUP, MAILBOX_QUEUE_0);
}


/*
** Generate Mailbox interrupt to CM3 by writing a dummy vlaue to mailbox register
*/
void MBGenerateMailboxInt(volatile UINT32 baseAdd)
{
	/*	Write to Mailbox register	*/
	MBSendMessage(baseAdd, MAILBOX_QUEUE_0, 0x12345678u);
}


/*
** Initialize the Mailbox
**	- Enalbe clock and reset mailbox
*/
void MBInitializeMailbox()
{
    volatile UINT32 baseAdd = (UINT32)OALPAtoUA(AM33X_MBOX_REGS_PA);
    
	/*	Enable Mailbox clock	*/
	PrcmDeviceEnableClocks(AM_DEVICE_MAILBOX0,TRUE);
	
	/*	Reset Mailbox	*/
	MBResetMailbox(baseAdd);
	
	/*	Clear new message status	*/
	MBClrNewMsgStatus(baseAdd, MAILBOX_USER_CM3WKUP, MAILBOX_QUEUE_0);
	
	/*	Enable new message interrupt	*/
	MBEnableNewMsgInt(baseAdd, MAILBOX_USER_CM3WKUP, MAILBOX_QUEUE_0);
	
	/*	Configure idle mode	*/
	MBConfigIdleMode(baseAdd, MAILBOX_SYSCONFIG_SIDLEMODE_FORCEIDLE);
}

