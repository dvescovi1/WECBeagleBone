/*
 * Copyright © 2007-2008 Intel Corporation
 *   Jesse Barnes <jesse.barnes@intel.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef __DRM_EDID_H__
#define __DRM_EDID_H__


#define EDID_LENGTH 128
#define DDC_ADDR 0x50

#define CEA_EXT	    0x02
#define VTB_EXT	    0x10
#define DI_EXT	    0x40
#define LS_EXT	    0x50
#define MI_EXT	    0x60


#pragma pack(1)

struct est_timings {
	UINT8 t1;
	UINT8 t2;
	UINT8 mfg_rsvd;
};

/* 00=16:10, 01=4:3, 10=5:4, 11=16:9 */
#define EDID_TIMING_ASPECT_SHIFT 6
#define EDID_TIMING_ASPECT_MASK  (0x3 << EDID_TIMING_ASPECT_SHIFT)

/* need to add 60 */
#define EDID_TIMING_VFREQ_SHIFT  0
#define EDID_TIMING_VFREQ_MASK   (0x3f << EDID_TIMING_VFREQ_SHIFT)

struct std_timing {
	UINT8 hsize; /* need to multiply by 8 then add 248 */
	UINT8 vfreq_aspect;
};

#define DRM_EDID_PT_HSYNC_POSITIVE (1 << 1)
#define DRM_EDID_PT_VSYNC_POSITIVE (1 << 2)
#define DRM_EDID_PT_SEPARATE_SYNC  (3 << 3)
#define DRM_EDID_PT_STEREO         (1 << 5)
#define DRM_EDID_PT_INTERLACED     (1 << 7)

/* If detailed data is pixel timing */
struct detailed_pixel_timing {
	UINT8 hactive_lo;
	UINT8 hblank_lo;
	UINT8 hactive_hblank_hi;
	UINT8 vactive_lo;
	UINT8 vblank_lo;
	UINT8 vactive_vblank_hi;
	UINT8 hsync_offset_lo;
	UINT8 hsync_pulse_width_lo;
	UINT8 vsync_offset_pulse_width_lo;
	UINT8 hsync_vsync_offset_pulse_width_hi;
	UINT8 width_mm_lo;
	UINT8 height_mm_lo;
	UINT8 width_height_mm_hi;
	UINT8 hborder;
	UINT8 vborder;
	UINT8 misc;
};

/* If it's not pixel timing, it'll be one of the below */
struct detailed_data_string {
	UINT8 str[13];
};

struct detailed_data_monitor_range {
	UINT8 min_vfreq;
	UINT8 max_vfreq;
	UINT8 min_hfreq_khz;
	UINT8 max_hfreq_khz;
	UINT8 pixel_clock_mhz; /* need to multiply by 10 */
	UINT8 flags;
	union {
		struct {
			UINT8 reserved;
			UINT8 hfreq_start_khz; /* need to multiply by 2 */
			UINT8 c; /* need to divide by 2 */
			UINT16 m;
			UINT8 k;
			UINT8 j; /* need to divide by 2 */
		}gtf2;
		struct {
			UINT8 version;
			UINT8 data1; /* high 6 bits: extra clock resolution */
			UINT8 data2; /* plus low 2 of above: max hactive */
			UINT8 supported_aspects;
			UINT8 flags; /* preferred aspect and blanking support */
			UINT8 supported_scalings;
			UINT8 preferred_refresh;
		}cvt;
	} formula;
};

struct detailed_data_wpindex {
	UINT8 white_yx_lo; /* Lower 2 bits each */
	UINT8 white_x_hi;
	UINT8 white_y_hi;
	UINT8 gamma; /* need to divide by 100 then add 1 */
};

struct detailed_data_color_point {
	UINT8 windex1;
	UINT8 wpindex1[3];
	UINT8 windex2;
	UINT8 wpindex2[3];
};

struct cvt_timing {
	UINT8 code[3];
};

struct detailed_non_pixel {
	UINT8 pad1;
	UINT8 type; /* ff=serial, fe=string, fd=monitor range, fc=monitor name
		    fb=color point data, fa=standard timing data,
		    f9=undefined, f8=mfg. reserved */
	UINT8 pad2;
	union {
		struct detailed_data_string str;
		struct detailed_data_monitor_range range;
		struct detailed_data_wpindex color;
		struct std_timing timings[6];
		struct cvt_timing cvt[4];
	} data;
};

#define EDID_DETAIL_EST_TIMINGS 0xf7
#define EDID_DETAIL_CVT_3BYTE 0xf8
#define EDID_DETAIL_COLOR_MGMT_DATA 0xf9
#define EDID_DETAIL_STD_MODES 0xfa
#define EDID_DETAIL_MONITOR_CPDATA 0xfb
#define EDID_DETAIL_MONITOR_NAME 0xfc
#define EDID_DETAIL_MONITOR_RANGE 0xfd
#define EDID_DETAIL_MONITOR_STRING 0xfe
#define EDID_DETAIL_MONITOR_SERIAL 0xff

struct detailed_timing {
	UINT16 pixel_clock; /* need to multiply by 10 KHz */
	union {
		struct detailed_pixel_timing pixel_data;
		struct detailed_non_pixel other_data;
	} data;
};

#define DRM_EDID_INPUT_SERRATION_VSYNC (1 << 0)
#define DRM_EDID_INPUT_SYNC_ON_GREEN   (1 << 1)
#define DRM_EDID_INPUT_COMPOSITE_SYNC  (1 << 2)
#define DRM_EDID_INPUT_SEPARATE_SYNCS  (1 << 3)
#define DRM_EDID_INPUT_BLANK_TO_BLACK  (1 << 4)
#define DRM_EDID_INPUT_VIDEO_LEVEL     (3 << 5)
#define DRM_EDID_INPUT_DIGITAL         (1 << 7)
#define DRM_EDID_DIGITAL_DEPTH_MASK    (7 << 4)
#define DRM_EDID_DIGITAL_DEPTH_UNDEF   (0 << 4)
#define DRM_EDID_DIGITAL_DEPTH_6       (1 << 4)
#define DRM_EDID_DIGITAL_DEPTH_8       (2 << 4)
#define DRM_EDID_DIGITAL_DEPTH_10      (3 << 4)
#define DRM_EDID_DIGITAL_DEPTH_12      (4 << 4)
#define DRM_EDID_DIGITAL_DEPTH_14      (5 << 4)
#define DRM_EDID_DIGITAL_DEPTH_16      (6 << 4)
#define DRM_EDID_DIGITAL_DEPTH_RSVD    (7 << 4)
#define DRM_EDID_DIGITAL_TYPE_UNDEF    (0)
#define DRM_EDID_DIGITAL_TYPE_DVI      (1)
#define DRM_EDID_DIGITAL_TYPE_HDMI_A   (2)
#define DRM_EDID_DIGITAL_TYPE_HDMI_B   (3)
#define DRM_EDID_DIGITAL_TYPE_MDDI     (4)
#define DRM_EDID_DIGITAL_TYPE_DP       (5)

#define DRM_EDID_FEATURE_DEFAULT_GTF		(1 << 0)
#define DRM_EDID_FEATURE_PREFERRED_TIMING	(1 << 1)
#define DRM_EDID_FEATURE_STANDARD_COLOR		(1 << 2)
/* If analog */
#define DRM_EDID_FEATURE_DISPLAY_TYPE		(3 << 3) /* 00=mono, 01=rgb, 10=non-rgb, 11=unknown */
/* If digital */
#define DRM_EDID_FEATURE_COLOR_MASK			(3 << 3)
#define DRM_EDID_FEATURE_RGB				(0 << 3)
#define DRM_EDID_FEATURE_RGB_YCRCB444		(1 << 3)
#define DRM_EDID_FEATURE_RGB_YCRCB422		(2 << 3)
#define DRM_EDID_FEATURE_RGB_YCRCB			(3 << 3) /* both 4:4:4 and 4:2:2 */

#define DRM_EDID_FEATURE_PM_ACTIVE_OFF		(1 << 5)
#define DRM_EDID_FEATURE_PM_SUSPEND			(1 << 6)
#define DRM_EDID_FEATURE_PM_STANDBY			(1 << 7)

struct edid {
	UINT8 header[8];
	/* Vendor & product info */
	UINT8 mfg_id[2];
	UINT8 prod_code[2];
	UINT32 serial; /* FIXME: byte order */
	UINT8 mfg_week;
	UINT8 mfg_year;
	/* EDID version */
	UINT8 version;
	UINT8 revision;
	/* Display info: */
	UINT8 input;
	UINT8 width_cm;
	UINT8 height_cm;
	UINT8 gamma;
	UINT8 features;
	/* Color characteristics */
	UINT8 red_green_lo;
	UINT8 black_white_lo;
	UINT8 red_x;
	UINT8 red_y;
	UINT8 green_x;
	UINT8 green_y;
	UINT8 blue_x;
	UINT8 blue_y;
	UINT8 white_x;
	UINT8 white_y;
	/* Est. timings and mfg rsvd timings*/
	struct est_timings established_timings;
	/* Standard timings 1-8*/
	struct std_timing standard_timings[8];
	/* Detailing timings 1-4 */
	struct detailed_timing detailed_timings[4];
	/* Number of 128 byte ext. blocks */
	UINT8 extensions;
	/* Checksum */
	UINT8 checksum;
};

#define EDID_PRODUCT_ID(e) ((e)->prod_code[0] | ((e)->prod_code[1] << 8))

#pragma pack()   // return packing to normal

/* Short Audio Descriptor */
struct cea_sad {
	UINT8 format;
	UINT8 channels; /* max number of channels - 1 */
	UINT8 freq;
	UINT8 byte2; /* meaning depends on format */
};




struct drm_encoder;
struct drm_connector;
struct drm_display_mode;
struct hdmi_avi_infoframe;
struct hdmi_vendor_infoframe;




void drm_edid_to_eld(struct drm_connector *connector, struct edid *edid);
int drm_edid_to_sad(struct edid *edid, struct cea_sad **sads);
int drm_edid_to_speaker_allocation(struct edid *edid, UINT8 **sadb);
int drm_av_sync_delay(struct drm_connector *connector,
		      struct drm_display_mode *mode);
struct drm_connector *drm_select_eld(struct drm_encoder *encoder,
				     struct drm_display_mode *mode);
int drm_load_edid_firmware(struct drm_connector *connector);

int
drm_hdmi_avi_infoframe_from_display_mode(struct hdmi_avi_infoframe *frame,
					 const struct drm_display_mode *mode);
int
drm_hdmi_vendor_infoframe_from_display_mode(struct hdmi_vendor_infoframe *frame,
					    const struct drm_display_mode *mode);

#endif /* __DRM_EDID_H__ */
