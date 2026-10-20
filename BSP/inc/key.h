#ifndef __KEY_H
#define __KEY_H

#include "main.h"

/* ============================ 配置区 ============================ */

/* 按键数量 */
#define KEY_NUM                4

/* Key_Scan_Task() 的调用周期(ms)。
 * 必须与定时器中断周期一致：TIM1 = 168MHz / 168 / 1000 = 1kHz -> 1ms
 * 见 tim.c 的 MX_TIM1_Init()。改定时器分频就必须改这里。 */
#define KEY_SCAN_PERIOD_MS     1

/* 时间参数：一律以 ms 书写，由 KEY_TICKS() 换算成扫描拍数 */
#define KEY_DEBOUNCE_MS        10     /* 消抖确认时间 */
#define KEY_SHORT_PRESS_MS     100    /* 短按门限 */
#define KEY_LONG_PRESS_MS      1000   /* 长按门限 */
#define KEY_DOUBLE_GAP_MS      200    /* 双击间隔窗口，超时判为单击 */
#define KEY_LONG_HOLD_MS       500    /* 长按确认后的连发间隔 */

/* 每个按键的事件队列深度（环形缓冲，满则丢弃最新事件） */
#define KEY_EVENT_QUEUE_LEN    4

/* ms -> 扫描拍数（编译期常量，无运行时开销） */
#define KEY_TICKS(ms)          ((uint32_t)((ms) / (KEY_SCAN_PERIOD_MS)))

/* ============================ 类型 ============================ */

/* 按键索引。注意：索引同时是 key_pins[] 的下标，必须从 0 开始连续 */
typedef enum {
    KEY_1 = 0,
    KEY_2,
    KEY_3,
    KEY_4
} KeyIndex_TypeDef;

typedef enum {
    KEY_EVENT_NONE = 0,     /* 无事件（也用作队列空返回值） */
    KEY_EVENT_SHORT,        /* 单击：松开后经 KEY_DOUBLE_GAP_MS 确认无双击 */
    KEY_EVENT_DOUBLE,       /* 双击 */
    KEY_EVENT_LONG,         /* 长按确认（按下满 KEY_LONG_PRESS_MS） */
    KEY_EVENT_LONG_HOLD,    /* 长按维持连发 */
    KEY_EVENT_LONG_RELEASE  /* 长按后松开 */
} KeyEvent_TypeDef;

/* 引脚映射 */
typedef struct {
    GPIO_TypeDef *port;
    uint16_t      pin;
} KeyPin_TypeDef;

/* ============================ 接口 ============================ */

/* 初始化。放在 MX_GPIO_Init() 之后、HAL_TIM_Base_Start_IT() 之前 */
void Key_Init(void);

/* 扫描任务。必须由 TIM1 更新中断（HAL_TIM_PeriodElapsedCallback）周期调用 */
void Key_Scan_Task(void);

/* --- 事件读取（主循环上下文） --- */

/* 取出并清除一个事件，无事件时返回 KEY_EVENT_NONE。推荐用这个 */
KeyEvent_TypeDef Key_PopEvent(uint8_t key_index);

/* 只查看队首事件，不清除 */
KeyEvent_TypeDef Key_PeekEvent(uint8_t key_index);

/* 丢弃该按键所有待处理事件 */
void Key_FlushEvents(uint8_t key_index);

/* 丢弃所有按键的待处理事件 */
void Key_FlushAllEvents(void);

/* --- 状态查询 --- */

/* 1 = 当前按下（已消抖） */
uint8_t Key_IsPressed(uint8_t key_index);

/* 1 = 松开, 0 = 按下（原始电平，未消抖）。一般只用于调试 */
uint8_t Key_GetRawState(uint8_t key_index);

#endif /* __KEY_H */
