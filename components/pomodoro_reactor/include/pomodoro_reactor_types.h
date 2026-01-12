#ifndef POMODORO_REACTOR_TYPES_H
#define POMODORO_REACTOR_TYPES_H

#include "pomodoro_fsm.h"

typedef enum reactor_event_type {
  REACTOR_FSM_EVENT,
  REACTOR_UI_EVENT,
} reactor_event_type_t;

typedef enum ui_event_type {
  UI_EVT_STATUS,
} ui_event_type_t;

typedef struct timestamped_event {
  reactor_event_type_t type;
  uint32_t timestamp_ms;
  union {
    ui_event_type_t ui_event;
    pomodoro_event_t fsm_event;
  } data;
} timestamped_event_t;

#endif // POMODORO_REACTOR_TYPES_H
