#ifndef POMODORO_REACTOR_TYPES_H
#define POMODORO_REACTOR_TYPES_H

#include "pomodoro_fsm.h"

typedef struct {
  pomodoro_event_t event;
  uint32_t timestamp_ms;
} timestamped_event_t;

#endif // POMODORO_REACTOR_TYPES_H
