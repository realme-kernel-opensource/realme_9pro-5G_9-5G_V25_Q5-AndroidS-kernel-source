#ifndef _WL2868C_H_
#define _WL2868C_H_

#define WL2868C_CHIP_REV_ADDR    0x00
#define FAN53870_CHIP_REV_ADDR   0x01

#define WL2868C_DIC_ADDR         0x02

#define WL2868C_LDO_I2C_ADDR     0x2F
#define FAN53870_LDO_I2C_ADDR    0x35

#define CAMERA_LDO_WL2868C  0x82 //VERID
#define CAMERA_LDO_FAN53870 0x01

#define LDO1_OUT_ADDR    0x04 //EXT_LDO1

#define WL2868C_LDO1_OUT_ADDR    0x03 //EXT_LDO1

#define LDO21_SEQ_ADDR   0x0B
#define LDO43_SEQ_ADDR   0x0C
#define LDO65_SEQ_ADDR   0x0D
#define LDO07_SEQ_ADDR   0x0E

#define WL2868C_LDO21_SEQ_ADDR   0x0A
#define WL2868C_LDO43_SEQ_ADDR   0x0B
#define WL2868C_LDO65_SEQ_ADDR   0x0C
#define WL2868C_LDO07_SEQ_ADDR   0x0D

#define FAN53870_LDO_EN_ADDR         0x03  //bit0:LDO1 ~ bit6:LDO7
#define WL2868C_LDO_EN_ADDR          0x0E  //bit0:LDO1 ~ bit6:LDO7
#define WL2868C_LDO_SEQ_CTRL_ADDR    0x0F

#define WL2868C_PRINT pr_info //printk

typedef enum {
	EXT_NONE=-1,
	EXT_LDO1,
	EXT_LDO2,
	EXT_LDO3,
	EXT_LDO4,
	EXT_LDO5,
	EXT_LDO6,
	EXT_LDO7,
	EXT_MAX
} EXT_SELECT;

/* DTS state */
typedef enum {
	WL2868C_GPIO_STATE_ENP0,
	WL2868C_GPIO_STATE_ENP1,
	WL2868C_GPIO_STATE_MAX,	/* for array size */
} WL2868C_GPIO_STATE;

enum CAM_SENSOR_INDEX {
	CAMERA_INDEX_MAIN = 0,    //BackMain
	CAMERA_INDEX_MONO,		  //BackMono
	CAMERA_INDEX_FRONT,		  //FrontMain
	CAMERA_INDEX_MACRO,   	  //BackMacro
	CAMERA_INDEX_MAX_NUM,
};

enum msm_camera_power_seq_type {
	SENSOR_MCLK,
	SENSOR_VANA,
	SENSOR_VDIG,
	SENSOR_VIO,
	SENSOR_VAF,
	SENSOR_VAF_PWDM,
	SENSOR_CUSTOM_REG1,
	SENSOR_CUSTOM_REG2,
	SENSOR_RESET,
	SENSOR_STANDBY,
	SENSOR_CUSTOM_GPIO1,
	SENSOR_CUSTOM_GPIO2,
	SENSOR_VANA1,
	SENSOR_EXTLDO_VANA,
	SENSOR_EXTLDO_VANA1,
	SENSOR_EXTLDO_VDIG,
	SENSOR_EXTLDO_VIO,
	SENSOR_EXTLDO_VAF,
	SENSOR_SEQ_TYPE_MAX,
};

struct wl2868c_ldomap {
	enum CAM_SENSOR_INDEX sensor_index;
	enum msm_camera_power_seq_type seq_type;
	EXT_SELECT ldo_selected;
};

/*****************************************************************************
 * Function Prototype
 *****************************************************************************/
// extern void wl2868c_gpio_select_state(WL2868C_GPIO_STATE s);
// extern void wl2868c_set_ldo_value(EXT_SELECT ldonum,unsigned int value);
// extern void wl2868c_set_en_ldo(EXT_SELECT ldonum,unsigned int en);
// extern int wl2868c_set_ldo_enable(uint32_t sensor_index, enum msm_camera_power_seq_type seq_type, uint32_t min_voltage, uint32_t max_voltage);
// extern int wl2868c_set_ldo_disable(uint32_t sensor_index, enum msm_camera_power_seq_type seq_type);
// int wl2868c_init_module(void);
// void wl2868c_exit_module(void);

#endif /* _WL2868C_H_*/
