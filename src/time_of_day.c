#include "time_of_day.h"

#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void clock_init(Clock *c, int start_minute) {
    *c = (Clock){.minute = start_minute % 1440};
}

void clock_tick(Clock *c, float dt, bool paused) {
    c->rolled_day = false;
    if (paused) {
        return;
    }
    c->accum += dt;
    while (c->accum >= 3.0f) {
        c->accum -= 3.0f;
        c->minute++;
        if (c->minute >= 1440) {
            c->minute = 0;
            c->day_index++;
            c->rolled_day = true;
        }
    }
}

bool clock_rolled_day(const Clock *c) {
    return c->rolled_day;
}

int clock_hour(const Clock *c) {
    return c->minute / 60;
}

void clock_advance_to(Clock *c, int target_minute) {
    target_minute = ((target_minute % 1440) + 1440) % 1440;
    if (target_minute <= c->minute) {
        c->day_index++;
    }
    c->minute = target_minute;
    c->rolled_day = false;
    c->accum = 0.0f;
}

SunVector sun_at(int minute) {
    float hour = (float)minute / 60.0f;
    if (hour < 6.0f || hour > 22.0f) {
        return (SunVector){0, 0, 0, 0};
    }
    float t = (hour - 6.0f) / 12.0f;
    if (t > 1.0f) {
        t = 1.0f;
    }
    float angle = (float)M_PI * t;
    float elevation = sinf(angle);
    return (SunVector){.dx = cosf(angle),
                       .dy = 0.35f,
                       .length = 1.0f - 0.65f * elevation,
                       .elevation = elevation};
}

bool entity_gate_active(int8_t a, int8_t b, int hour) {
    if (a < 0) {
        return true;
    }
    if (a <= b) {
        return hour >= a && hour <= b;
    }
    return hour >= a || hour <= b;
}
