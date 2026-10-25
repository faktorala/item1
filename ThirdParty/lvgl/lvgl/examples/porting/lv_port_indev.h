/**
 * @file lv_port_indev.h
 * @brief LVGL 9.4 输入端口（4 个按键，KEYPAD 方式）
 */

#ifndef LV_PORT_INDEV_H
#define LV_PORT_INDEV_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "lvgl.h"

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/* 初始化按键输入设备，并创建一个默认 group */
void lv_port_indev_init(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /* LV_PORT_INDEV_H */
