# dwell

Tiny internal variable contract checker for C programs.

`dwell` samples variables inside a program at explicit safe points. It can also
run as a small command-line checker for `key=value` sample streams. Its stream
format intentionally matches `trip`, so samples emitted by a dwell-instrumented
program can be checked by either tool.

## Build

```sh
make
make test
```

This builds:

```text
dwell              command-line checker
dwell_demo         toy instrumented program
library_contracts library-object example
```

## Contracts

Format:

```text
key op number level event_id
```

Examples:

```text
imu.temperature_c    > 80.0   warn   imu.overtemp
battery.voltage      < 10.8   error  battery.low
control.loop_ms      > 4.0    error  control_loop.overrun
```

Operators:

```text
> >= < <= == !=
```

Levels:

```text
info warn error
```

## CLI

`dwell` reads the same sample input shape as `trip`:

```text
control.loop_ms=4.36
gps.lock 0
```

Run:

```sh
./dwell -r examples/drone.contracts < samples.tlm
```

Events are tab-separated:

```text
error	control_loop.overrun	control.loop_ms	>	4	4.36
```

Field order:

```text
level event_id key op threshold value
```

Exit status:

```text
0  no failing contract
1  failing warn/error/info contract, depending on --fail-on
2  invalid input, contract, or argument
```

## Interface With Trip

The demo can emit raw samples:

```sh
./dwell_demo --samples > samples.tlm
```

Those samples can be checked by `dwell`:

```sh
./dwell -r examples/drone.contracts < samples.tlm
```

Or by `trip` using the same file:

```sh
./dwell_demo --samples | ../trip/trip -r examples/drone.contracts
```

Both tools emit the same event columns for limit rules.

## Embedding

Compile `dwell.c` with `-DDWELL_NO_MAIN` and include `dwell.h` from a C or C++
program.

```c
#include "dwell.h"

#include <stdio.h>

static float temperature_c = 72.0f;
static double loop_ms = 2.1;

static void on_event(const dwell_event *event, const char *line, void *user) {
    (void)event;
    (void)user;
    puts(line);
}

int main(void) {
    dwell_ctx dw;
    char err[DWELL_MAX_ERROR];

    dwell_init(&dw);
    dwell_set_event_callback(&dw, on_event, NULL);

    dwell_watch_f32(&dw, "sensor.temperature_c", &temperature_c);
    dwell_watch_f64(&dw, "loop.ms", &loop_ms);

    dwell_contract(&dw, "sensor.temperature_c > 80 warn sensor.hot", err, sizeof(err));
    dwell_contract(&dw, "loop.ms > 4 error loop.overrun", err, sizeof(err));

    temperature_c = 82.4f;
    loop_ms = 4.6;
    dwell_tick(&dw);

    return dw.error_count ? 1 : 0;
}
```

## Constraints

- C99
- no heap allocation in the core
- no background thread
- no external dependencies
- explicit sampling through `dwell_tick()`
- works from C and C++

For multithreaded programs, call `dwell_tick()` where reading watched variables
is safe, or use reader callbacks that handle synchronization.
