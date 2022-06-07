/*
 * Copyright (C) 2021 Siliconmitus Inc.
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
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/pinctrl/consumer.h>

#define LCD_BIAS_POSCNTL_REG	0x00
#define LCD_BIAS_NEGCNTL_REG	0x01
#define LCD_BIAS_CONTROL_REG	0x03

enum {
	FIRST_VP_AFTER_VN = 0,
	FIRST_VN_AFTER_VP,
};

enum {
	LCD_BIAS_GPIO_STATE_ENP0 = 0,
	LCD_BIAS_GPIO_STATE_ENP1,
	LCD_BIAS_GPIO_STATE_ENN0,
	LCD_BIAS_GPIO_STATE_ENN1,
	LCD_BIAS_GPIO_STATE_MAX,
};

/*****************************************************************************
 * GLobal Variable
 *****************************************************************************/
static struct i2c_client *lcd_bias_i2c_client;
static struct pinctrl *lcd_bias_pctrl;

/*****************************************************************************
 * Function Prototype
 *****************************************************************************/
static int lcd_bias_dts_probe(struct platform_device *pdev);
static int lcd_bias_dts_remove(struct platform_device *pdev);
static int lcd_bias_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int lcd_bias_i2c_remove(struct i2c_client *client);
static void lcd_bias_gpio_select_state(int s);

/*****************************************************************************
 * Extern Area
 *****************************************************************************/
static void lcd_bias_write_byte(unsigned char addr, unsigned char value)
{
    int ret = 0;
    unsigned char write_data[2] = {0};

    write_data[0] = addr;
    write_data[1] = value;

    if (NULL == lcd_bias_i2c_client) {
        printk("[LCD][BIAS] lcd_bias_i2c_client is null!!\n");
        return ;
    }
    ret = i2c_master_send(lcd_bias_i2c_client, write_data, 2);
    if (ret < 0)
        printk("[LCD][BIAS] i2c write data fail!! ret=%d\n", ret);
}

static void lcd_bias_set_vpn(unsigned int en, unsigned int seq, unsigned int value)
{
    unsigned char level;

    level = (value - 4000) / 100;  //eg.  5.0V= 4.0V + Hex 0x0A (Bin 0 1010) * 100mV
    if (seq == FIRST_VP_AFTER_VN) {
        if (en) {
            lcd_bias_gpio_select_state(LCD_BIAS_GPIO_STATE_ENP1);
            lcd_bias_write_byte(LCD_BIAS_POSCNTL_REG, level);
            mdelay(5);
            lcd_bias_gpio_select_state(LCD_BIAS_GPIO_STATE_ENN1);
            lcd_bias_write_byte(LCD_BIAS_NEGCNTL_REG, level);
        } else {
            lcd_bias_gpio_select_state(LCD_BIAS_GPIO_STATE_ENP0);
            mdelay(5);
            lcd_bias_gpio_select_state(LCD_BIAS_GPIO_STATE_ENN0);
        }
    } else if (seq == FIRST_VN_AFTER_VP) {
        if (en) {
            lcd_bias_gpio_select_state(LCD_BIAS_GPIO_STATE_ENN1);
            lcd_bias_write_byte(LCD_BIAS_NEGCNTL_REG, level);
            mdelay(5);
            lcd_bias_gpio_select_state(LCD_BIAS_GPIO_STATE_ENP1);
            lcd_bias_write_byte(LCD_BIAS_POSCNTL_REG, level);
        } else {
            lcd_bias_gpio_select_state(LCD_BIAS_GPIO_STATE_ENN0);
            mdelay(5);
            lcd_bias_gpio_select_state(LCD_BIAS_GPIO_STATE_ENP0);
        }
    }
}

void lcd_bias_set_vspn(unsigned int en, unsigned int seq, unsigned int value)
{
    if ((value <= 4000) || (value > 6500)) {
        printk("[LCD][BIAS] unreasonable voltage value:%d\n", value);
        return ;
    }
    lcd_bias_set_vpn(en, seq, value);
}

/*****************************************************************************
 * Data Structure
 *****************************************************************************/
static const char *lcd_bias_state_name[LCD_BIAS_GPIO_STATE_MAX] = {
    "lcd_bias_gpio_enp0",
    "lcd_bias_gpio_enp1",
    "lcd_bias_gpio_enn0",
    "lcd_bias_gpio_enn1"
};/* DTS state mapping name */

static const struct of_device_id gpio_of_match[] = {
    { .compatible = "qualcomm,gpio_lcd_bias", },
    {},
};

static const struct of_device_id i2c_of_match[] = {
    { .compatible = "qualcomm,i2c_lcd_bias", },
    {},
};

static const struct i2c_device_id lcd_bias_i2c_id[] = {
    {"lcd_Bias_I2C", 0},
    {},
};

static struct platform_driver lcd_bias_platform_driver = {
    .probe = lcd_bias_dts_probe,
    .remove = lcd_bias_dts_remove,
    .driver = {
        .name = "lcd_bias_dts",
        .of_match_table = gpio_of_match,
    },
};

static struct i2c_driver lcd_bias_i2c_driver = {
    .id_table = lcd_bias_i2c_id,
    .probe = lcd_bias_i2c_probe,
    .remove = lcd_bias_i2c_remove,
    .driver = {
        .name = "lcd_bias_i2c",
        .of_match_table = i2c_of_match,
    },
};

/*****************************************************************************
 * Function
 *****************************************************************************/
static long lcd_bias_set_state(const char *name)
{
    int ret = 0;
    struct pinctrl_state *pState = 0;

    BUG_ON(!lcd_bias_pctrl);

    pState = pinctrl_lookup_state(lcd_bias_pctrl, name);
    if (IS_ERR(pState)) {
        pr_err("set state '%s' failed\n", name);
        ret = PTR_ERR(pState);
        goto exit;
    }
    /* select state! */
    pinctrl_select_state(lcd_bias_pctrl, pState);

exit:
    return ret; /* Good! */
}

void lcd_bias_gpio_select_state(int s)
{
    BUG_ON(!((unsigned int)(s) < (unsigned int)(LCD_BIAS_GPIO_STATE_MAX)));
    lcd_bias_set_state(lcd_bias_state_name[s]);
}

static long lcd_bias_dts_init(struct platform_device *pdev)
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

    lcd_bias_pctrl = pctrl;
exit:
    return ret;
}

static int lcd_bias_dts_probe(struct platform_device *pdev)
{
    int ret = 0;

    ret = lcd_bias_dts_init(pdev);
    if (ret) {
        printk("[LCD][BIAS] lcd_bias_dts_probe failed\n");
        return ret;
    }

    printk("[LCD][BIAS] lcd_bias_dts_probe success\n");
    return 0;
}

static int lcd_bias_dts_remove(struct platform_device *pdev)
{

    platform_driver_unregister(&lcd_bias_platform_driver);
    return 0;
}

static int lcd_bias_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    if (NULL == client) {
        printk("[LCD][BIAS] i2c_client is NULL\n");
        return -1;
    }

    lcd_bias_i2c_client = client;
    printk("[LCD][BIAS] lcd_bias_i2c_probe success addr = 0x%x\n", client->addr);
    return 0;
}

static int lcd_bias_i2c_remove(struct i2c_client *client)
{
    lcd_bias_i2c_client = NULL;
    i2c_unregister_device(client);
    return 0;
}

int __init lcd_bias_init(void)
{
    if (i2c_add_driver(&lcd_bias_i2c_driver)) {
        printk("[LCD][BIAS] Failed to register lcd_bias_i2c_driver!\n");
        return -1;
    }

    if (platform_driver_register(&lcd_bias_platform_driver)) {
        printk("[LCD][BIAS] Failed to register lcd_bias_platform_driver!\n");
        i2c_del_driver(&lcd_bias_i2c_driver);
        return -1;
    }
    return 0;
}

void __exit lcd_bias_exit(void)
{
    platform_driver_unregister(&lcd_bias_platform_driver);
    i2c_del_driver(&lcd_bias_i2c_driver);
}

//module_init(lcd_bias_init);
//module_exit(lcd_bias_exit);

MODULE_DESCRIPTION("Qualcomm LCD BIAS I2C Driver");
MODULE_LICENSE("GPL");
