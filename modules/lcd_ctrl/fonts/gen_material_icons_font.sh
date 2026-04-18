#!/usr/bin/env bash
# Regenerate lv_font_material_icons_22.c from MaterialIcons-Regular.ttf (Google Fonts).
# Requires: Docker, network.
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
FONTS_DIR="$SCRIPT_DIR"
TTF_URL="https://raw.githubusercontent.com/google/material-design-icons/master/font/MaterialIcons-Regular.ttf"
TTF="$FONTS_DIR/MaterialIcons-Regular.ttf"
curl -fsSL -o "$TTF" "$TTF_URL"
OUT_C="$FONTS_DIR/lv_font_material_icons_22.c"
# Output basename must match symbol lv_font_material_icons_22 (lv_font_conv names from -o file stem).
docker run --rm \
  -v "$FONTS_DIR:/work" \
  node:20-bookworm-slim \
  bash -lc 'npm i -g lv_font_conv >/dev/null && lv_font_conv \
    --font /work/MaterialIcons-Regular.ttf \
    -r 0xeb2f,0xe63e,0xe2bd,0xe8b8,0xe430,0xf036,0xe176,0xe627,0xe037,0xe034,0xe1a7 \
    --size 22 --bpp 4 --format lvgl --no-compress \
    -o /work/lv_font_material_icons_22.c --lv-include lvgl.h'
echo "Wrote $OUT_C (re-apply top-of-file license comment if you use this script)."
