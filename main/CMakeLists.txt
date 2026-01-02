idf_component_register(SRCS "pomodoro_timer.c" "pomodoro_fsm.c" "main.c" "uart.c"
                       PRIV_REQUIRES esp_driver_uart esp_timer
                       INCLUDE_DIRS ".")
