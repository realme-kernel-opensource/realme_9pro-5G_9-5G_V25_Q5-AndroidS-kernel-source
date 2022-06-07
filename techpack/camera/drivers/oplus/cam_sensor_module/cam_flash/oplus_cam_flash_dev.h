/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2017-2019, The Linux Foundation. All rights reserved.
 */

#ifndef _OPLUS_CAM_FLASH_DEV_H_
#define _OPLUS_CAM_FLASH_DEV_H_

#include <linux/leds.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/leds-qpnp-flash.h>
#include <media/v4l2-subdev.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-event.h>
#include <media/cam_sensor.h>
#include <media/cam_req_mgr.h>
#include "cam_req_mgr_util.h"
#include "cam_req_mgr_interface.h"
#include "cam_subdev.h"
#include "cam_mem_mgr.h"
#include "cam_sensor_cmn_header.h"
#include "cam_soc_util.h"
#include "cam_debug_util.h"
#include "cam_sensor_io.h"
#include "cam_flash_core.h"
#include "cam_context.h"
#include "../../../cam_sensor_module/cam_sensor_extldo_wl2868c/wl2868c.h"
#include <linux/notifier.h>
#include <linux/kernel.h>
#include <linux/reboot.h>

#define TOTAL_FLASH_NUM 1
#define IIC_FLASH_INVALID -1
#define IIC_FLASH_OFF 0
#define IIC_FLASH_ON 1
#define IIC_FLASH_HIGH 2

struct cam_flash_ftm_reg_setting {
	struct cam_sensor_i2c_reg_array reg_setting[10];
	unsigned short size;
	enum camera_sensor_i2c_type addr_type;
	enum camera_sensor_i2c_type data_type;
	unsigned short delay;
};

struct cam_flash_ftm_power_setting {
	enum msm_camera_power_seq_type seq_type;
	long config_val;
	unsigned short delay;
	unsigned short size;
};
struct  cam_flash_probe_info {
	char *flash_name;
	uint32_t slave_write_address;
	uint32_t flash_id_address;
	uint16_t flash_id;
	enum camera_sensor_i2c_type  addr_type;
	enum camera_sensor_i2c_type  data_type;
};
struct cam_flash_ftm_settings {
	uint32_t need_standby_mode;
	struct cam_flash_probe_info flashprobeinfo;
	struct cam_sensor_cci_client cci_client;
	struct cam_flash_ftm_reg_setting flashinitsettings;
	struct cam_flash_ftm_reg_setting flashhighsettings;
	struct cam_flash_ftm_reg_setting flashlowsettings;
	struct cam_flash_ftm_reg_setting flashoffsettings;
	struct cam_flash_ftm_power_setting flashpowerupsetting;
	struct cam_flash_ftm_power_setting flashpowerdownsetting;
};
struct cam_flash_settings {
	uint32_t total_flash_dev;
	uint32_t flash_type;
	int valid_setting_index;
	int cur_flash_status;
	struct cam_flash_ftm_settings flash_ftm_settings[TOTAL_FLASH_NUM];
};
int reigster_flash_shutdown_notifier(void);
int unreigster_flash_shutdown_notifier(void);
int cam_ftm_i2c_set_standby(struct cam_flash_ctrl *flash_ctrl, struct cam_flash_ftm_settings *flash_ftm_data);


void oplus_cam_i2c_flash_proc_init(struct cam_flash_ctrl *flash_ctl, struct i2c_client *client);

void oplus_cam_flash_proc_init(struct cam_flash_ctrl *flash_ctl, struct platform_device *pdev);
int cam_flash_on(struct cam_flash_ctrl *flash_ctrl,
	struct cam_flash_frame_setting *flash_data,
	int mode);

#endif /*_CAM_FLASH_DEV_H_*/
