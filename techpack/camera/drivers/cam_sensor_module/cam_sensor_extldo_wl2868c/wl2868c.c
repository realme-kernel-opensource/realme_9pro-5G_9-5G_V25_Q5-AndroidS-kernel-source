/*
 * Copyright (C) 2015 HUAQIN Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/types.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>

#include "wl2868c.h"

#ifdef OPLUS_FEATURE_CAMERA_COMMON
static struct wl2868c wl2868c_instance;
#endif
/*****************************************************************************
 * GLobal Variable
 *****************************************************************************/
#define USE_CONTROL_ENGPIO //finger also use this ldo, it will use pinctrl;path is kernel/msm-5.4/driver/regulator/wl2868c.c
static struct i2c_client *wl2868c_i2c_client;
#ifdef USE_CONTROL_ENGPIO
static struct pinctrl *wl2868c_pctrl; /* static pinctrl instance */
#endif

/*****************************************************************************
 * Function Prototype
 *****************************************************************************/
static int wl2868c_dts_probe(struct platform_device *pdev);
static int wl2868c_dts_remove(struct platform_device *pdev);
static int wl2868c_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int wl2868c_i2c_remove(struct i2c_client *client);

/*****************************************************************************
 * Extern Area
 *****************************************************************************/
void wl2868c_set_en_ldo(WL2868C_SELECT ldonum,unsigned int en)
{
	s32 ret=0;
	unsigned int value = 0;
	/*Weifeng.Wang@ODM Cam.Drv 20211124 avoid io was powered down &up twice */
	static int gVLDOCount[WL2868C_MAX] = {0};
	if (NULL == wl2868c_i2c_client) {
		WL2868C_PRINT("[wl2868c] wl2868c_i2c_client is null!!\n");
		return ;
	}

	ret= i2c_smbus_read_byte_data(wl2868c_i2c_client, WL2868C_LDO_EN_ADDR);
	if (ret < 0)
	{
		WL2868C_PRINT("[wl2868c] wl2868c_set_en_ldo read error!\n");
		return;
	}

	if(en == 0)
	{
		// wl2868 spec: bit7 is used for system enable
		//bit7=0,the pmic is shut down
		//bit7=1 and RESET_N is pulled high, the ldo output is controlled by bit0~bit6
		//  ----------------------------------------------------------------------------------------------
		// |    Register   |  Bit7  |   Bit6  |   Bit5  |   Bit4  |   Bit3  |   Bit2  |   Bit1  |   Bit0  |
		// |---------------|--------|---------|---------|---------|---------|---------|---------|---------|
		// | Enable signal | SYS_EN | LDO7_EN | LDO6_EN | LDO5_EN | LDO4_EN | LDO3_EN | LDO2_EN | LDO1_EN |
		//  ----------------------------------------------------------------------------------------------
		value = (ret & (~(0x01<<ldonum)))&0xFF;
		gVLDOCount[ldonum]--;
		if(gVLDOCount[ldonum] > 0)
		{
			WL2868C_PRINT("[wl2868c] wl2868c_set_en_ldo skip ldo=%d power down gVLDOCount[%d]:%d\n", ldonum,ldonum,gVLDOCount[ldonum]);
			return;
		}
	}
	else
	{
		value = (ret|(0x01<<ldonum))|0x80;
		gVLDOCount[ldonum]++;
		if(gVLDOCount[ldonum] > 1)
		{
			WL2868C_PRINT("[wl2868c] wl2868c_set_en_ldo skip ldo=%d power up gVLDOCount[%d]:%d \n", ldonum,ldonum,gVLDOCount[ldonum]);
			return;
		}
	}

	i2c_smbus_write_byte_data(wl2868c_i2c_client,WL2868C_LDO_EN_ADDR,value);
	WL2868C_PRINT("[wl2868c] wl2868c_set_en_ldo ldo=%d,enable before:%x after set :%x  en:%d gVLDOCount[%d]:%d \n",ldonum, ret, value, en, ldonum, gVLDOCount[ldonum]);
	return;
}

//Voutx=0.496v+LDOX_OUT[6:0]*0.008V LDO1/LDO2
//Voutx=1.504v+LDOX_OUT[7:0]*0.008V LDO3~LDO2
void wl2868c_set_ldo_value(WL2868C_SELECT ldonum,unsigned int value)
{
	unsigned int  Ldo_out =0;
       unsigned char regaddr =0;
       s32 ret =0;
	WL2868C_PRINT("[wl2868c] %s enter!!!\n",__FUNCTION__);

	 if (NULL == wl2868c_i2c_client) {
	        WL2868C_PRINT("[wl2868c] wl2868c_i2c_client is null!!\n");
	        return ;
        }
	if(ldonum >= WL2868C_MAX)
	{
		WL2868C_PRINT("[wl2868c] error ldonum not support!!!\n");
		return;
	}

	switch(ldonum)
	{
		case WL2868C_DVDD1:
		case WL2868C_DVDD2:
		//	if(value<600)
		if (value<496) {
			WL2868C_PRINT("[wl2868c] error vol!!!\n");
			goto exit;
		} else {
			//Ldo_out = (value-600)*80/1000;
			Ldo_out = (value - 496)/8;
		}
	       break;
		case WL2868C_AVDD1:
		case WL2868C_AVDD2:
		case WL2868C_VDDAF:
		case WL2868C_VDDOIS:
		case WL2868C_VDDIO:
			if(value<1504)
			{
				WL2868C_PRINT("[wl2868c] error vol!!!\n");
				goto exit;

			}
			else
			{
			  //Ldo_out = (value-1200)*80/1000;
			   Ldo_out = (value-1504)/8;
			}
			break;
		default:
			goto exit;
			break;
	}
	regaddr = ldonum+WL2868C_LDO1_OUT_ADDR;

	WL2868C_PRINT("[wl2868c] ldo=%d,value=%d,Ldo_out:%d,regaddr=0x%x \n",ldonum,value,Ldo_out,regaddr);
	i2c_smbus_write_byte_data(wl2868c_i2c_client,regaddr,Ldo_out);
	ret=i2c_smbus_read_byte_data(wl2868c_i2c_client,regaddr);
	WL2868C_PRINT("[wl2868c] after write ret=0x%x\n",ret);

exit:
	WL2868C_PRINT("[wl2868c] %s exit!!!\n",__FUNCTION__);

}

static struct wl2868c_ldomap ldolist[] = {
	{CAMERA_INDEX_MAIN, SENSOR_WL2868C_VANA, WL2868C_AVDD1}, //BackMain AVDD
	{CAMERA_INDEX_MAIN, SENSOR_WL2868C_VDIG, WL2868C_DVDD1}, //BackMain DVDD
	{CAMERA_INDEX_MAIN, SENSOR_WL2868C_VIO, WL2868C_VDDOIS}, //BackMain IOVDD
	{CAMERA_INDEX_MAIN, SENSOR_WL2868C_VAF, WL2868C_VDDIO}, //BackMain AFVDD

	{CAMERA_INDEX_FRONT, SENSOR_WL2868C_VANA, WL2868C_AVDD2}, //FrontMain AVDD
	{CAMERA_INDEX_FRONT, SENSOR_WL2868C_VDIG, WL2868C_DVDD2}, //FrontMain DVDD
	{CAMERA_INDEX_FRONT, SENSOR_WL2868C_VIO, WL2868C_VDDOIS}, //FrontMain IOVDD

	{CAMERA_INDEX_MAIN_WIDE, SENSOR_WL2868C_VIO, WL2868C_VDDOIS}, //BackWide IOVDD
	{CAMERA_INDEX_MAIN_WIDE, SENSOR_WL2868C_VANA, WL2868C_VDDAF}, //BackWide AVDD
	{CAMERA_INDEX_MAIN_WIDE, SENSOR_WL2868C_VDIG, WL2868C_DVDD2}, //BackWide DVDD

	{CAMERA_INDEX_MAIN_MACRO, SENSOR_WL2868C_VIO, WL2868C_VDDOIS}, //BackMacro IOVDD
	{CAMERA_INDEX_MAIN_MACRO, SENSOR_WL2868C_VANA, WL2868C_VDDAF}, //BackMacro AVDD


	{CAMERA_INDEX_MAIN_DEPTH, SENSOR_WL2868C_VIO, WL2868C_VDDOIS}, //BackDepth IOVDD
	{CAMERA_INDEX_MAIN_DEPTH, SENSOR_WL2868C_VANA, WL2868C_VDDAF}, //BackDepth AVDD
};

int wl2868c_set_ldo_enable(uint32_t sensor_index, enum msm_camera_power_seq_type seq_type, uint32_t min_voltage, uint32_t max_voltage)
{
	WL2868C_SELECT ldonum = WL2868C_NONE;
	unsigned int ldo_vol_value = 0;
	unsigned int i = 0;

	if(sensor_index >= CAMERA_INDEX_MAX_NUM ||
		seq_type >= SENSOR_SEQ_TYPE_MAX ||
		min_voltage < 600000 ||
		max_voltage < 600000 ||
		min_voltage > max_voltage){
		WL2868C_PRINT("[wl2868c] %s invalid parameters!!!\n",__FUNCTION__);
		return -1;
	}

	for(i = 0;i < (sizeof(ldolist) / sizeof(ldolist[0]));i++) {
		if(sensor_index == ldolist[i].sensor_index && seq_type == ldolist[i].seq_type) {
			ldonum = ldolist[i].ldo_selected;
			break;
		}
	}

	if(ldonum == WL2868C_NONE) {
		WL2868C_PRINT("[wl2868c] %s ldo setting not found in ldolist!!!\n",__FUNCTION__);
		return -2;
	}

	ldo_vol_value = min_voltage / 1000;
	#ifdef OPLUS_FEATURE_CAMERA_COMMON
	mutex_lock(&wl2868c_instance.pwl2868c_mutex);
	wl2868c_set_ldo_value(ldonum, ldo_vol_value);
	wl2868c_set_en_ldo(ldonum, 1);
	mutex_unlock(&wl2868c_instance.pwl2868c_mutex);
	#else
	wl2868c_set_ldo_value(ldonum, ldo_vol_value);
	wl2868c_set_en_ldo(ldonum, 1);
	#endif

	return 0;
}

int wl2868c_set_ldo_disable(uint32_t sensor_index, enum msm_camera_power_seq_type seq_type)
{
	WL2868C_SELECT ldonum = WL2868C_NONE;
	unsigned int i = 0;

	if(sensor_index >= CAMERA_INDEX_MAX_NUM ||
					seq_type >= SENSOR_SEQ_TYPE_MAX) {
		WL2868C_PRINT("[wl2868c] %s invalid parameters!!!\n",__FUNCTION__);
		return -1;
	}

	for(i = 0;i < (sizeof(ldolist) / sizeof(ldolist[0]));i++) {
		if(sensor_index == ldolist[i].sensor_index && seq_type == ldolist[i].seq_type) {
			ldonum = ldolist[i].ldo_selected;
			break;
		}
	}

	if(ldonum == WL2868C_NONE) {
		WL2868C_PRINT("[wl2868c] %s ldo setting not found in ldolist!!!\n",__FUNCTION__);
		return -2;
	}

	#ifdef OPLUS_FEATURE_CAMERA_COMMON
	mutex_lock(&wl2868c_instance.pwl2868c_mutex);
	wl2868c_set_en_ldo(ldonum, 0);
	mutex_unlock(&wl2868c_instance.pwl2868c_mutex);
	#else
	wl2868c_set_en_ldo(ldonum, 0);
	#endif
	return 0;
}

/*****************************************************************************
 * Data Structure
 *****************************************************************************/
#ifdef USE_CONTROL_ENGPIO
static const char *wl2868c_state_name[WL2868C_GPIO_STATE_MAX] = {
    "wl2868c_gpio_enp0",
    "wl2868c_gpio_enp1"
};/* DTS state mapping name */
#endif

static const struct of_device_id gpio_of_match[] = {
    { .compatible = "qualcomm,gpio_wl2868c", },
    {},
};

static const struct of_device_id i2c_of_match[] = {
    { .compatible = "qualcomm,i2c_wl2868c", },
    {},
};

static const struct i2c_device_id wl2868c_i2c_id[] = {
    {"WL2868C_I2C", 0},
    {},
};

static struct platform_driver wl2868c_platform_driver = {
    .probe = wl2868c_dts_probe,
    .remove = wl2868c_dts_remove,
    .driver = {
        .name = "WL2868C_DTS",
        .of_match_table = gpio_of_match,
    },
};

static struct i2c_driver wl2868c_i2c_driver = {
/************************************************************
Attention:
Althouh i2c_bus do not use .id_table to match, but it must be defined,
otherwise the probe function will not be executed!
************************************************************/
    .id_table = wl2868c_i2c_id,
    .probe = wl2868c_i2c_probe,
    .remove = wl2868c_i2c_remove,
    .driver = {
        .name = "WL2868C_I2C",
        .of_match_table = i2c_of_match,
    },
};

/*****************************************************************************
 * Function
 *****************************************************************************/
#ifdef USE_CONTROL_ENGPIO
static long wl2868c_set_state(const char *name)
{
    int ret = 0;
    struct pinctrl_state *pState = 0;

    BUG_ON(!wl2868c_pctrl);

    pState = pinctrl_lookup_state(wl2868c_pctrl, name);
    if (IS_ERR(pState)) {
        pr_err("set state '%s' failed\n", name);
        ret = PTR_ERR(pState);
        goto exit;
    }

    /* select state! */
    pinctrl_select_state(wl2868c_pctrl, pState);

exit:
    return ret; /* Good! */
}

void wl2868c_gpio_select_state(WL2868C_GPIO_STATE s)
{
    WL2868C_PRINT("[wl2868c]%s,%d\n",__FUNCTION__,s);

    BUG_ON(!((unsigned int)(s) < (unsigned int)(WL2868C_GPIO_STATE_MAX)));
    wl2868c_set_state(wl2868c_state_name[s]);
}

static long wl2868c_dts_init(struct platform_device *pdev)
{
    int ret = 0;
    struct pinctrl *pctrl;

    /* retrieve */
    pctrl = devm_pinctrl_get(&pdev->dev);
    if (IS_ERR(pctrl)) {
        dev_err(&pdev->dev, "Cannot find disp pinctrl!");
        ret = PTR_ERR(pctrl);
        goto exit;
    }

    wl2868c_pctrl = pctrl;

exit:
    return ret;
}
#endif

static int wl2868c_dts_probe(struct platform_device *pdev)
{
#ifdef USE_CONTROL_ENGPIO
    int ret = 0;

    ret = wl2868c_dts_init(pdev);
    if (ret) {
        WL2868C_PRINT("[wl2868c]wl2868c_dts_probe failed\n");
        return ret;
    }
#endif
    WL2868C_PRINT("[wl2868c] wl2868c_dts_probe success\n");

    return 0;
}

static int wl2868c_dts_remove(struct platform_device *pdev)
{
    platform_driver_unregister(&wl2868c_platform_driver);

    return 0;
}

static int wl2868c_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    if (NULL == client) {
        WL2868C_PRINT("[wl2868c] i2c_client is NULL\n");
        return -1;
    }
    wl2868c_i2c_client = client;
    mutex_init(&wl2868c_instance.pwl2868c_mutex);
#ifdef USE_CONTROL_ENGPIO
    wl2868c_gpio_select_state(WL2868C_GPIO_STATE_ENP0);
#endif

    WL2868C_PRINT("[wl2868c]wl2868c_i2c_probe success addr = 0x%x\n", client->addr);
    return 0;
}

static int wl2868c_i2c_remove(struct i2c_client *client)
{
    wl2868c_i2c_client = NULL;
    i2c_unregister_device(client);
    mutex_destroy(&wl2868c_instance.pwl2868c_mutex);

    return 0;
}

int wl2868c_init_module(void)
{
		if (platform_driver_register(&wl2868c_platform_driver)) {
				WL2868C_PRINT("[wl2868c]Failed to register wl2868c_platform_driver!\n");
				i2c_del_driver(&wl2868c_i2c_driver);
				return -1;
		}

    if (i2c_add_driver(&wl2868c_i2c_driver)) {
        WL2868C_PRINT("[wl2868c]Failed to register wl2868c_i2c_driver!\n");
        return -1;
    }

    return 0;
}

void wl2868c_exit_module(void)
{
    platform_driver_unregister(&wl2868c_platform_driver);
    i2c_del_driver(&wl2868c_i2c_driver);
}

//module_init(wl2868c_init);
//module_exit(wl2868c_exit);

MODULE_AUTHOR("AmyGuo");
MODULE_DESCRIPTION("EXT CAMERA LDO Driver");
MODULE_LICENSE("GPL");