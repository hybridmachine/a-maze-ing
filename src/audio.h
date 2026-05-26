#pragma once

struct Game;

typedef enum { AUDIO_BASE, AUDIO_TEXTURE, AUDIO_TOD, AUDIO_LAYER_COUNT } AudioLayer;

void audio_init(void);
void audio_shutdown(void);
void audio_set_layer(AudioLayer layer, const char *clip_key, float target_volume);
void audio_clear_layer(AudioLayer layer);
void audio_oneshot(const char *clip_key, float volume);
void audio_update(float dt);
void audio_update_world(struct Game *g);
void audio_set_time_of_day(int hour);
void audio_master_volume(float v);
void audio_ambience_volume(float v);
void audio_sfx_volume(float v);
