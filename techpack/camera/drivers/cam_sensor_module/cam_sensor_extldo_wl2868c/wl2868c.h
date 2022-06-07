#ifndef _WL2868C_H_
#define _WL2868C_H_

#include "../cam_sensor_utils/cam_sensor_cmn_header.h"

#define WL2868C_CHIP_REV_ADDR    0x00
#define WL2868C_CL_ADDR    		0x01
#define WL2868C_DIC_ADDR         0x02

#define WL2868C_LDO1_OUT_ADDR    0x03 //DVDD1
#define WL2868C_LDO2_OUT_ADDR    0x04 //DVDD2
#define WL2868C_LDO3_OUT_ADDR    0x05 //AVDD1
#define WL2868C_LDO4_OUT_ADDR    0x06 //AVDD2
#define WL2868C_LDO5_OUT_ADDR    0x07 //VDDAF
#define WL2868C_LDO6_OUT_ADDR    0x08 //VDDOIS
#define WL2868C_LDO7_OUT_ADDR    0x09 //VDDIO

#define WL2868C_LDO21_SEQ_ADDR   0x0A
#define WL2868C_LDO43_SEQ_ADDR   0x0B
#define WL2868C_LDO65_SEQ_ADDR   0x0C
#define WL2868C_LDO07_SEQ_ADDR   0x0D

#define WL2868C_LDO_EN_ADDR          0x0E  //bit0:LDO1 ~ bit6:LDO7
#define WL2868C_LDO_SEQ_CTRL_ADDR    0x0F

#define WL2868C_PRINT pr_info //printk


typedef enum {
	WL2868C_NONE=-1,
	WL2868C_DVDD1,              //LDO1
	WL2868C_DVDD2,              //LDO2
	WL2868C_AVDD1,              //LDO3
	WL2868C_AVDD2,              //LDO4
	WL2868C_VDDAF,              //LDO5
	WL2868C_VDDOIS,             //LDO6
	WL2868C_VDDIO,              //LDO7
	WL2868C_MAX                 //0x07
} WL2868C_SELECT;


/* DTS state */
typedef enum {
	WL2868C_GPIO_STATE_ENP0,
	WL2868C_GPIO_STATE_ENP1,
	WL2868C_GPIO_STATE_MAX,	/* for array size */
} WL2868C_GPIO_STATE;

enum CAM_SENSOR_INDEX {
    CAMERA_INDEX_MAIN = 0,    //BackMain
//    CAMERA_INDEX_MAIN_DEPTH,
    CAMERA_INDEX_MAIN_WIDE,    //BackWide
    CAMERA_INDEX_FRONT,        //FrontMain
    CAMERA_INDEX_MAIN_MACRO,    //BackMacro
//    CAMERA_INDEX_MAIN_TELE,  //BackWide
    CAMERA_INDEX_MAIN_DEPTH,
    CAMERA_INDEX_MAX_NUM,
};

struct wl2868c_ldomap {
	enum CAM_SENSOR_INDEX sensor_index;
	enum msm_camera_power_seq_type seq_type;
	WL2868C_SELECT ldo_selected;
};

#ifdef OPLUS_FEATURE_CAMERA_COMMON
struct wl2868c {
	struct mutex pwl2868c_mutex;
};
#endif
/*****************************************************************************
 * Function Prototype
 *****************************************************************************/
//extern void wl2868c_gpio_select_state(WL2868C_GPIO_STATE s);
extern void wl2868c_set_ldo_value(WL2868C_SELECT ldonum,unsigned int value);
extern void wl2868c_set_en_ldo(WL2868C_SELECT ldonum,unsigned int en);
extern int wl2868c_set_ldo_enable(uint32_t sensor_index, enum msm_camera_power_seq_type seq_type, uint32_t min_voltage, uint32_t max_voltage);
extern int wl2868c_set_ldo_disable(uint32_t sensor_index, enum msm_camera_power_seq_type seq_type);
int wl2868c_init_module(void);
void wl2868c_exit_module(void);

#endif /* _WL2868C_H_*/