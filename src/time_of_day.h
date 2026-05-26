#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct Clock {
    int minute;
    int day_index;
    float accum;
    bool rolled_day;
} Clock;

typedef struct SunVector {
    float dx, dy;
    float length;
    float elevation;
} SunVector;

void clock_init(Clock *c, int start_minute);
void clock_tick(Clock *c, float dt, bool paused);
bool clock_rolled_day(const Clock *c);
int clock_hour(const Clock *c);
void clock_advance_to(Clock *c, int target_minute);
SunVector sun_at(int minute);
bool entity_gate_active(int8_t a, int8_t b, int hour);
