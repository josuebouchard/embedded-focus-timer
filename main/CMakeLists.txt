idf_component_register(SRCS "main.c" "uart_task.c"
                       PRIV_REQUIRES pomodoro_fsm pomodoro_timer pomodoro_uart pomodoro_reactor
                       INCLUDE_DIRS ".")
