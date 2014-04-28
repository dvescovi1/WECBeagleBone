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

#ifndef __TI81XXHDMI_H__
#define __TI81XXHDMI_H__

#ifdef __cplusplus
extern "C" {
#endif


//------------------------------------------------------------------------------

/* IOCLT Supported by this driver */
#define IOCTL_TI81XXHDMI_START     \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0900, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_TI81XXHDMI_STOP     \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0901, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_TI81XXHDMI_GET_STATUS     \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0902, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_TI81XXHDMI_READ_EDID     \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0903, METHOD_BUFFERED, FILE_ANY_ACCESS)

/* Use this command only when hdmi is streaming video to a sink */
/* TODO Not supported for now */
#if 0
#define IOCTL_TI81XXHDMI_GET_CONFIG     \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0904, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_TI81XXHDMI_SET_CONFIG     \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0905, METHOD_BUFFERED, FILE_ANY_ACCESS)

#endif

#define IOCTL_TI81XXHDMI_SET_MODE     \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0906, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_TI81XXHDMI_GET_MODE     \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0907, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_TI81XXHDMI_TEST_HDMI     \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0908, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_TI81XXHDMI_GET_PHY_STAT     \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0909, METHOD_BUFFERED, FILE_ANY_ACCESS)


/* Data Types */

/* Supported resolutions */
enum ti81xxhdmi_mode {
	hdmi_ntsc_mode = 0,
	hdmi_pal_mode,
	hdmi_1080P_60_mode,
	hdmi_720P_60_mode,
	hdmi_1080I_60_mode,
	hdmi_1080P_30_mode,
	hdmi_max_mode
};
struct ti81xxdhmi_edid_params {
	unsigned int slave_address;
	unsigned int segment_ptr;
	unsigned int offset;
	unsigned int no_of_bytes;
	void *buffer_ptr;
	unsigned int no_of_bytes_read;
	unsigned int timeout;
	unsigned int use_eddc_read;
};
struct ti81xxhdmi_phy_status {
	unsigned int rst_done_pclk;
	unsigned int rst_done_pwrclk;
	unsigned int rst_done_scpclk;
	unsigned int rst_done_refclk;
	unsigned int dct_5v_short_clk;
	unsigned int rx_detect;
	unsigned int dct_5v_short_data;
};
struct ti81xxhdmi_status {
	unsigned int is_hpd_detected;
	unsigned int is_hdmi_streaming;
};

#ifdef __cplusplus
}
#endif

#endif /* End of #ifndef __TI81XXHDMI_H__ */

