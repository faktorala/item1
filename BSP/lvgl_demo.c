/**
 * @file lvgl_demo.c
 * @brief LVGL 9.4 简单测试界面
 *
 * 不依赖触摸/按键，上电后就能看到：
 *   - 顶部标题 "Hello LVGL 9.4"
 *   - 中间进度条来回滚动（lv_anim + lv_timer_handler 都在正常工作）
 *   - 底部显示 FreeRTOS tick 折算的运行秒数（时基来自 osKernelGetTickCount）
 */

#include "lvgl_demo.h"
#include "lvgl.h"

static void bar_anim_cb(void * var, int32_t v)
{
    lv_bar_set_value((lv_obj_t *)var, v, LV_ANIM_OFF);
}

static void uptime_timer_cb(lv_timer_t * t)
{
    lv_obj_t * label = (lv_obj_t *)lv_timer_get_user_data(t);
    lv_label_set_text_fmt(label, "run: %lu s", (unsigned long)(lv_tick_get() / 1000U));
}

void lvgl_demo_init(void)
{
    lv_obj_t * scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x101820), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    /* 标题 */
    lv_obj_t * title = lv_label_create(scr);
    lv_label_set_text(title, "Hello LVGL 9.4");
    lv_obj_set_style_text_color(title, lv_color_hex(0x00E0FF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 14);

    /* 循环动画的进度条 */
    lv_obj_t * bar = lv_bar_create(scr);
    lv_obj_set_size(bar, 100, 12);
    lv_obj_align(bar, LV_ALIGN_CENTER, 0, 0);
    lv_bar_set_range(bar, 0, 100);
    lv_bar_set_value(bar, 0, LV_ANIM_OFF);

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, bar);
    lv_anim_set_values(&a, 0, 100);
    lv_anim_set_duration(&a, 1200);
    lv_anim_set_reverse_duration(&a, 1200);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_exec_cb(&a, bar_anim_cb);
    lv_anim_start(&a);

    /* 底部运行时间，每 500ms 刷新一次 */
    lv_obj_t * tick_label = lv_label_create(scr);
    lv_label_set_text(tick_label, "run: 0 s");
    lv_obj_set_style_text_color(tick_label, lv_color_white(), 0);
    lv_obj_align(tick_label, LV_ALIGN_BOTTOM_MID, 0, -12);

    lv_timer_create(uptime_timer_cb, 500, tick_label);
}
