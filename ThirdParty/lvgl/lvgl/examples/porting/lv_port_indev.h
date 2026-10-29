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

/* 软键盘模式：焦点在 lv_keyboard 上时打开。
 * KEY1/KEY2 从 NEXT/PREV 改为 LEFT/RIGHT，用来在键盘按键之间移动；
 * KEY3 仍然是 ENTER（输入选中字符），KEY4 仍然是 ESC。 */
void lv_port_indev_set_keyboard_mode(bool en);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /* LV_PORT_INDEV_H */
