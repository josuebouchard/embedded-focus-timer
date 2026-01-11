#include "pomodoro_timer.h"
#include "freertos/FreeRTOS.h"
#include "pomodoro_reactor_types.h"

static void timer_callback(void *args) {
  QueueHandle_t queue = (QueueHandle_t)args;

  timestamped_event_t evt = {
      .event = POMODORO_EVT_TIMEOUT,
      .timestamp_ms = pdTICKS_TO_MS(xTaskGetTickCount()),
  };

  xQueueSend(queue, &evt, 0);
}

void pomodoro_timer_context_initialize(pomodoro_timer_context_t *context,
                                       QueueHandle_t queue) {
  esp_timer_create_args_t *timer_args = &context->timer_args;
  timer_args->name = "focus_timer";
  timer_args->callback = timer_callback;
  timer_args->arg = queue;

  esp_timer_create(&context->timer_args, &context->timer_handle);
}

void pomodoro_timer_handle_effects(pomodoro_timer_context_t *context,
                                   const pomodoro_effects_t *effects) {
  for (uint32_t i = 0; i < effects->count; i++) {
    pomodoro_effect_t effect = effects->effects[i];

    uint32_t timeout_ms;
    switch (effect.type) {
    case POMODORO_EFFECT_TIMER_START:
      timeout_ms = effect.timer_start.timeout_ms;
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
