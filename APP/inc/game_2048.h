#ifndef __GAME_2048_H
#define __GAME_2048_H

#ifdef __cplusplus
extern "C" {
#endif

#include "custom.h"     /* gg_ui_t */

/* 在 screen_2 上创建隐藏的 2048 界面，并给 begin 按钮绑定“开始游戏”事件。
 * 必须在 setup_screen_2() 之后调用（custom_init 里已经处理）。 */
void Game2048_Init(gg_ui_t * ui);

#ifdef __cplusplus
}
#endif

#endif /* __GAME_2048_H */
