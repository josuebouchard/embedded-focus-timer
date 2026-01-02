#include "pomodoro_fsm.h"
#include <stdbool.h>

/*
 * @brief Allows for switching states and events without requiring multiple
 * switches
 */
#define KEY(state, event) ((state) * POMODORO_EVT_COUNT + (event))

static void handle_illegal_transition(pomodoro_session_t *session,
                                      pomodoro_event_t event) {
  // TODO!
}

static bool has_next_phase(const pomodoro_session_t *session) {
  return session->phase_index + 1 < session->config->count;
}

static inline const pomodoro_phase_t *
current_phase(const pomodoro_session_t *session) {
  return &session->config->phases[session->phase_index];
}

static void set_end_time_current_phase(pomodoro_session_t *session,
                                       uint32_t now_ms) {
  session->end_time_ms = now_ms + current_phase(session)->duration_ms;
}

static void advance_phase(pomodoro_session_t *session, uint32_t now_ms) {
  session->phase_index++;
  set_end_time_current_phase(session, now_ms);
}

static void store_remaining_time(pomodoro_session_t *session, uint32_t now_ms) {
  if ((int32_t)(session->end_time_ms - now_ms) > 0) {
    session->remaining_ms = session->end_time_ms - now_ms;
  } else {
    session->remaining_ms = 0; // Already expired
  }
}

static void restore_remaining_time(pomodoro_session_t *session,
                                   uint32_t now_ms) {
  session->end_time_ms = now_ms + session->remaining_ms;
  session->remaining_ms = 0;
}

static void zero_time_fields(pomodoro_session_t *session) {
  session->end_time_ms = 0;
  session->remaining_ms = 0;
}

static void timer_reset_context(pomodoro_session_t *session) {
  // Phases
  session->phase_index = 0;

  // Timing
  zero_time_fields(session);
}

void pomodoro_session_initialize(pomodoro_session_t *session,
                                 const pomodoro_config_t *config) {
  session->state = POMODORO_STATE_IDLE;
  session->config = config;
  timer_reset_context(session);
  pomodoro_effects_clear(&session->effects);
}

void pomodoro_session_dispatch(pomodoro_session_t *session,
                               const pomodoro_event_t event,
                               const uint32_t now_ms) {
  pomodoro_effects_clear(&session->effects);

  switch (KEY(session->state, event)) {

  // restart event
  case (KEY(POMODORO_STATE_IDLE, POMODORO_EVT_RESTART)):
  case (KEY(POMODORO_STATE_RUNNING, POMODORO_EVT_RESTART)):
  case (KEY(POMODORO_STATE_PAUSED, POMODORO_EVT_RESTART)):
  case (KEY(POMODORO_STATE_FINISHED, POMODORO_EVT_RESTART)):
    session->state = POMODORO_STATE_IDLE;
    timer_reset_context(session);
    pomodoro_effects_set(&session->effects,
                         (pomodoro_effect_t[]){POMODORO_EFFECT_TIMER_STOP}, 1);
    break;

  // IDLE
  case KEY(POMODORO_STATE_IDLE, POMODORO_EVT_START):
    session->state = POMODORO_STATE_RUNNING;
    set_end_time_current_phase(session, now_ms);
    pomodoro_effects_set(&session->effects,
                         (pomodoro_effect_t[]){POMODORO_EFFECT_TIMER_START}, 1);
    break;

  // RUNNING
  case KEY(POMODORO_STATE_RUNNING, POMODORO_EVT_PAUSE):
    session->state = POMODORO_STATE_PAUSED;
    store_remaining_time(session, now_ms);
    pomodoro_effects_set(&session->effects,
                         (pomodoro_effect_t[]){POMODORO_EFFECT_TIMER_STOP}, 1);
    break;
  case KEY(POMODORO_STATE_RUNNING, POMODORO_EVT_SKIP):
  case KEY(POMODORO_STATE_RUNNING, POMODORO_EVT_TIMEOUT):
    if (has_next_phase(session)) {
      session->state = POMODORO_STATE_RUNNING;
      advance_phase(session, now_ms);
      pomodoro_effects_set(&session->effects,
                           (pomodoro_effect_t[]){POMODORO_EFFECT_TIMER_START},
                           1);
    } else {
      session->state = POMODORO_STATE_FINISHED;
      zero_time_fields(session);
      pomodoro_effects_set(&session->effects,
                           (pomodoro_effect_t[]){POMODORO_EFFECT_TIMER_STOP},
                           1);
    }
    break;

  // PAUSED
  case (KEY(POMODORO_STATE_PAUSED, POMODORO_EVT_RESUME)):
    session->state = POMODORO_STATE_RUNNING;
    restore_remaining_time(session, now_ms);
    pomodoro_effects_set(&session->effects,
                         (pomodoro_effect_t[]){POMODORO_EFFECT_TIMER_START}, 1);
    break;
  case (KEY(POMODORO_STATE_PAUSED, POMODORO_EVT_SKIP)):
    if (has_next_phase(session)) {
      session->state = POMODORO_STATE_RUNNING;
      advance_phase(session, now_ms);
      pomodoro_effects_set(&session->effects,
                           (pomodoro_effect_t[]){POMODORO_EFFECT_TIMER_START},
                           1);
    } else {
      session->state = POMODORO_STATE_FINISHED;
      zero_time_fields(session);
      pomodoro_effects_set(&session->effects,
                           (pomodoro_effect_t[]){POMODORO_EFFECT_TIMER_STOP},
                           1);
    }
    break;
  case (KEY(POMODORO_STATE_PAUSED, POMODORO_EVT_TIMEOUT)):
    handle_illegal_transition(session, event);
    break;

  // FINISHED
  case (KEY(POMODORO_STATE_FINISHED, POMODORO_EVT_TIMEOUT)):
    handle_illegal_transition(session, event);
    break;

    // Ignore all other transitions
  default:
    break;
  }
}

void pomodoro_effects_clear(pomodoro_effects_t *effects) { effects->count = 0; }

void pomodoro_effects_set(pomodoro_effects_t *effects,
                          const pomodoro_effect_t effects_array[],
                          const uint32_t effects_amount) {
  // Cap the amount of effects
  uint32_t effects_being_written =
      effects_amount <= MAX_EFFECTS ? effects_amount : MAX_EFFECTS;

  for (uint32_t i = 0; i < effects_being_written; i++) {
    effects->effects[i] = effects_array[i];
  }
  effects->count = effects_being_written;
}
