#ifndef EVENTS_QUEUE_H
#define EVENTS_QUEUE_H

#include "pomodoro_fsm.h"

typedef struct {
  pomodoro_event_t event;
  uint32_t timestamp_ms;
} timestamped_event_t;

#endif // EVENTS_QUEUE_H
