#include "ui.h"
#include "raylib.h"
#include "raygui.h"

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define UI_STR_ARENA_BYTES 16384
static char ui_str_arena[UI_STR_ARENA_BYTES];
static int  ui_str_off = 0;

void UI_BeginFrame(void) { ui_str_off = 0; }

const char *UI_Fmt(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int remaining = UI_STR_ARENA_BYTES - ui_str_off;
    if (remaining < 2) { va_end(ap); return ""; }
    char *out = ui_str_arena + ui_str_off;
    int n = vsnprintf(out, remaining, fmt, ap);
    va_end(ap);
    if (n < 0) n = 0;
    if (n >= remaining) n = remaining - 1;
    ui_str_off += n + 1;
    return out;
}

const Color COL_BG_DEEP     = { 14,  16,  26,  255 };
const Color COL_BG          = { 22,  24,  36,  255 };
const Color COL_PANEL       = { 30,  34,  50,  235 };
const Color COL_PANEL_HOVER = { 46,  52,  74,  255 };
const Color COL_BORDER      = { 70,  78, 110,  255 };
const Color COL_ACCENT      = { 255, 184,  72, 255 };
const Color COL_ACCENT_DIM  = { 178, 122,  44, 255 };
const Color COL_DANGER      = { 220,  72,  72, 255 };
const Color COL_GOLD        = { 232, 196, 110, 255 };
const Color COL_TEXT        = { 232, 232, 240, 255 };
const Color COL_TEXT_DIM    = { 168, 172, 196, 255 };
const Color COL_TEXT_FAINT  = { 110, 116, 140, 255 };
const Color COL_HP          = { 196,  62,  68, 255 };
const Color COL_MP          = {  72, 130, 220, 255 };
const Color COL_XP          = { 156, 220,  92, 255 };

// raylib's default font is 10px tall; we scale via the size argument.
// The advance for a glyph at fontSize is approximately fontSize plus letterSpacing.
int UI_TextWidth(const char *text, int fontSize, int letterSpacing) {
    Font f = GetFontDefault();
    Vector2 v = MeasureTextEx(f, text, (float)fontSize, (float)letterSpacing);
    return (int)v.x;
}

void UI_DrawText(const char *text, float x, float y, int fontSize, int letterSpacing, Color color) {
    Font f = GetFontDefault();
    DrawTextEx(f, text, (Vector2){ x, y }, (float)fontSize, (float)letterSpacing, color);
}

void UI_DrawTextCentered(const char *text, Rectangle box, int fontSize, int letterSpacing, Color color) {
    int w = UI_TextWidth(text, fontSize, letterSpacing);
    float x = box.x + (box.width - w) * 0.5f;
    float y = box.y + (box.height - fontSize) * 0.5f;
    UI_DrawText(text, x, y, fontSize, letterSpacing, color);
}

// Tokenise wrapped text on spaces and draw words that fit on each line.
float UI_DrawTextWrapped(const char *text, float x, float y, float maxWidth,
                         int fontSize, int lineHeight, int letterSpacing, Color color) {
    if (lineHeight <= 0) lineHeight = fontSize + 4;
    float cursorX = x;
    float cursorY = y;
    char word[256];
    int wlen = 0;
    int spaceW = UI_TextWidth(" ", fontSize, letterSpacing);
    for (const char *p = text;; p++) {
        char c = *p;
        if (c == ' ' || c == '\0' || c == '\n') {
            if (wlen > 0) {
                word[wlen] = '\0';
                int ww = UI_TextWidth(word, fontSize, letterSpacing);
                if (cursorX + ww > x + maxWidth && cursorX > x) {
                    cursorY += lineHeight;
                    cursorX = x;
                }
                UI_DrawText(word, cursorX, cursorY, fontSize, letterSpacing, color);
                cursorX += ww + spaceW;
                wlen = 0;
            }
            if (c == '\0') break;
            if (c == '\n') { cursorY += lineHeight; cursorX = x; }
        } else {
            if (wlen < (int)sizeof(word) - 1) word[wlen++] = c;
        }
    }
    return cursorY + lineHeight;
}

void UI_Panel(Rectangle r, Color fill, UI_Border b, float cornerRadius) {
    if (fill.a > 0) {
        if (cornerRadius > 0.5f) {
            float roundness = (cornerRadius * 2.0f) / fminf(r.width, r.height);
            if (roundness > 1.0f) roundness = 1.0f;
            DrawRectangleRounded(r, roundness, 6, fill);
        } else {
            DrawRectangleRec(r, fill);
        }
    }
    if (b.color.a > 0) {
        if (b.top > 0)    DrawRectangleRec((Rectangle){ r.x, r.y, r.width, b.top }, b.color);
        if (b.bottom > 0) DrawRectangleRec((Rectangle){ r.x, r.y + r.height - b.bottom, r.width, b.bottom }, b.color);
        if (b.left > 0)   DrawRectangleRec((Rectangle){ r.x, r.y, b.left, r.height }, b.color);
        if (b.right > 0)  DrawRectangleRec((Rectangle){ r.x + r.width - b.right, r.y, b.right, r.height }, b.color);
    }
}

void UI_AccentBar(Rectangle r, Color color) {
    DrawRectangleRec(r, color);
}

// A modal latch: while a modal is up, base-layer Hover queries return false.
static bool g_modalActive = false;
void UI_SetModalActive(bool active) { g_modalActive = active; }

bool UI_Hovered(Rectangle r) {
    if (g_modalActive) return false;
    return CheckCollisionPointRec(GetMousePosition(), r);
}

// =====================================================================
// Sidebar
// =====================================================================

static const char *SCENE_KEYS[SCENE_COUNT]   = { "1", "2", "3", "4", "5" };
static const char *SCENE_LABELS[SCENE_COUNT] = {
    "MAIN MENU", "CHARACTER", "INVENTORY", "SETTINGS", "HUD / PAUSE",
};

void Sidebar(AppState *state, Rectangle area) {
    UI_Panel(area, COL_BG,
        (UI_Border){ .color = COL_BORDER, .right = 1 }, 0);

    float padX = 18, padY = 24;
    float x = area.x + padX;
    float y = area.y + padY;
    float w = area.width - padX * 2;

    UI_DrawText("RAYGUI  DEMO", x + 4, y, 22, 2, COL_ACCENT);
    y += 22 + 12;
    UI_DrawText("game menus", x + 4, y, 12, 1, COL_TEXT_FAINT);
    y += 12 + 18;

    int rowH = 36;
    int rowGap = 6;
    for (int i = 0; i < SCENE_COUNT; i++) {
        Rectangle row = { x, y, w, rowH };
        bool active = (state->scene == (SceneId)i);
        bool hover  = UI_Hovered(row);

        Color bg = active ? COL_PANEL_HOVER : (hover ? COL_PANEL : (Color){0,0,0,0});
        UI_Panel(row, bg, (UI_Border){0}, 6);
        if (active) {
            DrawRectangleRec((Rectangle){ row.x, row.y, 3, row.height }, COL_ACCENT);
        }

        // Number chip
        float chipSize = 20;
        Rectangle chip = { row.x + 12, row.y + (row.height - chipSize) / 2.0f, chipSize, chipSize };
        UI_Panel(chip, active ? COL_ACCENT : COL_BG_DEEP,
                 (UI_Border){ .color = COL_BORDER, .left = 1, .right = 1, .top = 1, .bottom = 1 }, 4);
        UI_DrawTextCentered(SCENE_KEYS[i], chip, 12, 0, active ? COL_BG_DEEP : COL_TEXT_DIM);

        UI_DrawText(SCENE_LABELS[i],
            chip.x + chipSize + 10,
            row.y + (row.height - 14) / 2.0f,
            14, 1,
            active ? COL_TEXT : COL_TEXT_DIM);

        if (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            state->scene = (SceneId)i;
        }

        y += rowH + rowGap;
    }

    // Bottom footer block
    float footH = 12 + 12 + 4 + 11 + 4 + 11 + 4 + 11;
    Rectangle foot = { x, area.y + area.height - padY - footH, w, footH };
    UI_Panel(foot, COL_BG_DEEP,
        (UI_Border){ .color = COL_BORDER, .left = 1, .right = 1, .top = 1, .bottom = 1 }, 6);
    float fy = foot.y + 12;
    UI_DrawText("v0.1.0  build 042", foot.x + 6, fy, 11, 0, COL_TEXT_FAINT);  fy += 11 + 4;
    UI_DrawText("layout: raygui 5.0", foot.x + 6, fy, 11, 0, COL_TEXT_FAINT); fy += 11 + 4;
    UI_DrawText("render: raylib 5.5", foot.x + 6, fy, 11, 0, COL_TEXT_FAINT);
}

// =====================================================================
// Footer
// =====================================================================

void Footer(Rectangle area) {
    UI_Panel(area, COL_BG,
        (UI_Border){ .color = COL_BORDER, .top = 1 }, 0);
    float padX = 18;
    float y = area.y + (area.height - 12) / 2.0f;
    float x = area.x + padX;

    const char *l1 = "[1-5] switch scene";
    const char *l2 = "[scroll] inventory list";
    const char *l3 = "[click] interact";
    UI_DrawText(l1, x, y, 12, 1, COL_TEXT_FAINT);  x += UI_TextWidth(l1, 12, 1) + 18;
    UI_DrawText(l2, x, y, 12, 1, COL_TEXT_FAINT);  x += UI_TextWidth(l2, 12, 1) + 18;
    UI_DrawText(l3, x, y, 12, 1, COL_TEXT_FAINT);

    const char *brand = "RAYGUI x RAYLIB";
    int bw = UI_TextWidth(brand, 12, 3);
    UI_DrawText(brand, area.x + area.width - padX - 8 - bw, y, 12, 3, COL_ACCENT_DIM);
}

// =====================================================================
// StatBar
// =====================================================================

void StatBar(const char *label, float value, float maxValue, Color fillColor, Rectangle area) {
    float pct = (maxValue <= 0.0f) ? 0.0f : value / maxValue;
    if (pct < 0) pct = 0;
    if (pct > 1) pct = 1;

    // Label row
    UI_DrawText(label, area.x, area.y, 12, 2, COL_TEXT_DIM);
    const char *qty = UI_Fmt("%d / %d", (int)value, (int)maxValue);
    int qw = UI_TextWidth(qty, 12, 0);
    UI_DrawText(qty, area.x + area.width - qw, area.y, 12, 0, COL_TEXT);

    Rectangle track = { area.x, area.y + 16, area.width, 10 };
    UI_Panel(track, COL_BG_DEEP,
        (UI_Border){ .color = COL_BORDER, .left = 1, .right = 1, .top = 1, .bottom = 1 }, 3);
    Rectangle fill = { track.x, track.y, track.width * pct, track.height };
    if (fill.width >= 1) UI_Panel(fill, fillColor, (UI_Border){0}, 3);
}
