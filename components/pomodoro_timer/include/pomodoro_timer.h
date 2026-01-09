#ifndef POMODORO_TIMER_H
#define POMODORO_TIMER_H

#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "pomodoro_fsm.h"

typedef struct {
  esp_timer_create_args_t timer_args;
  esp_timer_handle_t timer_handle;
  QueueHandle_t queue;
} pomodoro_timer_context;

void pomodoro_timer_context_initialize(pomodoro_timer_context *context,
                                       QueueHandle_t queue);

void pomodoro_timer_handle_effects(pomodoro_timer_context *context,
                                   const pomodoro_effects_t effects);

#endif // POMODORO_TIMER_H
