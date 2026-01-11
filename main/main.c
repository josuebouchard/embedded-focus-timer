#include "esp_log.h"
#include "freertos/FreeRTOS.h" // required for pdTICKS_TO_MS and configASSERT
#include "freertos/queue.h"
#include "pomodoro_fsm.h"
#include "pomodoro_reactor_types.h"
#include "pomodoro_timer.h"
#include "pomodoro_uart.h"
#include "uart_task.h"
#include "ui_task.h"

#define TAG "MAIN"

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
  pomodoro_effects_t effects;
  pomodoro_session_initialize(&session, &effects, &pomodoro_config);

  // === END Finite State Machine initialization ===

  // Timestamped atomic queue
  QueueHandle_t queue = xQueueCreate(8, sizeof(timestamped_event_t));
  configASSERT(queue);

  // UI context
  ui_context_t ui_task_context;
  ui_task_initialize(&ui_task_context, &session);

  // === START tasks ===
  // == UART ==

  uart_task_context_t uart_task_ctx = {
      .pomodoro_session = &session,
      .queue_handle = queue,
  };
  TaskHandle_t uart_task_handle = NULL;
  xTaskCreate(uart_task, "uart-command-parser", 2048, &uart_task_ctx,
              tskIDLE_PRIORITY, &uart_task_handle);

  // == TIMERS ==

  pomodoro_timer_context_t pomodoro_timer_context;
  pomodoro_timer_context_initialize(&pomodoro_timer_context, queue);

  // == UI ==

  TaskHandle_t ui_task_handle = NULL;
  xTaskCreate(ui_task, "ui-task", 2048, &ui_task_context, tskIDLE_PRIORITY,
              &ui_task_handle);

  // === END tasks ===

  // === WHILE LOOP - Handlers ===

  while (true) {
    timestamped_event_t timestamped_event;
    if (!xQueueReceive(queue, &timestamped_event, portMAX_DELAY)) {
      // -- No event --
      continue;
    }

    // On event, dispatch it to fsm
    pomodoro_err_t pomodoro_dispatch_status =
        pomodoro_session_dispatch(&session, timestamped_event.event,
                                  timestamped_event.timestamp_ms, &effects);

    if (pomodoro_dispatch_status != POMODORO_STATUS_OK) {
      const char *status_str = pomodoro_err_to_string(pomodoro_dispatch_status);
      ESP_LOGW(TAG, "Dispatch failed: %s", status_str);
    }

    // === Invoke handlers ===
    pomodoro_timer_handle_effects(&pomodoro_timer_context, &effects);

    if (pomodoro_dispatch_status == POMODORO_STATUS_OK) {
      ui_update_snapshot(&ui_task_context, &session);
    }
  }
}
