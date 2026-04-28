CC ?= cc
CFLAGS ?= -std=c99 -O2 -Wall -Wextra -pedantic
PREFIX ?= /usr/local

.PHONY: all test install clean

all: dwell dwell_demo library_contracts

dwell: dwell.c dwell.h
	$(CC) $(CFLAGS) dwell.c -o dwell

dwell_demo: example.c dwell.c dwell.h
	$(CC) $(CFLAGS) -DDWELL_NO_MAIN example.c dwell.c -o dwell_demo

library_contracts: examples/library_contracts.c dwell.c dwell.h
	$(CC) $(CFLAGS) -DDWELL_NO_MAIN examples/library_contracts.c dwell.c -o library_contracts

test: all
	@tmp=$$(mktemp -d); \
	trap 'rm -rf "$$tmp"' EXIT HUP INT TERM; \
	set +e; \
	./dwell_demo > "$$tmp/dwell_events.out"; \
	status=$$?; \
	set -e; \
	cat "$$tmp/dwell_events.out"; \
	test "$$status" -eq 1; \
	grep -q 'warn	imu.overtemp	imu.temperature_c	>' "$$tmp/dwell_events.out"; \
	grep -q 'error	gps.lost	gps.lock	==' "$$tmp/dwell_events.out"; \
	set +e; \
	./library_contracts > "$$tmp/library_events.out"; \
	status=$$?; \
	set -e; \
	cat "$$tmp/library_events.out"; \
	test "$$status" -eq 1; \
	grep -q 'error	cache.unbounded	cache.entries	>' "$$tmp/library_events.out"; \
	./dwell_demo --samples > "$$tmp/samples.tlm"; \
	set +e; \
	./dwell -r examples/drone.contracts --summary < "$$tmp/samples.tlm" > "$$tmp/stream_events.out"; \
	status=$$?; \
	set -e; \
	cat "$$tmp/stream_events.out"; \
	test "$$status" -eq 1; \
	grep -q 'error	control_loop.overrun	control.loop_ms	>' "$$tmp/stream_events.out"; \
	if test -x ../trip/trip; then \
		set +e; \
		../trip/trip -r examples/drone.contracts --summary < "$$tmp/samples.tlm" > "$$tmp/trip_events.out"; \
		trip_status=$$?; \
		set -e; \
		cat "$$tmp/trip_events.out"; \
		test "$$trip_status" -eq 1; \
		grep -q 'error	control_loop.overrun	control.loop_ms	>' "$$tmp/trip_events.out"; \
	fi

install: dwell
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 0755 dwell $(DESTDIR)$(PREFIX)/bin/dwell

clean:
	rm -f dwell dwell_demo library_contracts dwell_events.out library_events.out stream_events.out
