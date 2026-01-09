#ifndef POMODORO_UART_H
#define POMODORO_UART_H

#include "freertos/FreeRTOS.h"
#include <stdint.h>

void configure_uart(void);
esp_err_t read_line(char *buf, uint32_t length, TickType_t ticks_to_wait,
                    char **out_trimmed);

#endif // POMODORO_UART_H
