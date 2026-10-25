/**
 * @file lv_port_disp.h
 * @brief LVGL 9.4 显示端口（ST7735 128x160，SPI1 + DMA）
 */

#ifndef LV_PORT_DISP_H
#define LV_PORT_DISP_H

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

/* 初始化 LVGL 显示端口 */
void lv_port_disp_init(void);

/* 暂停 / 恢复 LVGL 刷屏（例如切换页面时临时屏蔽） */
void disp_enable_update(void);
void disp_disable_update(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /* LV_PORT_DISP_H */
