#include "shadow.h"

ShadowOpts shadow_for(SunVector sun, float entity_height) {
    return (ShadowOpts){.offx = sun.dx * sun.length * entity_height,
                        .offy = sun.dy * sun.length * entity_height,
                        .scale_y = sun.length,
                        .alpha = sun.elevation};
}
