#!/usr/bin/env bash
# Capture one screenshot per scene + the new-hero form, into docs/assets/.
set -euo pipefail
cd "$(dirname "$0")/.."

make >/dev/null

mkdir -p docs/assets
shoot() {
    local outname="$1" scene="$2" form="${3:-}"
    RAYGUI_SCENE="$scene" RAYGUI_FORM="$form" RAYGUI_SCREENSHOT="$outname" \
        ./build/raygui-menus >/dev/null 2>&1
    mv -f "$outname" "docs/assets/$outname"
    echo "wrote docs/assets/$outname"
}

shoot 01-main-menu.png        0
shoot 02-character.png        1
shoot 03-new-hero-form.png    1 1
shoot 04-inventory.png        2
shoot 05-settings.png         3
shoot 06-hud.png              4
