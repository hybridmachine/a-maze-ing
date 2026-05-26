#!/usr/bin/env sh
set -eu
python3 - <<'PY'
import os, struct, zlib

def png(path, w, h, rgba):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    raw = b''.join(b'\x00' + bytes(rgba) * w for _ in range(h))
    def chunk(t, data):
        return struct.pack(">I", len(data)) + t + data + struct.pack(">I", zlib.crc32(t + data) & 0xffffffff)
    data = b'\x89PNG\r\n\x1a\n'
    data += chunk(b'IHDR', struct.pack(">IIBBBBB", w, h, 8, 6, 0, 0, 0))
    data += chunk(b'IDAT', zlib.compress(raw, 9))
    data += chunk(b'IEND', b'')
    with open(path, 'wb') as f:
        f.write(data)

sprites = {
    "assets/themes/nature/grass.png": (64, 32, (114, 154, 83, 255)),
    "assets/themes/nature/dirt.png": (64, 32, (152, 127, 83, 255)),
    "assets/themes/nature/tree_oak.png": (64, 96, (79, 98, 54, 255)),
    "assets/themes/nature/water.png": (64, 32, (72, 137, 170, 255)),
    "assets/themes/nature/mud.png": (64, 32, (116, 88, 58, 255)),
    "assets/themes/nature/withered_plant.png": (32, 48, (126, 91, 57, 255)),
    "assets/themes/nature/flower.png": (32, 48, (218, 92, 135, 255)),
    "assets/themes/nature/seed_pickup.png": (32, 32, (222, 184, 84, 255)),
    "assets/themes/nature/sign.png": (32, 48, (161, 111, 61, 255)),
    "assets/themes/nature/sundial.png": (48, 48, (184, 178, 132, 255)),
    "assets/themes/nature/locked_gate.png": (64, 64, (120, 110, 100, 255)),
    "assets/themes/nature/npc_gardener.png": (32, 64, (90, 150, 95, 255)),
    "assets/themes/nature/rain_boots_pickup.png": (32, 32, (68, 92, 180, 255)),
    "assets/character/outfits/default.png": (32, 64, (220, 220, 225, 255)),
    "assets/character/outfits/rain_boots.png": (32, 64, (68, 92, 180, 255)),
    "assets/character/faces/neutral.png": (16, 16, (238, 202, 170, 255)),
    "assets/character/faces/happy.png": (16, 16, (248, 212, 172, 255)),
    "assets/character/faces/surprised.png": (16, 16, (238, 196, 168, 255)),
    "assets/character/faces/thinking.png": (16, 16, (226, 196, 166, 255)),
    "assets/ui/icons/seed.png": (24, 24, (222, 184, 84, 255)),
    "assets/ui/icons/rain_boots.png": (24, 24, (68, 92, 180, 255)),
}
for path, args in sprites.items():
    png(path, *args)
PY
