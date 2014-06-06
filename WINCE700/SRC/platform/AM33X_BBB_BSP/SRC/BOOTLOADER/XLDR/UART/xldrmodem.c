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
//  File:  xldrmodem.c
//
//  This file contains X-Loader implementation xmodem protocol
//
#ifdef UART_BOOT

#define XSOH 0x01
#define XEOT 0x04
#define XACK 0x06
#define XNAK 0x15
#define XCAN 0x18

#define XSTATE_SOH_EOT             0x01
#define XSTATE_PKT_NUM             0x02
#define XSTATE_PKT_NUM_1COMPLEMENT 0x03
#define XSTATE_DATA                0x04
#define XSTATE_CHECK_SUM           0x05
#define XSTATE_EOT2                0x06

#define XERROR_NO_ERROR          1 
#define XERROR_START_RESPONSE   -1 
#define XERROR_SOH_EOT          -2
#define XERROR_PKT_NUM          -3
#define XERROR_PKT_NUM_1COMPL   -4
#define XERROR_RECEIVE          -5
#define XERROR_UNKNOWN_STATE    -6
#define XERROR_TIMEOUT          -7

#define START_RESPONSE_TIMEOUT    5  /* sec */
#define RECEIVE_TIMEOUT           1   /* sec */

extern int XLDRReadCharMaxTime(unsigned char *uc, int num_sec);  /* returns error code {-1, 0, 1} */
extern void XLDRWriteChar(const unsigned char c);

static int xblock_cnt = 0;
static int xack_cnt = 0;
static int xnak_cnt = 0;
static int xcan_cnt = 0;
static int xothers_cnt = 0;
static int checksum_error_cnt = 0;
static int dup_pkt_cnt = 0;

void XGetStats
(
	int *p_xblock_cnt,
	int *p_xack_cnt,
	int *p_xnak_cnt,
	int *p_xcan_cnt,
	int *p_xothers_cnt,
	int *p_checksum_error_cnt,
	int *p_dup_pkt_cnt
)
{
	*p_xblock_cnt         = xblock_cnt;
	*p_xack_cnt           = xack_cnt;
	*p_xnak_cnt           = xnak_cnt;
	*p_xcan_cnt           = xcan_cnt;
	*p_xothers_cnt        = xothers_cnt;
	*p_checksum_error_cnt = checksum_error_cnt;
	*p_dup_pkt_cnt        = dup_pkt_cnt;
}

void XWriteStats(const unsigned char c)
{
    if(c == XACK)
        ++xack_cnt;
    else if(c == XNAK)
        ++xnak_cnt;
    else if(c == XCAN)
        ++xcan_cnt;
    else
        ++xothers_cnt;
}

int XWriteChar(const unsigned char c)
{
    XWriteStats(c);
    XLDRWriteChar(c);
	return 0;
}


int XReceive(unsigned char *p_data_buff, int buff_size, unsigned int *p_receive_size)
{
    int si, i;
    int done = 0;
    unsigned char us_ch;

    int xstate;
    unsigned char check_sum;     /* sum of data bytes, ignores any carry */
    unsigned char last_pkt_num = 0xff;
    unsigned char pkt_num = 1;   /* starts from 1, wraps 0xff to 0x00, not to 0x01 */
    unsigned char pkt_data_num = 0;

    i = 0;
    check_sum = 0;
    last_pkt_num = 0xff;
    pkt_num = 1;
    pkt_data_num = 0;
    xstate = XSTATE_SOH_EOT;
    memset(p_data_buff, 0, buff_size);

    XWriteChar(XNAK);

    /* Send NAK every 10 secs to start */
    while (0 == (si = XLDRReadCharMaxTime(&us_ch, START_RESPONSE_TIMEOUT)))
    {
        XWriteChar(XNAK);
    }

    if (si < 0)
	{
		done = XERROR_START_RESPONSE;
		goto DONE;
	}

    while (i < buff_size)
    {
        switch(xstate)
        {
            case XSTATE_SOH_EOT:
                if (us_ch == XEOT)
                {
                    XWriteChar(XNAK);
                    xstate = XSTATE_EOT2;
                }
                else if (us_ch != XSOH)
                {
                    XWriteChar(XCAN);
                    done = XERROR_SOH_EOT;
                }
                else
                    xstate = XSTATE_PKT_NUM;
            break;

            case XSTATE_PKT_NUM:
#if 0
                if (pkt_num != us_ch)
                {
                    XWriteChar(XCAN);
                    done = XERROR_PKT_NUM;
                }
                else
                    xstate = XSTATE_PKT_NUM_1COMPLEMENT;
#else
                if (us_ch == pkt_num)
                {
                    /* Good. Move on */
                    xstate = XSTATE_PKT_NUM_1COMPLEMENT;
                }
                else if (us_ch == last_pkt_num)
                {
                    /* This is a repeat.  Pretend this is the first time to receive the pkt. */
					++dup_pkt_cnt;
                    pkt_num = last_pkt_num;
                    i = i - 128;
                    --xblock_cnt;
                    xstate = XSTATE_PKT_NUM_1COMPLEMENT;
                }
                else
                {
                    /* Not expected pkt_num and not a repeat.  This is bad. */
                    XWriteChar(XCAN);
                    done = XERROR_PKT_NUM;
                }
#endif
            break;

            case XSTATE_PKT_NUM_1COMPLEMENT:
                if((pkt_num + us_ch) != 0xff)
                {
                    XWriteChar(XCAN);
                    done = XERROR_PKT_NUM_1COMPL;
                }
                else
                    xstate = XSTATE_DATA;
            break;

            case XSTATE_DATA:

                p_data_buff[i++] = us_ch;
                check_sum = (unsigned char)(check_sum + us_ch);
                ++pkt_data_num;
                if(pkt_data_num >= 128)
                {
                    pkt_data_num = 0;
                    xstate = XSTATE_CHECK_SUM;
                }
            break;

            case XSTATE_CHECK_SUM:
                if (check_sum != us_ch)
                {
                    XWriteChar(XNAK);

                    /* start over again for this pkt */
					++checksum_error_cnt;
                    i = i - pkt_data_num;
                    memset(p_data_buff+i, 0, pkt_data_num);
                }
                else
                {
                    /* got one pkt */
                    XWriteChar(XACK);
                    ++xblock_cnt;
                }

                /* next pkt */
                ++pkt_num;
                check_sum = 0;
                pkt_data_num = 0;
                xstate = XSTATE_SOH_EOT;
            break;

            case XSTATE_EOT2:
                XWriteChar(XACK);
                done = XERROR_NO_ERROR;
            break;

            default:
                XWriteChar(XCAN);
                done = XERROR_UNKNOWN_STATE;
            break;

        }  /* switch */

        if (done)
            break;


        /* Wait for next incoming char */
        si = XLDRReadCharMaxTime(&us_ch, RECEIVE_TIMEOUT);
        if (si < 0) 
        {
            done = XERROR_RECEIVE;
            break;
        }
		else if (si == 0)
		{
			done = XERROR_TIMEOUT;
			break;
		}
    
    }  /* while */

DONE:
	*p_receive_size = (unsigned int)i;
    return done;
}  /* XReceive */

#endif  /* UART_BOOT */

