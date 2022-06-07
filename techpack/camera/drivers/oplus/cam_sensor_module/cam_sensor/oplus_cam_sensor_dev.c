// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2017-2020, The Linux Foundation. All rights reserved.
 */
#include "cam_sensor_dev.h"
#include "cam_req_mgr_dev.h"
#include "cam_sensor_soc.h"
#include "cam_sensor_core.h"
#include "oplus_cam_sensor_dev.h"

#define S5KGM1ST_SENSOR_ID		0xF8D1
#define HI846_SENSOR_ID			0x4608
#define OV8856_SENSOR_ID		0x885A
#define GC02K0_SENSOR_ID		0x2385
#define OV02B10_SENSOR_ID		0x002B
#define GC02M1B_SENSOR_ID		0x02e0
#define OV48B_SENSOR_ID			0x5648
#define IMX471_SENSOR_ID		0x0471
#define OV13B10_SENSOR_ID		0x0D42
#define OV64B_SENSOR_ID			0x5664
#define IMX615_SENSOR_ID		0x615
#define OV08BD10_SENSOR_ID		0x5647
#define GC02M1_SENSOR_ID		0xe0
#define OV16A1Q_SENSOR_ID		0x1641
#define IMX581_SENSOR_ID		0x0581
#define S5KJN1SQ_SENSOR_ID		0x38e1

struct cam_sensor_settings sensor_settings = {
#include "cam_sensor_settings.h"
};

struct cam_sensor_settings sensor_init_settings = {
#include "init_camera_setting/s5kgm1st_setting.h"
#include "init_camera_setting/hi846_setting.h"
#include "init_camera_setting/ov8856_setting.h"
#include "init_camera_setting/gc02k0_setting.h"
#include "init_camera_setting/ov02b10_setting.h"
#include "init_camera_setting/gc02m1b_setting.h"
#include "init_camera_setting/ov48b_setting.h"
#include "init_camera_setting/imx471_setting.h"
#include "init_camera_setting/ov13b10_setting.h"
#include "init_camera_setting/ov64b_setting.h"
#include "init_camera_setting/imx615_setting.h"
#include "init_camera_setting/ov08d10_setting.h"
#include "init_camera_setting/ov16a1q_setting.h"
#include "init_camera_setting/imx581_setting.h"
#include "init_camera_setting/s5kjn1sq_setting.h"
};

/* Add for AT camera test */
long oplus_cam_sensor_subdev_ioctl(struct v4l2_subdev *sd,
	unsigned int cmd, void *arg, unsigned int *is_ftm_current_test)
{
	int rc = 0;
	struct cam_sensor_ctrl_t *s_ctrl =
		v4l2_get_subdevdata(sd);

	struct cam_sensor_i2c_reg_setting sensor_setting;
	struct cam_sensor_i2c_reg_setting_array *ptr = NULL;
	switch (cmd) {
	case VIDIOC_CAM_FTM_POWNER_DOWN:
		CAM_ERR(CAM_SENSOR, "FTM stream off");
		rc = cam_sensor_power_down(s_ctrl);
		CAM_ERR(CAM_SENSOR, "FTM power down.rc=%d, sensorid is %x",rc,s_ctrl->sensordata->slave_info.sensor_id);
		break;
	case VIDIOC_CAM_FTM_POWNER_UP:
		rc = cam_sensor_power_up(s_ctrl);
		CAM_ERR(CAM_SENSOR, "FTM power up sensor id 0x%x,result %d",s_ctrl->sensordata->slave_info.sensor_id,rc);
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR, "FTM power up failed!");
			break;
		}
		*is_ftm_current_test = 1;
		switch(s_ctrl->sensordata->slave_info.sensor_id){
		case S5KGM1ST_SENSOR_ID:
			ptr = &sensor_settings.s5kgm1st_setting;
			break;
		case HI846_SENSOR_ID:
			ptr = &sensor_settings.hi846_setting;
			break;
		case OV8856_SENSOR_ID:
			ptr = &sensor_settings.ov8856_setting;
			break;
		case GC02K0_SENSOR_ID:
			ptr = &sensor_settings.gc02k0_setting;
			break;
		case OV02B10_SENSOR_ID:
			ptr = &sensor_settings.ov02b10_setting;
			break;
		case GC02M1B_SENSOR_ID:
			ptr = &sensor_settings.gc02m1b_setting;
			break;
		case OV48B_SENSOR_ID:
			ptr = &sensor_settings.ov48b_setting;
			break;
		case IMX471_SENSOR_ID:
			ptr = &sensor_settings.imx471_setting;
			break;
		case OV13B10_SENSOR_ID:
			ptr = &sensor_settings.ov13b10_setting;
			break;
		case GC02M1_SENSOR_ID:
			ptr = &sensor_settings.gc02m1b_setting;
			break;
		case OV64B_SENSOR_ID:
			ptr = &sensor_settings.ov64b_setting;
			break;
		case IMX615_SENSOR_ID:
			ptr = &sensor_settings.imx615_setting;
			break;
		case OV08BD10_SENSOR_ID:
			ptr = &sensor_settings.ov08d10_setting;
			break;
		case OV16A1Q_SENSOR_ID:
			ptr = &sensor_settings.ov16a1q_setting;
			break;
		case IMX581_SENSOR_ID:
			ptr = &sensor_settings.imx581_setting;
			break;
		case S5KJN1SQ_SENSOR_ID:
			ptr = &sensor_settings.s5kjn1sq_setting;
			break;
		default:
			break;
		}
		if (ptr != NULL){
			sensor_setting.reg_setting = ptr->reg_setting;
			sensor_setting.addr_type = ptr->addr_type;
			sensor_setting.data_type = ptr->data_type;
			sensor_setting.size = ptr->size;
			sensor_setting.delay = ptr->delay;
		}
		rc = camera_io_dev_write(&(s_ctrl->io_master_info), &sensor_setting);

		if (rc < 0) {
			CAM_ERR(CAM_SENSOR, "FTM Failed to write sensor setting");
		} else {
			CAM_ERR(CAM_SENSOR, "FTM successfully to write sensor setting");
		}
		break;
	default:
		CAM_ERR(CAM_SENSOR, "Invalid ioctl cmd: %d", cmd);
		break;
	}
	return rc;
}

int sensor_start_thread(void *arg) {
	struct cam_sensor_ctrl_t *s_ctrl = (struct cam_sensor_ctrl_t *)arg;
	int rc = 0;
	struct cam_sensor_i2c_reg_setting sensor_init_setting;
	struct cam_sensor_i2c_reg_setting_array *ptr;

	if (!s_ctrl)
	{
		CAM_ERR(CAM_SENSOR, "s_ctrl is NULL");
		return -1;
	}
	mutex_lock(&(s_ctrl->cam_sensor_mutex));
	ptr = NULL;

	//power up for sensor
	mutex_lock(&(s_ctrl->sensor_power_state_mutex));
	if(s_ctrl->sensor_power_state == CAM_SENSOR_POWER_OFF)
	{
		rc = cam_sensor_power_up(s_ctrl);
		if(rc < 0) {
			CAM_ERR(CAM_SENSOR, "sensor power up faild!");
		 } else {
			CAM_INFO(CAM_SENSOR, "sensor power up success sensor id 0x%x rc:%d", s_ctrl->sensordata->slave_info.sensor_id, rc);
			s_ctrl->sensor_power_state = CAM_SENSOR_POWER_ON;
		 }
	} else {
		CAM_INFO(CAM_SENSOR, "sensor have power up!");
	}
	mutex_unlock(&(s_ctrl->sensor_power_state_mutex));

	//write initsetting for sensor
	if (rc == 0) {
		mutex_lock(&(s_ctrl->sensor_initsetting_mutex));

		CAM_INFO(CAM_SENSOR, "sensor sensor_initsetting_state:%d", s_ctrl->sensor_initsetting_state);
		if(s_ctrl->sensor_initsetting_state == CAM_SENSOR_SETTING_WRITE_INVALID){
			switch(s_ctrl->sensordata->slave_info.sensor_id){
			case S5KGM1ST_SENSOR_ID:
				ptr = &sensor_init_settings.s5kgm1st_setting;
				CAM_INFO(CAM_SENSOR, "S5KGM1ST_SENSOR_ID");
				break;
			case HI846_SENSOR_ID:
				ptr = &sensor_init_settings.hi846_setting;
				CAM_INFO(CAM_SENSOR, "HI846_SENSOR_ID");
				break;
			case OV8856_SENSOR_ID:
				ptr = &sensor_init_settings.ov8856_setting;
				CAM_INFO(CAM_SENSOR, "OV8856_SENSOR_ID");
				break;
			case GC02K0_SENSOR_ID:
				ptr = &sensor_init_settings.gc02k0_setting;
				CAM_INFO(CAM_SENSOR, "GC02K0_SENSOR_ID");
				break;
			case OV02B10_SENSOR_ID:
				ptr = &sensor_init_settings.ov02b10_setting;
				CAM_INFO(CAM_SENSOR, "OV02B10_SENSOR_ID");
				break;
			case GC02M1B_SENSOR_ID:
				ptr = &sensor_init_settings.gc02m1b_setting;
				CAM_INFO(CAM_SENSOR, "GC02M1B_SENSOR_ID");
				break;
			case OV48B_SENSOR_ID:
				ptr = &sensor_init_settings.ov48b_setting;
				CAM_INFO(CAM_SENSOR, "OV48B_SENSOR_ID");
				break;
			case IMX471_SENSOR_ID:
				ptr = &sensor_init_settings.imx471_setting;
				CAM_INFO(CAM_SENSOR, "IMX471_SENSOR_ID");
				break;
			case OV13B10_SENSOR_ID:
				ptr = &sensor_init_settings.ov13b10_setting;
				CAM_INFO(CAM_SENSOR, "OV13B10_SENSOR_ID");
				break;
			case OV64B_SENSOR_ID:
				ptr = &sensor_init_settings.ov64b_setting;
				CAM_INFO(CAM_SENSOR, "OV64B_SENSOR_ID");
				break;
			case IMX615_SENSOR_ID:
				ptr = &sensor_init_settings.imx615_setting;
				CAM_INFO(CAM_SENSOR, "IMX615_SENSOR_ID");
				break;
			case OV08BD10_SENSOR_ID:
				ptr = &sensor_init_settings.ov08d10_setting;
				CAM_INFO(CAM_SENSOR, "OV08D10_SENSOR_ID");
				break;
			case OV16A1Q_SENSOR_ID:
				ptr = &sensor_init_settings.ov16a1q_setting;
				CAM_INFO(CAM_SENSOR, "OV16A1Q_SENSOR_ID");
				break;
			case IMX581_SENSOR_ID:
				ptr = &sensor_init_settings.imx581_setting;
				CAM_INFO(CAM_SENSOR, "IMX581_SENSOR_ID");
				break;
			case S5KJN1SQ_SENSOR_ID:
				ptr = &sensor_init_settings.s5kjn1sq_setting;
				CAM_INFO(CAM_SENSOR, "S5KJN1SQ_SENSOR_ID");
				break;
			default:
			    CAM_INFO(CAM_SENSOR, "no matching sensor_id");
				break;
			}

			CAM_INFO(CAM_SENSOR, "sensor_init_setting ptr:%p", ptr);
			if (NULL != ptr){
				sensor_init_setting.reg_setting = ptr->reg_setting;
				sensor_init_setting.addr_type = ptr->addr_type;
				sensor_init_setting.data_type = ptr->data_type;
				sensor_init_setting.size = ptr->size;
				sensor_init_setting.delay = ptr->delay;

				rc = camera_io_dev_write(&(s_ctrl->io_master_info), &sensor_init_setting);
				if(rc < 0)
				{
					CAM_ERR(CAM_SENSOR, "write setting failed!");
				} else {
					CAM_INFO(CAM_SENSOR, "write setting success!");
					s_ctrl->sensor_initsetting_state = CAM_SENSOR_SETTING_WRITE_SUCCESS;
				}
			}
			else
			{
				CAM_ERR(CAM_SENSOR, "not find sensor initsetting!");
			}
		} else {
			CAM_INFO(CAM_SENSOR, "sensor setting have write!");
		}
		mutex_unlock(&(s_ctrl->sensor_initsetting_mutex));
	}

	mutex_unlock(&(s_ctrl->cam_sensor_mutex));
	return rc;

}

int cam_sensor_start(struct cam_sensor_ctrl_t *s_ctrl) {
	int rc = 0;

	if(s_ctrl == NULL)
	{
		CAM_ERR(CAM_SENSOR, "s_ctrl is null ");
		return -1;
	}

	mutex_lock(&(s_ctrl->cam_sensor_mutex));

	mutex_lock(&(s_ctrl->sensor_power_state_mutex));
	if(s_ctrl->sensor_power_state == CAM_SENSOR_POWER_OFF)
	{
		s_ctrl->sensor_open_thread = kthread_run(sensor_start_thread, s_ctrl, s_ctrl->device_name);
		if (!s_ctrl->sensor_open_thread) {
			CAM_ERR(CAM_SENSOR, "create sensor start thread failed");
			rc = -1;
		}
		else
		{
			CAM_INFO(CAM_SENSOR, "create sensor start thread success");
		}
	}
	else
	{
		CAM_INFO(CAM_SENSOR, "sensor have power up");
	}
	mutex_unlock(&(s_ctrl->sensor_power_state_mutex));

	mutex_unlock(&(s_ctrl->cam_sensor_mutex));
	return rc;
}

int cam_sensor_stop(struct cam_sensor_ctrl_t *s_ctrl) {
	int rc = 0;
	CAM_INFO(CAM_SENSOR,"sensor do stop");
	mutex_lock(&(s_ctrl->cam_sensor_mutex));

	//power off for sensor
	mutex_lock(&(s_ctrl->sensor_power_state_mutex));
	if(s_ctrl->sensor_power_state == CAM_SENSOR_POWER_ON)
	{
		rc = cam_sensor_power_down(s_ctrl);
		if(rc < 0) {
			CAM_ERR(CAM_SENSOR, "sensor power down faild!");
		 } else {
			CAM_INFO(CAM_SENSOR, "sensor power down success sensor id 0x%x",s_ctrl->sensordata->slave_info.sensor_id);
			s_ctrl->sensor_power_state = CAM_SENSOR_POWER_OFF;
			mutex_lock(&(s_ctrl->sensor_initsetting_mutex));
			s_ctrl->sensor_initsetting_state = CAM_SENSOR_SETTING_WRITE_INVALID;
			mutex_unlock(&(s_ctrl->sensor_initsetting_mutex));
		 }
	} else {
		CAM_INFO(CAM_SENSOR, "sensor have power down!");
	}
	mutex_unlock(&(s_ctrl->sensor_power_state_mutex));

	mutex_unlock(&(s_ctrl->cam_sensor_mutex));
	return rc;
}