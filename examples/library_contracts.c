#include "../dwell.h"

#include <stdint.h>
#include <stdio.h>

/* Example: contract-test internal state of a library object. */
typedef struct cache {
    uint32_t entries;
    uint32_t evictions;
    double hit_rate;
} cache;

static void emit(const dwell_event *event, const char *json, void *user) {
    (void)event;
    (void)user;
    puts(json);
}

int main(void) {
    cache c = {0, 0, 1.0};
    dwell_ctx dw;
    char err[DWELL_MAX_ERROR];

    dwell_init(&dw);
    dwell_set_event_callback(&dw, emit, NULL);

    dwell_watch_u32(&dw, "cache.entries", &c.entries);
    dwell_watch_u32(&dw, "cache.evictions", &c.evictions);
    dwell_watch_f64(&dw, "cache.hit_rate", &c.hit_rate);

    if (dwell_contract(&dw, "cache.entries > 10000 error cache.unbounded", err, sizeof(err)) < 0) {
        fprintf(stderr, "%s\n", err);
        return 2;
    }
    if (dwell_contract(&dw, "cache.hit_rate < 0.60 warn cache.ineffective", err, sizeof(err)) < 0) {
        fprintf(stderr, "%s\n", err);
        return 2;
    }

    c.entries = 12000;
    c.hit_rate = 0.43;
    dwell_tick(&dw);

    return dw.error_count ? 1 : 0;
}
