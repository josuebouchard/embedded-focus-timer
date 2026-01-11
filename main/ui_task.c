#include "ui_task.h"
#include "esp_log.h"
#include "freertos/projdefs.h"
#include "pomodoro_fsm.h"
#include "portmacro.h"
#include <inttypes.h>
#include <stdint.h>
#include <string.h>

#define TAG "UI"
#define UI_UPDATE_INTERVAL_MS 1000

void ui_task_initialize(ui_context_t *ui_context,
                        ui_fsm_snapshot *first_snapshot) {
  ui_context->queue = xQueueCreate(1, sizeof(ui_fsm_snapshot));
  configASSERT(ui_context->queue);

  memcpy(&ui_context->snapshot, first_snapshot, sizeof(ui_fsm_snapshot));
}

static void print_snapshot(ui_context_t *ctx, uint32_t now_ms) {
  const size_t PRINT_BUFFER_SIZE = sizeof(ctx->print_buffer);
  const ui_fsm_snapshot *snapshot = &ctx->snapshot;

  int required = snprintf(
      ctx->print_buffer, PRINT_BUFFER_SIZE,
      "now_ms=%" PRIu32
      " state=\"%s\" current_phase=\"%s\" time_remaining_ms=%" PRIu32 "\n",
      now_ms, pomodoro_state_to_string(snapshot->state),
      pomodoro_current_phase(snapshot)->name,
      pomodoro_time_remaining_ms(snapshot, now_ms));

  if (required < 0) {
    ESP_LOGW(TAG, "There has been an encoding problem");
  } else if (required >= PRINT_BUFFER_SIZE) {
    ESP_LOGW(TAG, "Buffer truncated! Needed %d bytes, had %d", required,
             PRINT_BUFFER_SIZE);
  }

  printf("%s", ctx->print_buffer);
}

void ui_task(void *args) {
  ui_context_t *context = (ui_context_t *)args;

  ESP_LOGI(TAG, "UI Task initialized");

  while (true) {
    uint32_t queue_receive_timeout =
        context->snapshot.state == POMODORO_STATE_RUNNING
            ? pdMS_TO_TICKS(UI_UPDATE_INTERVAL_MS)
            : portMAX_DELAY;
    (void)xQueueReceive(context->queue, &context->snapshot,
                        queue_receive_timeout);

    TickType_t now_ms = pdTICKS_TO_MS(xTaskGetTickCount());
    print_snapshot(context, now_ms);
  }
}

void ui_update_snapshot(const ui_context_t *ctx,
                        const pomodoro_session_t *pomodoro_session) {
  xQueueOverwrite(ctx->queue, pomodoro_session);
}
