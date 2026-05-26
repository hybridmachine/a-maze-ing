#include "audio.h"

#include "game.h"

static float g_master = 1.0f;
static float g_ambience = 1.0f;
static float g_sfx = 1.0f;

void audio_init(void) {
}

void audio_shutdown(void) {
}

void audio_set_layer(AudioLayer layer, const char *clip_key, float target_volume) {
    (void)layer;
    (void)clip_key;
    (void)target_volume;
}

void audio_clear_layer(AudioLayer layer) {
    (void)layer;
}

void audio_oneshot(const char *clip_key, float volume) {
    (void)clip_key;
    (void)volume;
}

void audio_update(float dt) {
    (void)dt;
}

void audio_update_world(Game *g) {
    (void)g;
}

void audio_set_time_of_day(int hour) {
    (void)hour;
}

void audio_master_volume(float v) {
    g_master = v;
    (void)g_master;
}

void audio_ambience_volume(float v) {
    g_ambience = v;
    (void)g_ambience;
}

void audio_sfx_volume(float v) {
    g_sfx = v;
    (void)g_sfx;
}
