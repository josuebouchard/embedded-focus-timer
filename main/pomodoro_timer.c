#include "pomodoro_timer.h"
#include "events_queue.h"
#include "freertos/FreeRTOS.h"

static void timer_callback(void *args) {
  QueueHandle_t queue = (QueueHandle_t)args;

  timestamped_event_t evt = {
      .event = POMODORO_EVT_TIMEOUT,
      .timestamp_ms = pdTICKS_TO_MS(xTaskGetTickCount()),
  };

  xQueueSend(queue, &evt, 0);
}

void pomodoro_timer_context_initialize(pomodoro_timer_context *context,
                                       QueueHandle_t queue) {
  esp_timer_create_args_t *timer_args = &context->timer_args;
  timer_args->name = "focus_timer";
  timer_args->callback = timer_callback;
  timer_args->arg = queue;

  esp_timer_create(&context->timer_args, &context->timer_handle);
}

void pomodoro_timer_handle_effects(pomodoro_timer_context *context,
                                   const pomodoro_session_t *session,
                                   TickType_t now_ms) {
  for (uint32_t i = 0; i < session->effects.count; i++) {
    pomodoro_effect_t effect = session->effects.effects[i];

    uint32_t timeout_ms;
    switch (effect) {
    case POMODORO_EFFECT_TIMER_START:
      timeout_ms = session->end_time_ms - now_ms;
      esp_timer_start_once(context->timer_handle, (uint64_t)timeout_ms * 1000);
      break;
    case POMODORO_EFFECT_TIMER_STOP:
      esp_timer_stop(context->timer_handle);
      break;
    default:
      break;
    }
  }
}
