#include "uart_task.h"
#include "esp_log.h"
#include "pomodoro_reactor_types.h"
#include "pomodoro_uart.h"
#include "string.h"

static bool handle_command(const char *cmd, timestamped_event_t *event_ptr,
                           uint32_t now_ms) {
  if (strcmp(cmd, "start") == 0) {
    event_ptr->type = REACTOR_FSM_EVENT;
    event_ptr->data.fsm_event = POMODORO_EVT_START;
  }

  else if (strcmp(cmd, "pause") == 0) {
    event_ptr->type = REACTOR_FSM_EVENT;
    event_ptr->data.fsm_event = POMODORO_EVT_PAUSE;
  }

  else if (strcmp(cmd, "resume") == 0) {
    event_ptr->type = REACTOR_FSM_EVENT;
    event_ptr->data.fsm_event = POMODORO_EVT_RESUME;
  }

  else if (strcmp(cmd, "skip") == 0) {
    event_ptr->type = REACTOR_FSM_EVENT;
    event_ptr->data.fsm_event = POMODORO_EVT_SKIP;
  }

  else if (strcmp(cmd, "timeout") == 0) {
    event_ptr->type = REACTOR_FSM_EVENT;
    event_ptr->data.fsm_event = POMODORO_EVT_TIMEOUT;
  }

  else if (strcmp(cmd, "restart") == 0) {
    event_ptr->type = REACTOR_FSM_EVENT;
    event_ptr->data.fsm_event = POMODORO_EVT_RESTART;
  }

  else if (strcmp(cmd, "status") == 0) {
    event_ptr->type = REACTOR_UI_EVENT;
    event_ptr->data.ui_event = UI_EVT_STATUS;
  }

  else {
    return false;
  }

  event_ptr->timestamp_ms = now_ms;
  return true;
}

void uart_task(void *args) {
  uart_task_context_t *ctx = (uart_task_context_t *)args;
  timestamped_event_t timestamped_event;

  char uart_buffer[UART_BUFFER_SIZE];
  char *trimmed_ptr;

  while (true) {
    esp_err_t err = read_line(uart_buffer, sizeof(uart_buffer), portMAX_DELAY,
                              &trimmed_ptr);
    if (err != ESP_OK) {
      ESP_LOGW(UART_TAG, "read_line failed: %s", esp_err_to_name(err));
      continue;
    }

    if (strlen(trimmed_ptr) == 0)
      continue;

    bool was_command_detected = handle_command(
        trimmed_ptr, &timestamped_event, pdTICKS_TO_MS(xTaskGetTickCount()));

    if (!was_command_detected) {
      ESP_LOGW(UART_TAG, "Unknown command: %s", trimmed_ptr);
      continue;
    }

    xQueueSend(ctx->queue_handle, &timestamped_event, 0);
  }
}