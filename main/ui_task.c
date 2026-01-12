#include "ui_task.h"
#include "esp_log.h"
#include "freertos/projdefs.h"
#include "pomodoro_fsm.h"
#include "portmacro.h"
#include <inttypes.h>
#include <stdint.h>
#include <string.h>

#define UI_TAG "UI"
#define UI_UPDATE_INTERVAL_MS 1000

void ui_task_initialize(ui_context_t *ui_context,
                        const ui_fsm_snapshot_t *first_snapshot) {
  ui_context->queue = xQueueCreate(1, sizeof(ui_task_event_t));
  configASSERT(ui_context->queue);

  memcpy(&ui_context->snapshot, first_snapshot, sizeof(ui_fsm_snapshot_t));
}

static void print_snapshot(ui_context_t *ctx, uint32_t now_ms) {
  const size_t PRINT_BUFFER_SIZE = sizeof(ctx->print_buffer);
  const ui_fsm_snapshot_t *snapshot = &ctx->snapshot;

  int required = snprintf(
      ctx->print_buffer, PRINT_BUFFER_SIZE,
      "now_ms=%" PRIu32
      " state=\"%s\" current_phase=\"%s\" time_remaining_ms=%" PRIu32 "\n",
      now_ms, pomodoro_state_to_string(snapshot->state),
      pomodoro_current_phase(snapshot)->name,
      pomodoro_time_remaining_ms(snapshot, now_ms));

  if (required < 0) {
    ESP_LOGW(UI_TAG, "There has been an encoding problem");
  } else if (required >= (int)PRINT_BUFFER_SIZE) {
    ESP_LOGW(UI_TAG, "Buffer truncated! Needed %d bytes, had %zu", required,
             PRINT_BUFFER_SIZE);
  }

  printf("%s", ctx->print_buffer);
}

void ui_task(void *args) {
  ui_context_t *context = (ui_context_t *)args;

  ESP_LOGI(UI_TAG, "UI Task initialized");
  ui_task_event_t event;
  uint32_t now_ms = pdTICKS_TO_MS(xTaskGetTickCount());

  while (true) {
    TickType_t queue_receive_timeout =
        context->snapshot.state == POMODORO_STATE_RUNNING
            ? pdMS_TO_TICKS(UI_UPDATE_INTERVAL_MS)
            : portMAX_DELAY;

    if (xQueueReceive(context->queue, &event, queue_receive_timeout)) {
      switch (event.type) {
      case PRINT_STATUS:
        break; // Don't do anything special
      case UPDATE_SNAPSHOT:
        // Update snapshot
        memcpy(&context->snapshot, &event.data.snapshot,
               sizeof(ui_fsm_snapshot_t));
        break;
      }
    }

    now_ms = pdTICKS_TO_MS(xTaskGetTickCount());
    print_snapshot(context, now_ms);
  }
}

void ui_update_snapshot(const ui_context_t *ctx,
                        const ui_fsm_snapshot_t *new_snapshot) {
  ui_task_event_t event = {
      .type = UPDATE_SNAPSHOT,
      .data.snapshot = *new_snapshot,
  };
  xQueueOverwrite(ctx->queue, &event);
}

void ui_request_status(const ui_context_t *ctx) {
  ui_task_event_t event = {.type = PRINT_STATUS};
  xQueueOverwrite(ctx->queue, &event);
}
