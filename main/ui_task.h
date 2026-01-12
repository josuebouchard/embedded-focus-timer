#ifndef UI_TASK_H
#define UI_TASK_H

#include "pomodoro_fsm.h"
#include <freertos/FreeRTOS.h>

typedef pomodoro_session_t ui_fsm_snapshot_t;

typedef enum ui_task_event_type {
  UPDATE_SNAPSHOT,
  PRINT_STATUS,
} ui_task_event_type_t;

typedef struct ui_task_event {
  ui_task_event_type_t type;
  union {
    ui_fsm_snapshot_t snapshot;
  } data;
} ui_task_event_t;

typedef struct ui_context {
  QueueHandle_t queue;
  ui_fsm_snapshot_t snapshot;
  char print_buffer[512];
} ui_context_t;

void ui_task_initialize(ui_context_t *ui_context,
                        const ui_fsm_snapshot_t *first_snapshot);

void ui_task(void *args);

void ui_update_snapshot(const ui_context_t *ctx,
                        const ui_fsm_snapshot_t *new_snapshot);

void ui_request_status(const ui_context_t *ctx);

#endif // UI_TASK_H
