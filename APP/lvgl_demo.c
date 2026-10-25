/**
 * @file lvgl_demo.c
 * @brief LVGL 9.4 按键测试界面
 *
 * 硬件：4 个按键（KEYPAD 方式，见 lv_port_indev.c）
 *   K1 -> 焦点下一个    K2 -> 焦点上一个
 *   K3 -> 确认/点击     K4 -> 取消
 *
 * 验证点：
 *   1. 焦点框在 3 个按钮之间移动
 *   2. K3 短按触发 LV_EVENT_CLICKED，长按触发 LV_EVENT_LONG_PRESSED
 *   3. K4 触发 LV_EVENT_CANCEL
 *   4. CHECK 按钮可以切换选中状态
 *
 * 前提：lv_port_indev_init() 必须在本函数之前调用（它创建并设置了默认 group），
 *       这样 lv_button_create() 创建的对象才会自动加入 group。
 */

#include "lvgl_demo.h"
#include <stdbool.h>
#include "lvgl.h"

static lv_obj_t * s_status;
static uint32_t s_clicks;

static void btn_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * btn = (lv_obj_t *)lv_event_get_target(e);
    const char * name = (const char *)lv_event_get_user_data(e);

    switch(code) {
        case LV_EVENT_FOCUSED:
            lv_label_set_text_fmt(s_status, "focus: %s", name);
            break;

        case LV_EVENT_CLICKED:
            s_clicks++;
            lv_label_set_text_fmt(s_status, "%s%s  n=%lu", name,
                                  lv_obj_has_state(btn, LV_STATE_CHECKED) ? " ON" : "",
                                  (unsigned long)s_clicks);
            break;

        case LV_EVENT_LONG_PRESSED:
            lv_label_set_text_fmt(s_status, "%s LONG", name);
            break;

        case LV_EVENT_CANCEL:
            lv_label_set_text_fmt(s_status, "%s ESC", name);
            break;

        default:
            break;
    }
}

static lv_obj_t * create_test_button(lv_obj_t * parent, const char * text,
                                     int32_t y, const char * name, bool checkable)
{
    lv_obj_t * btn = lv_button_create(parent);
    lv_obj_set_size(btn, 104, 26);
    lv_obj_align(btn, LV_ALIGN_TOP_MID, 0, y);

    lv_obj_t * label = lv_label_create(btn);
    lv_label_set_text(label, text);
    lv_obj_center(label);

    if(checkable) {
        lv_obj_add_flag(btn, LV_OBJ_FLAG_CHECKABLE);
    }

    /* 按钮类创建时已自动加入默认 group，这里只挂事件 */
    lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_ALL, (void *)name);
    return btn;
}

void lvgl_demo_init(void)
{
    lv_obj_t * scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x101820), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    /* 标题 */
    lv_obj_t * title = lv_label_create(scr);
    lv_label_set_text(title, "KEYPAD TEST");
    lv_obj_set_style_text_color(title, lv_color_hex(0x00E0FF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 8);

    /* 3 个可聚焦按钮 */
    create_test_button(scr, "BTN A", 32, "A", false);
    create_test_button(scr, "BTN B", 64, "B", false);
    create_test_button(scr, "CHECK", 96, "C", true);

    /* 状态栏 */
    s_status = lv_label_create(scr);
    lv_label_set_text(s_status, "K1/K2: move\nK3: ok  K4: esc");
    lv_label_set_long_mode(s_status, LV_LABEL_LONG_MODE_WRAP);
    lv_obj_set_width(s_status, 120);
    lv_obj_set_style_text_align(s_status, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(s_status, lv_color_white(), 0);
    lv_obj_align(s_status, LV_ALIGN_BOTTOM_MID, 0, -4);
}
