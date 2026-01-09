#include "esp_log.h"
#include "events_queue.h"
#include "freertos/FreeRTOS.h" // required for pdTICKS_TO_MS and configASSERT
#include "freertos/queue.h"
#include "pomodoro_fsm.h"
#include "pomodoro_timer.h"
#include "uart.h"

#define TAG "MAIN"
#define UART_TAG "UART_TAG"
#define UART_BUFFER_SIZE 200

static inline const char *pomodoro_err_to_string(pomodoro_err_t err) {
  static const char *names[] = {
      "OK",
      "INVALID_TRANSITION",
      "ILLEGAL_TRANSITION",
      "INVALID_ARGUMENTS",
  };
  return (err < sizeof(names) / sizeof(names[0])) ? names[err] : "UNKNOWN";
}

  static const char *state_names[] = {
      "IDLE",
      "RUNNING",
      "PAUSED",
      "FINISHED",
  };

static void print_status(const pomodoro_session_t *session, uint32_t now_ms) {
  const char *state_str = (session->state < POMODORO_STATE_COUNT)
                              ? state_names[session->state]
                              : "UNKNOWN";
  const char *phase_name = session->config->phases[session->phase_index].name;

  ESP_LOGI(TAG,
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

typedef struct {
  const pomodoro_session_t *pomodoro_session;
  QueueHandle_t queue_handle;
} uart_task_ctx_t;

static void uart_task(void *args) {
  uart_task_ctx_t *ctx = (uart_task_ctx_t *)args;
  timestamped_event_t timestamped_event;

  char uart_buffer[UART_BUFFER_SIZE];
  char *trimmed_ptr;

  while (true) {
    esp_err_t err = read_line(uart_buffer, sizeof(uart_buffer), portMAX_DELAY,
                              &trimmed_ptr);
    if (err != ESP_OK) {
      ESP_LOGW("UART", "read_line failed: %s", esp_err_to_name(err));
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

void app_main(void) {
  configure_uart();
  ESP_LOGI(TAG, "Focus Timer initialized");

  // === START Finite State Machine initialization ===

  const pomodoro_config_t pomodoro_config = {
      .phases =
          {
              {.name = "Work", .duration_ms = 25 * 1000},
              {.name = "Rest", .duration_ms = 5 * 1000},
          },
      .count = 2,
  };

  pomodoro_session_t session;
  pomodoro_session_initialize(&session, &pomodoro_config);

  // === END Finite State Machine initialization ===

  // Timestamped atomic queue
  QueueHandle_t queue = xQueueCreate(8, sizeof(timestamped_event_t));
  configASSERT(queue);

  // === START tasks ===
  // == UART ==

  uart_task_ctx_t uart_task_ctx = {
      .pomodoro_session = &session,
      .queue_handle = queue,
  };
  TaskHandle_t uart_task_handle = NULL;
  xTaskCreate(uart_task, "uart-command-parser", 2048, &uart_task_ctx,
              tskIDLE_PRIORITY, &uart_task_handle);

  // == TIMERS ==

  pomodoro_timer_context pomodoro_timer_context;
  pomodoro_timer_context_initialize(&pomodoro_timer_context, queue);

  // === END tasks ===

  // === WHILE LOOP - Handlers ===

  while (true) {
    timestamped_event_t timestamped_event;
    if (!xQueueReceive(queue, &timestamped_event, portMAX_DELAY)) {
      // -- No event --
      continue;
    }

    // On event, dispatch it to fsm
    pomodoro_err_t pomodoro_dispatch_status = pomodoro_session_dispatch(
        &session, timestamped_event.event, timestamped_event.timestamp_ms);

    if (pomodoro_dispatch_status != POMODORO_STATUS_OK) {
      const char *status_str = pomodoro_err_to_string(pomodoro_dispatch_status);
      ESP_LOGW(TAG, "Dispatch failed: %s", status_str);
    }

    // === Invoke handlers ===
    pomodoro_timer_handle_effects(&pomodoro_timer_context, &session,
                                  timestamped_event.timestamp_ms);
  }
}
