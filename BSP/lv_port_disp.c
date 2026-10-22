/**
 * @file lv_port_disp.c
 * @brief LVGL 9.4 display port for ST7735 128x160 over SPI1 + DMA
 *
 * - 像素格式：RGB565（LV_COLOR_DEPTH 16）
 * - 双缓冲，每次刷新 10 行，DMA 发送完成后才通知 LVGL 释放缓冲
 * - ST7735 要求高字节在前，因此发送前做一次 RGB565 字节交换
 */

#include "lv_port_disp.h"
#include <stdbool.h>
#include "lcd.h"
#include "spi.h"

#define MY_DISP_HOR_RES     128
#define MY_DISP_VER_RES     160

#define BYTE_PER_PIXEL      (LV_COLOR_FORMAT_GET_SIZE(LV_COLOR_FORMAT_RGB565))
#define DISP_BUF_LINES      10

static lv_display_t * s_disp;
static volatile bool  s_flush_enabled = true;

static void disp_init(void);
static void disp_flush(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map);

void lv_port_disp_init(void)
{
    disp_init();

    s_disp = lv_display_create(MY_DISP_HOR_RES, MY_DISP_VER_RES);
    lv_display_set_flush_cb(s_disp, disp_flush);

    /* 双缓冲 + 局部刷新：一个缓冲在刷屏时，另一个可以继续渲染 */
    LV_ATTRIBUTE_MEM_ALIGN
    static uint8_t buf_1[MY_DISP_HOR_RES * DISP_BUF_LINES * BYTE_PER_PIXEL];

    LV_ATTRIBUTE_MEM_ALIGN
    static uint8_t buf_2[MY_DISP_HOR_RES * DISP_BUF_LINES * BYTE_PER_PIXEL];

    lv_display_set_buffers(s_disp, buf_1, buf_2, sizeof(buf_1),
                           LV_DISPLAY_RENDER_MODE_PARTIAL);
}

void disp_enable_update(void)
{
    s_flush_enabled = true;
}

void disp_disable_update(void)
{
    s_flush_enabled = false;
}

/* 初始化屏幕本身（复位、ST7735 寄存器序列） */
static void disp_init(void)
{
    LCD_Init();
}

/**
 * @brief LVGL 刷新回调
 * @note  用 DMA 异步发送，DMA 完成中断里再调用 lv_display_flush_ready()，
 *        千万不要在这里就通知 LVGL，否则缓冲会被提前复用导致花屏。
 */
static void disp_flush(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map)
{
    if(!s_flush_enabled) {
        lv_display_flush_ready(disp);
        return;
    }

    uint32_t w   = (uint32_t)(area->x2 - area->x1 + 1);
    uint32_t h   = (uint32_t)(area->y2 - area->y1 + 1);
    uint32_t len = w * h * BYTE_PER_PIXEL;

    if(len == 0U) {
        lv_display_flush_ready(disp);
        return;
    }

    /* LVGL 的 RGB565 缓冲是小端，ST7735 走 SPI 需要高字节在前 */
    lv_draw_sw_rgb565_swap(px_map, w * h);

    /* 设置显示窗口，然后连续写入像素 */
    LCD_Address_Set(area->x1, area->y1, area->x2, area->y2);

    s_disp = disp;
    LCD_CS_Clr();

    if(HAL_SPI_Transmit_DMA(&hspi1, px_map, (uint16_t)len) != HAL_OK) {
        /* DMA 启动失败：退化成阻塞发送，保证这一帧不丢 */
        HAL_SPI_Transmit(&hspi1, px_map, (uint16_t)len, HAL_MAX_DELAY);
        LCD_CS_Set();
        lv_display_flush_ready(disp);
    }
}

/* SPI1 TX DMA 完成：此时缓冲才真正空闲 */
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef * hspi)
{
    if(hspi->Instance == SPI1) {
        LCD_CS_Set();
        lv_display_flush_ready(s_disp);
    }
}
