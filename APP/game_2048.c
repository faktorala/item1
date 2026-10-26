/**
 * @file game_2048.c
 * @brief screen_2 上的 2048 小游戏（128x160）
 *
 * 按键（游戏运行时）：
 *   KEY1 = 左    KEY2 = 右    KEY3 = 上    KEY4 = 下
 *   长按 KEY4 >= 1.2s = 退出游戏，回到 screen_2 的 begin/return 菜单
 *   游戏结束 / 达成 2048 后：短按 KEY3 = 重新开始
 *
 * 实现说明：
 *   游戏运行时先调用 lv_port_indev_set_enabled(false) 关掉 LVGL 的 keypad 事件，
 *   由本模块自己的 10ms LVGL 定时器轮询 get_press_id()。
 *   这样可以同时支持四个方向键和“长按退出”，
 *   不受 LVGL keypad 固定映射（NEXT/PREV/ENTER/ESC）的限制。
 *   退出游戏后再把 keypad 恢复。
 */

#include "game_2048.h"
#include "key.h"
#include "lv_port_indev.h"
#include <string.h>

LV_FONT_DECLARE(lv_font_montserratMedium_10);

/* ------------------------- 参数 ------------------------- */
#define GRID_N              4
#define SCAN_MS             10      /* 按键扫描周期 */
#define DEBOUNCE_TICKS      2       /* 2 x 10ms = 20ms 消抖 */
#define EXIT_HOLD_MS        1200    /* KEY4 长按退出时间 */

#define TILE_SIZE           25
#define TILE_GAP            1
#define BOARD_PAD           2
#define BOARD_SIZE          (GRID_N * TILE_SIZE + (GRID_N - 1) * TILE_GAP + 2 * BOARD_PAD)
#define BOARD_X             ((128 - BOARD_SIZE) / 2)
#define BOARD_Y             18

/* ------------------------- 状态 ------------------------- */
static gg_ui_t * s_ui;

static lv_obj_t * s_root;           /* 整个游戏界面，默认隐藏 */
static lv_obj_t * s_board;
static lv_obj_t * s_score_label;
static lv_obj_t * s_hint_label;
static lv_obj_t * s_cells[GRID_N][GRID_N];
static lv_obj_t * s_cell_labels[GRID_N][GRID_N];

static uint16_t s_grid[GRID_N][GRID_N];
static uint32_t s_score;
static bool     s_running;
static bool     s_game_over;
static bool     s_won;

static uint32_t s_rng = 0x12345678u;

/* 按键消抖 / 边沿检测 */
static uint8_t  s_key_raw;
static uint8_t  s_key_stable;
static uint8_t  s_key_cnt;
static uint32_t s_key_down_ms;

/* ------------------------- 内部函数声明 ------------------------- */
static void Game2048_Start(void);
static void Game2048_Stop(void);
static void game_render(void);
static void game_timer_cb(lv_timer_t * timer);
static void begin_btn_cb(lv_event_t * e);

/* ------------------------- 小工具 ------------------------- */
static uint32_t rng_next(void)
{
    s_rng = s_rng * 1103515245u + 12345u;
    return (s_rng >> 16) & 0x7FFFu;
}

static lv_color_t tile_color(uint16_t v)
{
    switch(v) {
        case 0:    return lv_color_hex(0x1b2430);
        case 2:    return lv_color_hex(0xeee4da);
        case 4:    return lv_color_hex(0xede0c8);
        case 8:    return lv_color_hex(0xf2b179);
        case 16:   return lv_color_hex(0xf59563);
        case 32:   return lv_color_hex(0xf67c5f);
        case 64:   return lv_color_hex(0xf65e3b);
        case 128:  return lv_color_hex(0xedcf72);
        case 256:  return lv_color_hex(0xedcc61);
        case 512:  return lv_color_hex(0xedc850);
        case 1024: return lv_color_hex(0xedc53f);
        case 2048: return lv_color_hex(0xedc22e);
        default:   return lv_color_hex(0x3c3a32);
    }
}

/* ------------------------- game core ------------------------- */

/* 把一行 4 个数向“左”滑动并合并（行内正向处理） */
static bool slide_line(uint16_t line[GRID_N], uint32_t * score)
{
    uint16_t tmp[GRID_N];
    uint16_t out[GRID_N] = {0, 0, 0, 0};
    bool changed = false;
    int i;
    int n = 0;
    int m = 0;

    for(i = 0; i < GRID_N; i++) {
        if(line[i] != 0) {
            tmp[n++] = line[i];
        }
    }

    for(i = 0; i < n; i++) {
        if((i + 1 < n) && (tmp[i] == tmp[i + 1])) {
            uint32_t v = (uint32_t)tmp[i] * 2u;
            out[m++] = (uint16_t)v;
            *score += v;
            if(v == 2048u) {
                s_won = true;
            }
            i++;    /* 跳过被合并掉的那个 */
        }
        else {
            out[m++] = tmp[i];
        }
    }

    for(i = 0; i < GRID_N; i++) {
        if(line[i] != out[i]) {
            changed = true;
        }
        line[i] = out[i];
    }

    return changed;
}

static bool move_left(void)
{
    bool changed = false;
    uint32_t gain = 0;
    int r;

    for(r = 0; r < GRID_N; r++) {
        if(slide_line(s_grid[r], &gain)) {
            changed = true;
        }
    }
    s_score += gain;
    return changed;
}

static bool move_right(void)
{
    bool changed = false;
    uint32_t gain = 0;
    int r, c;

    for(r = 0; r < GRID_N; r++) {
        uint16_t line[GRID_N];

        for(c = 0; c < GRID_N; c++) {
            line[c] = s_grid[r][GRID_N - 1 - c];
        }
        if(slide_line(line, &gain)) {
            changed = true;
        }
        for(c = 0; c < GRID_N; c++) {
            s_grid[r][GRID_N - 1 - c] = line[c];
        }
    }
    s_score += gain;
    return changed;
}

static bool move_up(void)
{
    bool changed = false;
    uint32_t gain = 0;
    int r, c;

    for(c = 0; c < GRID_N; c++) {
        uint16_t line[GRID_N];

        for(r = 0; r < GRID_N; r++) {
            line[r] = s_grid[r][c];
        }
        if(slide_line(line, &gain)) {
            changed = true;
        }
        for(r = 0; r < GRID_N; r++) {
            s_grid[r][c] = line[r];
        }
    }
    s_score += gain;
    return changed;
}

static bool move_down(void)
{
    bool changed = false;
    uint32_t gain = 0;
    int r, c;

    for(c = 0; c < GRID_N; c++) {
        uint16_t line[GRID_N];

        for(r = 0; r < GRID_N; r++) {
            line[r] = s_grid[GRID_N - 1 - r][c];
        }
        if(slide_line(line, &gain)) {
            changed = true;
        }
        for(r = 0; r < GRID_N; r++) {
            s_grid[GRID_N - 1 - r][c] = line[r];
        }
    }
    s_score += gain;
    return changed;
}

static void spawn_tile(void)
{
    int er[GRID_N * GRID_N];
    int ec[GRID_N * GRID_N];
    int n = 0;
    int r, c, k;

    for(r = 0; r < GRID_N; r++) {
        for(c = 0; c < GRID_N; c++) {
            if(s_grid[r][c] == 0) {
                er[n] = r;
                ec[n] = c;
                n++;
            }
        }
    }

    if(n == 0) return;

    k = (int)(rng_next() % (uint32_t)n);
    s_grid[er[k]][ec[k]] = (rng_next() % 10u == 0u) ? 4 : 2;
}

static bool can_move(void)
{
    int r, c;

    for(r = 0; r < GRID_N; r++) {
        for(c = 0; c < GRID_N; c++) {
            if(s_grid[r][c] == 0) return true;
            if((c + 1 < GRID_N) && (s_grid[r][c] == s_grid[r][c + 1])) return true;
            if((r + 1 < GRID_N) && (s_grid[r][c] == s_grid[r + 1][c])) return true;
        }
    }
    return false;
}

static void game_render(void)
{
    int r, c;

    for(r = 0; r < GRID_N; r++) {
        for(c = 0; c < GRID_N; c++) {
            uint16_t v = s_grid[r][c];

            if(v == 0) {
                lv_label_set_text(s_cell_labels[r][c], "");
            }
            else {
                lv_label_set_text_fmt(s_cell_labels[r][c], "%u", (unsigned)v);
            }

            lv_obj_set_style_bg_color(s_cells[r][c], tile_color(v), 0);
            lv_obj_set_style_text_color(s_cell_labels[r][c],
                                        (v <= 4) ? lv_color_hex(0x1a1a1a) : lv_color_white(), 0);
        }
    }

    if(s_game_over) {
        lv_label_set_text(s_score_label, "GAME OVER");
    }
    else if(s_won) {
        lv_label_set_text(s_score_label, "YOU WIN");
    }
    else {
        lv_label_set_text_fmt(s_score_label, "SCORE %lu", (unsigned long)s_score);
    }
}

static void reset_key_state(void)
{
    s_key_raw = 0;
    s_key_stable = 0;
    s_key_cnt = 0;
    s_key_down_ms = 0;
}

static void Game2048_Start(void)
{
    s_running = true;
    s_game_over = false;
    s_won = false;
    s_score = 0;
    memset(s_grid, 0, sizeof(s_grid));

    s_rng = lv_tick_get() * 1103515245u + 12345u;
    if(s_rng == 0) s_rng = 1;

    spawn_tile();
    spawn_tile();

    /* 隐藏 begin/return 菜单和标题，显示游戏界面 */
    lv_obj_add_flag(s_ui->screen_2.container_1, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s_ui->screen_2.button_1, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s_ui->screen_2.button_1_copy, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(s_root, LV_OBJ_FLAG_HIDDEN);

    reset_key_state();
    lv_port_indev_set_enabled(false);   /* 游戏自己扫键，关掉 LVGL keypad */

    game_render();
}

static void Game2048_Stop(void)
{
    s_running = false;

    lv_obj_add_flag(s_root, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(s_ui->screen_2.container_1, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(s_ui->screen_2.button_1, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(s_ui->screen_2.button_1_copy, LV_OBJ_FLAG_HIDDEN);

    lv_port_indev_set_enabled(true);    /* 恢复 LVGL keypad */

    lv_group_t * g = lv_group_get_default();
    if(g != NULL) {
        lv_group_focus_obj(s_ui->screen_2.button_1);
    }
}

/* 10ms 扫描一次物理按键：消抖 + 按下/松开边沿 + 长按 */
static void game_timer_cb(lv_timer_t * timer)
{
    uint8_t raw;
    uint8_t prev;
    uint32_t held;

    (void)timer;

    if(!s_running) return;

    raw = get_press_id();

    if(raw != s_key_raw) {
        s_key_raw = raw;
        s_key_cnt = 0;
    }
    else if(s_key_cnt < DEBOUNCE_TICKS) {
        s_key_cnt++;
    }

    if(s_key_cnt < DEBOUNCE_TICKS) return;
    if(raw == s_key_stable) return;

    prev = s_key_stable;
    s_key_stable = raw;

    /* 按下 */
    if(prev == 0) {
        s_key_down_ms = lv_tick_get();
        return;
    }

    /* 松开 */
    held = lv_tick_get() - s_key_down_ms;

    /* 长按 KEY4 退出 */
    if((prev == 4) && (held >= EXIT_HOLD_MS)) {
        Game2048_Stop();
        return;
    }

    /* 结束/胜利界面：KEY3 重开 */
    if(s_game_over) {
        if(prev == 3) {
            Game2048_Start();
        }
        return;
    }

    {
        bool moved = false;

        switch(prev) {
            case 1: moved = move_left();  break;
            case 2: moved = move_right(); break;
            case 3: moved = move_up();    break;
            case 4: moved = move_down();  break;
            default: break;
        }

        if(!moved) return;

        spawn_tile();
        if(!can_move()) {
            s_game_over = true;
        }
        game_render();
    }
}

static void begin_btn_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) != LV_EVENT_CLICKED) return;

    Game2048_Start();
}

/* ------------------------- 界面创建 ------------------------- */
static void create_game_ui(void)
{
    int r, c;

    s_root = lv_obj_create(s_ui->screen_2.screen);
    lv_obj_set_size(s_root, 128, 160);
    lv_obj_set_pos(s_root, 0, 0);
    lv_obj_set_style_bg_color(s_root, lv_color_hex(0x101820), 0);
    lv_obj_set_style_bg_opa(s_root, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_root, 0, 0);
    lv_obj_set_style_radius(s_root, 0, 0);
    lv_obj_set_style_pad_all(s_root, 0, 0);
    lv_obj_remove_flag(s_root, LV_OBJ_FLAG_SCROLLABLE);

    s_score_label = lv_label_create(s_root);
    lv_obj_set_width(s_score_label, 128);
    lv_obj_set_style_text_align(s_score_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(s_score_label, lv_color_white(), 0);
    lv_obj_set_pos(s_score_label, 0, 2);
    lv_label_set_text(s_score_label, "SCORE 0");

    s_board = lv_obj_create(s_root);
    lv_obj_set_size(s_board, BOARD_SIZE, BOARD_SIZE);
    lv_obj_set_pos(s_board, BOARD_X, BOARD_Y);
    lv_obj_set_style_bg_color(s_board, lv_color_hex(0x0b0f14), 0);
    lv_obj_set_style_bg_opa(s_board, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_board, 0, 0);
    lv_obj_set_style_radius(s_board, 4, 0);
    lv_obj_set_style_pad_all(s_board, 0, 0);
    lv_obj_remove_flag(s_board, LV_OBJ_FLAG_SCROLLABLE);

    for(r = 0; r < GRID_N; r++) {
        for(c = 0; c < GRID_N; c++) {
            lv_obj_t * cell = lv_obj_create(s_board);

            lv_obj_set_size(cell, TILE_SIZE, TILE_SIZE);
            lv_obj_set_pos(cell,
                           BOARD_PAD + c * (TILE_SIZE + TILE_GAP),
                           BOARD_PAD + r * (TILE_SIZE + TILE_GAP));
            lv_obj_remove_flag(cell, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_radius(cell, 3, 0);
            lv_obj_set_style_border_width(cell, 0, 0);
            lv_obj_set_style_pad_all(cell, 0, 0);
            lv_obj_set_style_bg_color(cell, tile_color(0), 0);

            lv_obj_t * lab = lv_label_create(cell);
            lv_obj_set_style_text_font(lab, &lv_font_montserratMedium_10, 0);
            lv_obj_center(lab);

            s_cells[r][c] = cell;
            s_cell_labels[r][c] = lab;
        }
    }

    s_hint_label = lv_label_create(s_root);
    lv_obj_set_width(s_hint_label, 128);
    lv_obj_set_style_text_align(s_hint_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(s_hint_label, lv_color_hex(0x9fb3c8), 0);
    lv_obj_set_pos(s_hint_label, 0, 126);
    lv_label_set_long_mode(s_hint_label, LV_LABEL_LONG_MODE_WRAP);
    lv_label_set_text(s_hint_label, "K1:L  K2:R  K3:U  K4:D\nHOLD K4: EXIT");

    lv_obj_add_flag(s_root, LV_OBJ_FLAG_HIDDEN);
}

void Game2048_Init(gg_ui_t * ui)
{
    if(ui == NULL || ui->screen_2.screen == NULL) return;

    s_ui = ui;

    create_game_ui();

    lv_obj_add_event_cb(ui->screen_2.button_1, begin_btn_cb, LV_EVENT_CLICKED, NULL);

    lv_timer_create(game_timer_cb, SCAN_MS, NULL);
}
