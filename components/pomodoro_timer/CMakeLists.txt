idf_component_register(SRCS "pomodoro_timer.c"
    REQUIRES esp_timer
    PRIV_REQUIRES "pomodoro_fsm" "pomodoro_reactor"
    INCLUDE_DIRS "include")
