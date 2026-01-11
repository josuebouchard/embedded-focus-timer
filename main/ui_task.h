#ifndef UI_TASK_H
#define UI_TASK_H

#include "pomodoro_fsm.h"
#include <freertos/FreeRTOS.h>

typedef pomodoro_session_t ui_fsm_snapshot;

typedef struct {
  QueueHandle_t queue;
  ui_fsm_snapshot snapshot;
  char print_buffer[512];
} ui_context_t;

void ui_task_initialize(ui_context_t *ui_context,
                        ui_fsm_snapshot *first_snapshot);

void ui_task(void *args);

void ui_update_snapshot(const ui_context_t *ctx,
                        const ui_fsm_snapshot *new_snapshot);

#endif // UI_TASK_H
