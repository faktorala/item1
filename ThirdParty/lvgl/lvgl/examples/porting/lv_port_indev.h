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

/* 临时打开/关闭 keypad 的 LVGL 事件处理。
 * 例如 2048 游戏需要自己轮询四个方向键时，先关掉，退出游戏时再打开。 */
void lv_port_indev_set_enabled(bool en);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /* LV_PORT_INDEV_H */
