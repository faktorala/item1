/*
* Copyright 2026 NXP
* NXP Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/


/*********************
 *      INCLUDES
 *********************/
#include <stdio.h>
#include "lvgl.h"
#include "gg_utils.h"
#include "custom.h"
#include "rtc_port.h"
#include "game_2048.h"
#include "lv_port_indev.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void screen_loaded_cb(lv_event_t * e);
static void group_keep_screen(lv_obj_t * scr);
static void group_add_focusable(lv_obj_t * parent);
static void focus_first_on_screen(lv_obj_t * scr);
static void kb_focus_cb(lv_event_t * e);
static void kb_ready_cb(lv_event_t * e);
static void kb_cancel_cb(lv_event_t * e);

/**********************
 *  STATIC VARIABLES
 **********************/

/* 把 group 里不属于 scr 的对象移除。
 * 注意：不要用 LV_OBJ_FLAG_HIDDEN 去隐藏屏幕，
 * lv_obj_remove_flag() 会对 screen 的 parent(NULL) 调 lv_obj_mark_layout_as_dirty()，
 * 在 LVGL 9.4 上会直接 HardFault。 */
static void group_keep_screen(lv_obj_t * scr)
{
    lv_group_t * g = lv_group_get_default();
    if(g == NULL) return;

    uint32_t i = 0;
    while(i < lv_group_get_obj_count(g)) {
        lv_obj_t * obj = lv_group_get_obj_by_index(g, i);
        if(obj != NULL && lv_obj_get_screen(obj) != scr) {
            lv_group_remove_obj(obj);
            i = 0;              /* 链表变了，从头再扫 */
        }
        else {
            i++;
        }
    }
}

/* 递归把 scr 上所有可聚焦对象加回 group（lv_group_add_obj 会检查重复） */
static void group_add_focusable(lv_obj_t * parent)
{
    lv_group_t * g = lv_group_get_default();
    if(g == NULL) return;

    uint32_t n = lv_obj_get_child_count(parent);
    for(uint32_t i = 0; i < n; i++) {
        lv_obj_t * child = lv_obj_get_child(parent, (int32_t)i);
        if(child == NULL) continue;

        if(lv_obj_is_group_def(child) && lv_obj_get_group(child) == NULL) {
            lv_group_add_obj(g, child);
        }
        group_add_focusable(child);
    }
}

/* 把焦点放到指定屏幕上的第一个可聚焦对象 */
static void focus_first_on_screen(lv_obj_t * scr)
{
    lv_group_t * g = lv_group_get_default();
    if(g == NULL) return;

    uint32_t cnt = lv_group_get_obj_count(g);
    for(uint32_t i = 0; i < cnt; i++) {
        lv_obj_t * obj = lv_group_get_obj_by_index(g, i);
        if(obj == NULL) continue;
        if(lv_obj_get_screen(obj) != scr) continue;

        lv_group_focus_obj(obj);
        return;
    }
}

/**
 * 屏幕切换完成后：只保留新屏幕上的对象在 group 里，
 * 这样 keypad 的 NEXT/PREV 不会跑到看不见的屏幕上。
 */
static void screen_loaded_cb(lv_event_t * e)
{
    gg_ui_t * ui = (gg_ui_t *)lv_event_get_user_data(e);

    if(lv_event_get_code(e) != LV_EVENT_SCREEN_LOADED) return;

    lv_obj_t * scr = lv_event_get_target(e);

    group_keep_screen(scr);
    group_add_focusable(scr);

    if((ui != NULL) && (scr == ui->screen_3.screen)) {
        /* 没有触摸屏：进入 screen_3 就直接弹出软键盘并绑定文本框 */
        lv_obj_remove_flag(ui->screen_3.keyboard_1, LV_OBJ_FLAG_HIDDEN);
        lv_keyboard_set_textarea(ui->screen_3.keyboard_1, ui->screen_3.textarea_1);
        lv_group_focus_obj(ui->screen_3.keyboard_1);
        lv_port_indev_set_keyboard_mode(true);
    }
    else {
        lv_port_indev_set_keyboard_mode(false);
        focus_first_on_screen(scr);
    }
}

/* 软键盘获得/失去焦点时切换 KEY1/KEY2 的映射 */
static void kb_focus_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_FOCUSED) {
        lv_port_indev_set_keyboard_mode(true);
    }
    else if(code == LV_EVENT_DEFOCUSED) {
        lv_port_indev_set_keyboard_mode(false);
    }
}

/* 软键盘上的 OK 键：键盘已被生成的代码隐藏，这里把焦点交回按钮 */
static void kb_ready_cb(lv_event_t * e)
{
    gg_ui_t * ui = (gg_ui_t *)lv_event_get_user_data(e);

    if(lv_event_get_code(e) != LV_EVENT_READY) return;

    lv_port_indev_set_keyboard_mode(false);

    lv_group_t * g = lv_group_get_default();
    if((g != NULL) && (ui != NULL)) {
        lv_group_focus_obj(ui->screen_3.container_2_button_1);
    }
}

/* KEY4(ESC)：收起键盘并返回 screen_2，复用 perv 按钮原来的返回事件 */
static void kb_cancel_cb(lv_event_t * e)
{
    gg_ui_t * ui = (gg_ui_t *)lv_event_get_user_data(e);

    if(lv_event_get_code(e) != LV_EVENT_CANCEL) return;
    if(ui == NULL) return;

    lv_port_indev_set_keyboard_mode(false);
    lv_obj_add_flag(ui->screen_3.keyboard_1, LV_OBJ_FLAG_HIDDEN);
    lv_obj_send_event(ui->screen_3.container_2_button_1, LV_EVENT_RELEASED, NULL);
}

/**
 * Create a demo application
 */

void custom_init(gg_ui_t *ui)
{
    /* 创建主界面并加载 screen */
    setup_ui(ui);

    /* 预创建 screen_1：生成的代码是第一次点击按钮时才创建它，
     * 在这里提前创建，才能给它挂上 SCREEN_LOADED 回调。 */
    if(ui->screen_1.screen == NULL) {
        setup_screen_1(ui);
    }

    /* screen_2 也提前创建，方便挂回调和初始化 2048 */
    if(ui->screen_2.screen == NULL) {
        setup_screen_2(ui);
    }

    /* screen_3 提前创建，才能给软键盘挂焦点/OK/ESC 回调 */
    if(ui->screen_3.screen == NULL) {
        setup_screen_3(ui);
    }

    lv_obj_add_event_cb(ui->screen.screen, screen_loaded_cb, LV_EVENT_SCREEN_LOADED, ui);
    lv_obj_add_event_cb(ui->screen_1.screen, screen_loaded_cb, LV_EVENT_SCREEN_LOADED, ui);
    lv_obj_add_event_cb(ui->screen_2.screen, screen_loaded_cb, LV_EVENT_SCREEN_LOADED, ui);
    lv_obj_add_event_cb(ui->screen_3.screen, screen_loaded_cb, LV_EVENT_SCREEN_LOADED, ui);

    /* 软键盘：焦点切换时改键值映射；OK 把焦点交回按钮；ESC 返回 screen_2 */
    lv_obj_add_event_cb(ui->screen_3.keyboard_1, kb_focus_cb, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->screen_3.keyboard_1, kb_ready_cb, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->screen_3.keyboard_1, kb_cancel_cb, LV_EVENT_ALL, ui);

    /* 启动时 screen_1 的控件已经自动进了默认 group，先清掉 */
    group_keep_screen(ui->screen.screen);
    focus_first_on_screen(ui->screen.screen);

    /* 把 RTC 时间接到主屏的 hour / min / sec 三个标签上，之后自动走时 */
    RtcPort_Init(ui->screen.container_2_hour,
                 ui->screen.container_2_min,
                 ui->screen.container_2_sec);

    /* screen_2 上的 2048：创建隐藏的游戏界面并绑定 begin 按钮 */
    Game2048_Init(ui);

    /* 这里可以继续添加你自己的初始化代码 */
}
