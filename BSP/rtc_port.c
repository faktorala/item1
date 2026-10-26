/**
 * @file rtc_port.c
 * @brief 读取 STM32F4 内部 RTC 的时间，并刷新 LVGL 的 hour / min / sec 标签
 *
 * 说明：
 *   - RTC 由 CubeMX 的 MX_RTC_Init() 初始化（LSE 32.768kHz，24 小时制）
 *   - HAL_RTC_GetTime() 读 TR 寄存器时会锁住日历影子寄存器，
 *     必须紧接着调用一次 HAL_RTC_GetDate() 读 DR 才能解锁，
 *     否则时间会一直停在第一次读到的值（这是 HAL 的固定用法，不能省）
 *   - 刷新由 LVGL 定时器驱动，运行在 lv_timer_handler() 所在的任务里，
 *     不在中断里碰 LVGL
 */

#include "rtc_port.h"
#include "rtc.h"

/* 检查秒变化的周期：200ms -> 秒数最多延迟 200ms 显示，肉眼看就是连续走秒 */
#define RTC_PORT_REFRESH_MS     200

static lv_obj_t * s_hour_label;
static lv_obj_t * s_min_label;
static lv_obj_t * s_sec_label;

static uint8_t s_last_sec = 0xFF;   /* 0xFF 保证第一次一定刷新 */

static void rtc_port_timer_cb(lv_timer_t * timer)
{
    (void)timer;
    RtcPort_Refresh();
}

void RtcPort_Refresh(void)
{
    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};

    if(HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK) {
        return;
    }

    /* 必须读一次日期来解锁影子寄存器，否则秒不会往前走 */
    if(HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK) {
        return;
    }

    /* 秒没变就不重绘标签 */
    if(sTime.Seconds == s_last_sec) {
        return;
    }
    s_last_sec = sTime.Seconds;

    if(s_hour_label != NULL) {
        lv_label_set_text_fmt(s_hour_label, "%02u", (unsigned)sTime.Hours);
    }
    if(s_min_label != NULL) {
        lv_label_set_text_fmt(s_min_label, "%02u", (unsigned)sTime.Minutes);
    }
    if(s_sec_label != NULL) {
        lv_label_set_text_fmt(s_sec_label, "%02u", (unsigned)sTime.Seconds);
    }
}

void RtcPort_Init(lv_obj_t * hour_label, lv_obj_t * min_label, lv_obj_t * sec_label)
{
    s_hour_label = hour_label;
    s_min_label  = min_label;
    s_sec_label  = sec_label;
    s_last_sec   = 0xFF;

    /* 上电先立刻显示一次，不用等第一个定时器周期 */
    RtcPort_Refresh();

    lv_timer_create(rtc_port_timer_cb, RTC_PORT_REFRESH_MS, NULL);
}
