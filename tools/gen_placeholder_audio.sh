#!/usr/bin/env sh
set -eu
mkdir -p assets/audio
ffmpeg -hide_banner -loglevel error -y -f lavfi -i "sine=frequency=880:duration=4" -filter:a "volume=0.10" assets/audio/birds_loop.ogg
ffmpeg -hide_banner -loglevel error -y -f lavfi -i "anoisesrc=color=brown:duration=4" -filter:a "volume=0.07" assets/audio/wind_soft.ogg
ffmpeg -hide_banner -loglevel error -y -f lavfi -i "anoisesrc=color=pink:duration=4" -filter:a "volume=0.10" assets/audio/water_creek.ogg
ffmpeg -hide_banner -loglevel error -y -f lavfi -i "sine=frequency=1320:duration=0.15" -filter:a "volume=0.18" assets/audio/pickup.ogg
ffmpeg -hide_banner -loglevel error -y -f lavfi -i "sine=frequency=220:duration=4" -filter:a "volume=0.06" assets/audio/tod_day.ogg
ffmpeg -hide_banner -loglevel error -y -f lavfi -i "sine=frequency=165:duration=4" -filter:a "volume=0.06" assets/audio/tod_dusk.ogg
ffmpeg -hide_banner -loglevel error -y -f lavfi -i "sine=frequency=110:duration=4" -filter:a "volume=0.05" assets/audio/tod_night.ogg
