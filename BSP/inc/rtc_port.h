#ifndef __RTC_PORT_H
#define __RTC_PORT_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 绑定 hour / min / sec 三个标签，并启动周期刷新的 LVGL 定时器。
 * 必须在 lv_init() 和界面创建完成之后调用。
 * 传 NULL 的标签会被跳过。 */
void RtcPort_Init(lv_obj_t * hour_label, lv_obj_t * min_label, lv_obj_t * sec_label);

/* 立即从 RTC 读一次并刷新标签（一般由内部定时器调用，也可以手动调用） */
void RtcPort_Refresh(void);

#ifdef __cplusplus
}
#endif

#endif /* __RTC_PORT_H */
