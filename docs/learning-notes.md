# Learning notes

This document collects things I researched for this project: lessons learned and small tricks I want to keep in my pocket.

## Lessons learned

> Note: not all lessons here have necessarily been applied to this project yet.

- **The importance of design and correctness**: Taking time to understand the problem, state it in words or drawings, and model it not in code, but on paper, can drastically simplify its implementation.
- **A good design helps you build a walking skeleton**: Once the design is complete, my idea was to implement it in its entirety. That got me the shortest design-to-complete time, but also the longest design-to-runnable time. Instead, an alternative would have consisted in recognizing the big picture (in this case, the finite state machine and the reactor pattern) and making partial implementations that were sooner runnable.
- **There is a price for uncoupling**: While adding effects was a great way to not have to tie the FSM to the ESP timers, it took a lot of scaffolding to make it run, and a couple of tries to get it right.
- **Finite State Machines are an amazing tool for modeling domains where invariants change discretely over time**: In this case, the invariants changed whenever the "state" (used loosely here) of the pomodoro changed, and in each state there were specific rules. The FSM made it really easy to model them and uphold those invariants through state transition actions.
- **Both transitions and effects may carry additional information**: Events and effects are not restricted to being regular enums, but they can become fat enums.
  - Event transitions might be dependent on parameters such as `now_ms` (current time in milliseconds). In those cases, that context can be added as a parameter to the dispatch function, or modeled as a fat enum if it's only pertinent to one state.
  - Effects should contain all the information the handlers might need to act upon it.
- **In some cases there are no destructors in embedded**: Given that the assumption is that the microcontroller will run in a loop until powered off or reset, there are some cases where there is no need to bother about destructors/free.
- **Use `assert` for programmer errors, `if`s for dynamic input errors**: `assert`s should be used for internal/setup invariants, since they are removed on release builds without any penalty. For external/dynamic input, `if`s should be used since they won't be removed.

## The influence of Domain Driven Design (DDD) in this project

The same ideas that keep large backend systems sane—clear domain boundaries and explicit modeling—also help embedded projects scale past the “one-file prototype” phase. When the behavior gets complex (modes, timing rules, error handling, multiple inputs), DDD-style separation prevents the RTOS + drivers from leaking into the core logic.

In this project I ended up with DDD-like building blocks:

  - **Ubiquitous language**: *phases*, *session*, *state*, *events*, *effects* (commands). Naming the concepts made discussions and debugging simpler.
  - **Value objects**: e.g. `phases_config` as an immutable description of the timer program (durations, ordering). Passing this around is safer than scattering raw integers.
  - **Entities**: e.g. `timer_session_t` as the thing with identity and evolving state over time (phase index, timing fields, current FSM state).
  - **Domain events (inputs)**: `START`, `PAUSE`, `RESUME`, `TIMEOUT`, etc. are facts that happened *to* the system.
  - **Effects (commands / outputs)**: what the domain *wants done* (arm timer, print status, update LEDs). The FSM emits these as data; handlers perform the side effects.

The key insight: **keeping the domain pure** (FSM + rules) and pushing all I/O into adapters (UART/timer tasks + effect handlers) is basically the embedded version of separating *domain* from *infrastructure*. It’s more work up front than just calling the driver, but it pays off in testability, portability, and the ability to extend behavior without rewriting everything.


## Interesting C/C++ tricks

- **Enums counting**: Add a last enum constant **at the end of the enum** to be able to know how many enum constants are in the enum.
- **How to switch over multiple enums at the same time**: if the enums are compact (no gaps), a macro such as the `KEY()` macro allows you to map multiple enums to a single integer in a one-to-one relationship. This allows that by comparing the values of `KEY`, you are essentially comparing multiple enums together

```c
/*
  * @brief Allows for switching states and events without requiring multiple
  * switches
  */
#define KEY(state, event) ((state) * POMODORO_EVT_COUNT + (event))
```

- **Fat enum pattern**: while C/C++ don’t support fat enums like Rust, you can approximate them using a regular enum as a tag plus a union payload (which is called a _tagged union_).

```c
// The "thin" enum
typedef enum {
  EVENT_1,
  EVENT_2,
} event_type_t;

// The "fat" enum (tagged union)
typedef struct {
  // The regular enum (tag)
  event_type_t type;

  // The extra data (union)
  // Only read the union member that matches type.
  union {
    // Valid when type == EVENT_1
    // Access: e.data.event_1.example_int
    struct { int example_int; } event_1;

    // Valid when type == EVENT_2
    // Access: e.data.event_2.example_string
    struct { const char *example_string; } event_2;
  } data;
} event_t;

// Example construction:
event_t e1 = (event_t){
  .type = EVENT_1,
  .data.event_1 = { .example_int = 123 },
};

// Example use:
if (e1.type == EVENT_1) {
  do_something(e1.data.event_1.example_int);
}
```
