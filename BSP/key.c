#include "key.h"

/* ==========================================================================
 *  按键扫描模块
 *
 *  上下文约定（改代码前务必先看这里）：
 *
 *    生产者：Key_Scan_Task()  —— 只在 TIM1 更新中断里跑（1ms 一拍）
 *    消费者：Key_PopEvent() 等 —— 只在主循环里跑
 *    数据通道：环形队列 key_event_q[][]
 *
 *  这是标准的单生产者/单消费者模型：head 只被中断写、tail 只被主循环写，
 *  两者都是 uint8_t，Cortex-M4 上单字节读写是原子的，因此不需要进出临界区。
 *  队列满时丢弃"最新"事件而不是覆盖最旧的，保证已入队事件的先后顺序不被破坏。
 * ========================================================================== */

/* 编译期校验：枚举最后一个值必须是 KEY_NUM-1。
 * 防止加了 KEY_5 却忘了改 KEY_NUM（会导致 key_pins[] 越界读）。 */
#define KEY_STATIC_ASSERT(cond, tag) typedef char key_sa_##tag[(cond) ? 1 : -1]
KEY_STATIC_ASSERT((int)KEY_4 == (int)(KEY_NUM - 1), key_index_enum_matches_key_num);

/* ------------------------- 引脚映射（常量，放 flash） ------------------------- */
static const KeyPin_TypeDef key_pins[KEY_NUM] = {
    { KEY1_GPIO_Port, KEY1_Pin },
    { KEY2_GPIO_Port, KEY2_Pin },
    { KEY3_GPIO_Port, KEY3_Pin },
    { KEY4_GPIO_Port, KEY4_Pin },
};

/* --------------------- 扫描状态：仅中断上下文访问，无需 volatile --------------------- */
typedef enum {
    KEY_STATE_IDLE = 0,       /* 空闲，等待按下 */
    KEY_STATE_PRESS_DETECT,   /* 按下消抖中 */
    KEY_STATE_PRESS,          /* 稳定按下，判定长按中 */
    KEY_STATE_RELEASE_DETECT, /* 松开消抖中 */
    KEY_STATE_WAIT_DOUBLE,    /* 已松开一次，等待第二次按下 */
    KEY_STATE_LONG_PRESS      /* 长按已确认，周期性连发 */
} KeyState_TypeDef;

typedef struct {
    KeyState_TypeDef state;
    uint32_t press_time;    /* 按下已持续的拍数 */
    uint32_t release_time;  /* 松开后已等待的拍数（双击窗口计时） */
    uint32_t hold_count;    /* 长按已连发的次数 */
    uint8_t  filter_count;  /* 消抖计数 */
    uint8_t  click_count;   /* 连击计数 */
} KeyCtx_TypeDef;

static KeyCtx_TypeDef key_ctx[KEY_NUM];

/* ------------------------- 跨上下文共享量：必须 volatile ------------------------- */
static volatile KeyEvent_TypeDef key_event_q[KEY_NUM][KEY_EVENT_QUEUE_LEN];
static volatile uint8_t key_q_head[KEY_NUM];   /* 队首：生产者写，消费者读 */
static volatile uint8_t key_q_tail[KEY_NUM];   /* 队尾：消费者写，生产者读 */
static volatile uint8_t key_pressed[KEY_NUM];  /* 已消抖的按下标志，供主循环查询 */

/* ========================== 事件队列 — 生产者侧 ========================== */

/* 仅在中断上下文调用 */
static void Key_PostEvent(uint8_t key_index, KeyEvent_TypeDef ev)
{
    uint8_t head = key_q_head[key_index];
    uint8_t next = (uint8_t)((head + 1u) % KEY_EVENT_QUEUE_LEN);

    if (next == key_q_tail[key_index]) {
        /* 队列满：丢弃新事件。宁可少发一个事件，也不能让已入队的事件乱序 */
        return;
    }

    key_event_q[key_index][head] = ev;
    key_q_head[key_index] = next;
}

/* ========================== 事件队列 — 消费者侧 ========================== */

/* 仅在主循环上下文调用 */
static uint8_t Key_TryPopEvent(uint8_t key_index, KeyEvent_TypeDef *out)
{
    uint8_t tail = key_q_tail[key_index];

    if (tail == key_q_head[key_index]) {
        return 0;   /* 队列空 */
    }

    *out = key_event_q[key_index][tail];
    key_q_tail[key_index] = (uint8_t)((tail + 1u) % KEY_EVENT_QUEUE_LEN);
    return 1;
}

KeyEvent_TypeDef Key_PopEvent(uint8_t key_index)
{
    KeyEvent_TypeDef ev = KEY_EVENT_NONE;

    if (key_index >= KEY_NUM) {
        return KEY_EVENT_NONE;
    }

    (void)Key_TryPopEvent(key_index, &ev);
    return ev;
}

KeyEvent_TypeDef Key_PeekEvent(uint8_t key_index)
{
    if (key_index >= KEY_NUM) {
        return KEY_EVENT_NONE;
    }
    if (key_q_tail[key_index] == key_q_head[key_index]) {
        return KEY_EVENT_NONE;
    }

    return key_event_q[key_index][key_q_tail[key_index]];
}

void Key_FlushEvents(uint8_t key_index)
{
    if (key_index >= KEY_NUM) {
        return;
    }
    key_q_tail[key_index] = key_q_head[key_index];
}

void Key_FlushAllEvents(void)
{
    for (uint8_t i = 0; i < KEY_NUM; i++) {
        key_q_tail[i] = key_q_head[i];
    }
}

/* ============================== 引脚读取 ============================== */

/* 返回 0 = 按下（低电平有效）, 1 = 松开 */
static uint8_t Key_ReadPin(uint8_t key_index)
{
    return (HAL_GPIO_ReadPin(key_pins[key_index].port,
                             key_pins[key_index].pin) == GPIO_PIN_RESET) ? (uint8_t)0
                                                                         : (uint8_t)1;
}

uint8_t Key_GetRawState(uint8_t key_index)
{
    if (key_index >= KEY_NUM) {
        return 1u;   /* 无效索引视为松开 */
    }
    return Key_ReadPin(key_index);
}

uint8_t Key_IsPressed(uint8_t key_index)
{
    if (key_index >= KEY_NUM) {
        return 0u;
    }
    return key_pressed[key_index];
}

/* ============================== 初始化 ============================== */

void Key_Init(void)
{
    for (uint8_t i = 0; i < KEY_NUM; i++) {
        key_ctx[i].state        = KEY_STATE_IDLE;
        key_ctx[i].press_time   = 0;
        key_ctx[i].release_time = 0;
        key_ctx[i].hold_count   = 0;
        key_ctx[i].filter_count = 0;
        key_ctx[i].click_count  = 0;

        key_q_head[i] = 0;
        key_q_tail[i] = 0;
        key_pressed[i] = 0;
    }
}

/* ============================== 扫描状态机 ============================== */

static void Key_Scan_Single(uint8_t key_index)
{
    KeyCtx_TypeDef *key = &key_ctx[key_index];
    uint8_t pin = Key_ReadPin(key_index);   /* 0 = 按下, 1 = 松开 */

    switch (key->state)
    {
    /* ---------------- 空闲：等待按下 ---------------- */
    case KEY_STATE_IDLE:
        if (pin == 0u) {
            key->state        = KEY_STATE_PRESS_DETECT;
            key->filter_count = 0;
        }
        break;

    /* ---------------- 按下消抖 ---------------- */
    case KEY_STATE_PRESS_DETECT:
        if (pin == 0u) {
            if (++key->filter_count >= KEY_TICKS(KEY_DEBOUNCE_MS)) {
                key->state             = KEY_STATE_PRESS;
                key->press_time        = 0;
                key->hold_count        = 0;
                key_pressed[key_index] = 1u;
            }
        } else {
            key->state = KEY_STATE_IDLE;   /* 没撑过消抖，判为毛刺 */
        }
        break;

    /* ---------------- 稳定按下：只负责判定长按 ---------------- */
    case KEY_STATE_PRESS:
        if (pin == 0u) {
            key->press_time++;

            if (key->press_time >= KEY_TICKS(KEY_LONG_PRESS_MS)) {
                key->state      = KEY_STATE_LONG_PRESS;
                key->hold_count = 0;
                Key_PostEvent(key_index, KEY_EVENT_LONG);
            }
            /* 注意：连发(KEY_EVENT_LONG_HOLD)不在这里做。
             * 长按尚未确认就发连发事件是错的，会在 0.6s 左右误报一次 */
        } else {
            key->state        = KEY_STATE_RELEASE_DETECT;
            key->filter_count = 0;
        }
        break;

    /* ---------------- 长按维持：按固定间隔连发 ---------------- */
    case KEY_STATE_LONG_PRESS:
        if (pin == 0u) {
            key->press_time++;

            /* 以"长按确认时刻"为零点，每满一个间隔连发一次：
             * LONG_PRESS_MS 时发 LONG，之后 +500ms / +1000ms ... 发 HOLD */
            if ((key->press_time - KEY_TICKS(KEY_LONG_PRESS_MS)) >=
                ((key->hold_count + 1u) * KEY_TICKS(KEY_LONG_HOLD_MS))) {
                key->hold_count++;
                Key_PostEvent(key_index, KEY_EVENT_LONG_HOLD);
            }
        } else {
            Key_PostEvent(key_index, KEY_EVENT_LONG_RELEASE);
            key->state             = KEY_STATE_IDLE;
            key->click_count       = 0;
            key_pressed[key_index] = 0u;
        }
        break;

    /* ---------------- 松开消抖：判定单击 / 双击 ---------------- */
    case KEY_STATE_RELEASE_DETECT:
        if (pin == 1u) {
            if (++key->filter_count >= KEY_TICKS(KEY_DEBOUNCE_MS)) {
                key_pressed[key_index] = 0u;

                if (key->press_time >= KEY_TICKS(KEY_SHORT_PRESS_MS)) {
                    key->click_count++;

                    if (key->click_count >= 2u) {
                        /* 窗口内第二次按下并松开 -> 双击 */
                        Key_PostEvent(key_index, KEY_EVENT_DOUBLE);
                        key->click_count = 0;
                        key->state       = KEY_STATE_IDLE;
                    } else {
                        /* 第一次松开，挂起等第二次 */
                        key->state        = KEY_STATE_WAIT_DOUBLE;
                        key->release_time = 0;
                    }
                } else {
                    key->state = KEY_STATE_IDLE;   /* 按得太短，判为抖动 */
                }
            }
        } else {
            /* 松开过程中又抖回按下，退回按下状态继续计时 */
            key->state        = KEY_STATE_PRESS;
            key->filter_count = 0;
        }
        break;

    /* ---------------- 等待第二次按下 ---------------- */
    case KEY_STATE_WAIT_DOUBLE:
        if (pin == 0u) {
            key->state        = KEY_STATE_PRESS_DETECT;
            key->filter_count = 0;
            key->release_time = 0;
        } else if (++key->release_time >= KEY_TICKS(KEY_DOUBLE_GAP_MS)) {
            /* 双击窗口超时 -> 确认是单击 */
            Key_PostEvent(key_index, KEY_EVENT_SHORT);
            key->click_count = 0;
            key->state       = KEY_STATE_IDLE;
        }
        break;

    default:
        key->state = KEY_STATE_IDLE;
        break;
    }
}

/* 由 TIM1 更新中断周期调用（1ms） */
void Key_Scan_Task(void)
{
    for (uint8_t i = 0; i < KEY_NUM; i++) {
        Key_Scan_Single(i);
    }
}
