/**
 * @file lv_port_disp_template.c
 *
 */

/*Copy this file as "lv_port_disp.c" and set this value to "1" to enable content*/
#if 1

/*********************
 *      INCLUDES
 *********************/
#include "lv_port_disp.h"
#include <stdbool.h>
#include "main.h"
#include "spi.h"
#include "lcd.h"
/*********************
 *      DEFINES
 *********************/
#ifndef MY_DISP_HOR_RES
    #warning Please define or replace the macro MY_DISP_HOR_RES with the actual screen width, default value 320 is used for now.
    #define MY_DISP_HOR_RES    128
#endif

#ifndef MY_DISP_VER_RES
    #warning Please define or replace the macro MY_DISP_VER_RES with the actual screen height, default value 240 is used for now.
    #define MY_DISP_VER_RES    160
#endif

#define BYTE_PER_PIXEL (LV_COLOR_FORMAT_GET_SIZE(LV_COLOR_FORMAT_RGB565)) /*will be 2 for RGB565 */

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void disp_init(void);

static void disp_flush(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_port_disp_init(void)
{
    /*-------------------------
     * Initialize your display
     * -----------------------*/
    disp_init();

    /*------------------------------------
     * Create a display and set a flush_cb
     * -----------------------------------*/
    lv_display_t * disp = lv_display_create(MY_DISP_HOR_RES, MY_DISP_VER_RES);
    lv_display_set_flush_cb(disp, disp_flush);

    // /* Example 1
    //  * One buffer for partial rendering*/
    // LV_ATTRIBUTE_MEM_ALIGN
    // static uint8_t buf_1_1[MY_DISP_HOR_RES * 10 * BYTE_PER_PIXEL];            /*A buffer for 10 rows*/
    // lv_display_set_buffers(disp, buf_1_1, NULL, sizeof(buf_1_1), LV_DISPLAY_RENDER_MODE_PARTIAL);

    /* Example 2
     * Two buffers for partial rendering
     * In flush_cb DMA or similar hardware should be used to update the display in the background.*/
    LV_ATTRIBUTE_MEM_ALIGN
    static uint8_t buf_2_1[MY_DISP_HOR_RES * 10 * BYTE_PER_PIXEL];

    LV_ATTRIBUTE_MEM_ALIGN
    static uint8_t buf_2_2[MY_DISP_HOR_RES * 10 * BYTE_PER_PIXEL];
    lv_display_set_buffers(disp, buf_2_1, buf_2_2, sizeof(buf_2_1), LV_DISPLAY_RENDER_MODE_PARTIAL);

    // /* Example 3
    //  * Two buffers screen sized buffer for double buffering.
    //  * Both LV_DISPLAY_RENDER_MODE_DIRECT and LV_DISPLAY_RENDER_MODE_FULL works, see their comments*/
    // LV_ATTRIBUTE_MEM_ALIGN
    // static uint8_t buf_3_1[MY_DISP_HOR_RES * MY_DISP_VER_RES * BYTE_PER_PIXEL];

    // LV_ATTRIBUTE_MEM_ALIGN
    // static uint8_t buf_3_2[MY_DISP_HOR_RES * MY_DISP_VER_RES * BYTE_PER_PIXEL];
    // lv_display_set_buffers(disp, buf_3_1, buf_3_2, sizeof(buf_3_1), LV_DISPLAY_RENDER_MODE_DIRECT);

}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/*Initialize your display and the required peripherals.*/
static void disp_init(void)
{
    /*You code here*/
    LCD_Init();
}

volatile bool disp_flush_enabled = true;

/* Enable updating the screen (the flushing process) when disp_flush() is called by LVGL
 */
void disp_enable_update(void)
{
    disp_flush_enabled = true;
}

/* Disable updating the screen (the flushing process) when disp_flush() is called by LVGL
 */
void disp_disable_update(void)
{
    disp_flush_enabled = false;
}

/*Flush the content of the internal buffer the specific area on the display.
 *`px_map` contains the rendered image as raw pixel map and it should be copied to `area` on the display.
 *You can use DMA or any hardware acceleration to do this operation in the background but
 *'lv_display_flush_ready()' has to be called when it's finished.*/
static lv_display_t * s_disp;
static void disp_flush(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map)
{
    if(!disp_flush_enabled) { lv_display_flush_ready(disp); return; }

    uint32_t w  = (uint32_t)(area->x2 - area->x1 + 1);
    uint32_t h  = (uint32_t)(area->y2 - area->y1 + 1);
    uint32_t px = w * h;

    /* LVGL 内存里是小端，ST7735 要吃“高字节在前”，必须交换 */
    lv_draw_sw_rgb565_swap(px_map, px);

    LCD_Address_Set(area->x1, area->y1, area->x2, area->y2);
    s_disp = disp;
    LCD_CS_Clr();
    if(HAL_SPI_Transmit_DMA(&hspi1, px_map, (uint16_t)(px * 2)) != HAL_OK) {
        HAL_SPI_Transmit(&hspi1, px_map, (uint16_t)(px * 2), HAL_MAX_DELAY);
        LCD_CS_Set();
        lv_display_flush_ready(disp);
    }
    /* DMA 没完成前不能调用 flush_ready */
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef * hspi)
{
    if(hspi->Instance == SPI1) {
        LCD_CS_Set();
        lv_display_flush_ready(s_disp);
    }
}

#else /*Enable this file at the top*/

/*This dummy typedef exists purely to silence -Wpedantic.*/
typedef int keep_pedantic_happy;
#endif
