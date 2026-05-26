#pragma once

#include "time_of_day.h"

typedef struct ShadowOpts {
    float offx, offy;
    float scale_y;
    float alpha;
} ShadowOpts;

ShadowOpts shadow_for(SunVector sun, float entity_height);
