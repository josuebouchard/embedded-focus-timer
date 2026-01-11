#include "uart_task.h"
#include "esp_log.h"
#include "pomodoro_reactor_types.h"
#include "pomodoro_uart.h"
#include "string.h"

static void print_status(const pomodoro_session_t *session, uint32_t now_ms) {
  const char *state_str = pomodoro_state_to_string(session->state);
  const char *phase_name = session->config->phases[session->phase_index].name;

  ESP_LOGI(UART_TAG,
           "State: %s | Phase: %s | End Time: %lu ms | Remaining: %lu ms | "
           "Now: %lu ms",
           state_str, phase_name, session->end_time_ms, session->remaining_ms,
           now_ms);
}

static bool handle_command(const char *cmd, pomodoro_event_t *event_ptr) {
  if (strcmp(cmd, "start") == 0)
    *event_ptr = POMODORO_EVT_START;

  else if (strcmp(cmd, "pause") == 0)
    *event_ptr = POMODORO_EVT_PAUSE;

  else if (strcmp(cmd, "resume") == 0)
    *event_ptr = POMODORO_EVT_RESUME;

  else if (strcmp(cmd, "skip") == 0)
    *event_ptr = POMODORO_EVT_SKIP;

  else if (strcmp(cmd, "timeout") == 0)
    *event_ptr = POMODORO_EVT_TIMEOUT;

  else if (strcmp(cmd, "restart") == 0)
    *event_ptr = POMODORO_EVT_RESTART;
  else
    return false;

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

    if (strcmp(trimmed_ptr, "status") == 0) {
      print_status(ctx->pomodoro_session, pdTICKS_TO_MS(xTaskGetTickCount()));
      continue;
    }

    bool was_command_detected =
        handle_command(trimmed_ptr, &timestamped_event.event);

    if (!was_command_detected) {
      ESP_LOGW(UART_TAG, "Unknown command: %s", trimmed_ptr);
      continue;
    }

    timestamped_event.timestamp_ms = pdTICKS_TO_MS(xTaskGetTickCount());
    xQueueSend(ctx->queue_handle, &timestamped_event, 0);
  }
}