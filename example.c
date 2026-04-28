#include "dwell.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

/*
 * Toy drone loop. These are internal variables: the program does not need to
 * expose them as public telemetry. dwell samples them at explicit safe points.
 */
static float imu_temperature_c = 72.0f;
static float battery_voltage = 12.4f;
static double control_loop_ms = 2.2;
static uint32_t dropped_packets = 0;
static int gps_lock = 1;

static void print_event(const dwell_event *event, const char *json, void *user) {
    (void)event;
    (void)user;
    puts(json);
}

static void print_sample(const char *key, double value, void *user) {
    char line[DWELL_MAX_LINE];

    (void)user;
    if (dwell_format_sample(key, value, line, sizeof(line)) == 0) {
        puts(line);
    }
}

static void step_simulation(unsigned tick) {
    imu_temperature_c += 1.15f;
    battery_voltage -= 0.11f;
    control_loop_ms += (tick > 5) ? 0.42 : 0.08;

    if (tick == 7) {
        dropped_packets = 3;
    }

    if (tick == 9) {
        gps_lock = 0;
    }
}

int main(int argc, char **argv) {
    dwell_ctx dw;
    char err[DWELL_MAX_ERROR];
    unsigned tick;
    int samples_only = 0;

    if (argc > 1 && strcmp(argv[1], "--samples") == 0) {
        samples_only = 1;
    }

    dwell_init(&dw);
    if (samples_only) {
        dwell_set_sample_callback(&dw, print_sample, NULL);
    } else {
        dwell_set_event_callback(&dw, print_event, NULL);
    }

    dwell_watch_f32(&dw, "imu.temperature_c", &imu_temperature_c);
    dwell_watch_f32(&dw, "battery.voltage", &battery_voltage);
    dwell_watch_f64(&dw, "control.loop_ms", &control_loop_ms);
    dwell_watch_u32(&dw, "net.dropped_packets", &dropped_packets);
    dwell_watch_bool(&dw, "gps.lock", &gps_lock);

    if (!samples_only) {
        if (dwell_load_contracts_file(&dw, "examples/drone.contracts", err, sizeof(err)) != 0) {
            fprintf(stderr, "dwell: %s\n", err);
            return 2;
        }
    }

    for (tick = 0; tick < 12; ++tick) {
        step_simulation(tick);
        dwell_tick(&dw);
    }

    return (!samples_only && dw.error_count) ? 1 : 0;
}
