#ifndef POMODORO_FSM_H
#define POMODORO_FSM_H

#include <stdint.h>

typedef enum {
  POMODORO_STATUS_OK = 0,
  POMODORO_STATUS_INVALID_TRANSITION,
  POMODORO_STATUS_ILLEGAL_TRANSITION,
  POMODORO_STATUS_INVALID_ARGUMENTS,
} pomodoro_err_t;

typedef enum {
  POMODORO_STATE_IDLE = 0,
  POMODORO_STATE_RUNNING,
  POMODORO_STATE_PAUSED,
  POMODORO_STATE_FINISHED,
  // MUST BE LAST: Used for getting the count
  POMODORO_STATE_COUNT,
} pomodoro_state_t;

typedef enum {
  POMODORO_EVT_START = 0,
  POMODORO_EVT_PAUSE,
  POMODORO_EVT_RESUME,
  POMODORO_EVT_SKIP,
  POMODORO_EVT_TIMEOUT,
  POMODORO_EVT_RESTART,
  // MUST BE LAST: Used for getting the count
  POMODORO_EVT_COUNT,
} pomodoro_event_t;

typedef enum {
  POMODORO_EFFECT_TIMER_START,
  POMODORO_EFFECT_TIMER_STOP,
} pomodoro_effect_t;

#define MAX_EFFECTS 6

typedef struct {
  pomodoro_effect_t effects[MAX_EFFECTS];
  uint32_t count;
} pomodoro_effects_t;

void pomodoro_effects_clear(pomodoro_effects_t *effects);
void pomodoro_effects_set(pomodoro_effects_t *effects,
                          const pomodoro_effect_t effects_array[],
                          const uint32_t count);

#define MAX_NAME 25

typedef struct {
  char name[MAX_NAME];
  uint32_t duration_ms;
} pomodoro_phase_t;

#define MAX_PHASES 20

typedef struct {
  pomodoro_phase_t phases[MAX_PHASES];
  uint32_t count;
} pomodoro_config_t;

typedef struct {
  // Current state
  pomodoro_state_t state;
  // Phases - immutable after initialization
  const pomodoro_config_t *config;
  uint32_t phase_index;
  // Timing
  uint32_t end_time_ms;
  uint32_t remaining_ms;
  // Effects
  pomodoro_effects_t effects;
} pomodoro_session_t;

void pomodoro_session_initialize(pomodoro_session_t *session,
                                 const pomodoro_config_t *config);

pomodoro_err_t pomodoro_session_dispatch(pomodoro_session_t *session,
                                         pomodoro_event_t event,
                                         uint32_t now_ms);

#endif // POMODORO_FSM_H
