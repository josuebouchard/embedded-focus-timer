#ifndef UART_TASK_H
#define UART_TASK_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "pomodoro_fsm.h"

#define UART_TAG "UART_TAG"
#define UART_BUFFER_SIZE 200

typedef struct {
  const pomodoro_session_t *pomodoro_session;
  QueueHandle_t queue_handle;
} uart_task_ctx_t;

void uart_task(void *args);

#endif // UART_TASK_H
