#include "ui.h"
#include "raylib.h"
#include "raygui.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

// External setter from ui.c.
extern void UI_SetModalActive(bool active);

// Forward decl — drawn after the rest of the character scene.
static void NewHeroForm(AppState *state);

// =====================================================================
// Small inline button helper — matches clay-ui's custom button look.
// =====================================================================

typedef struct {
    Color fill, fillHover;
    Color border;
    int borderL, borderR, borderT, borderB;
    int fontSize;
    int letterSpacing;
    Color textColor, textHoverColor;
    float cornerRadius;
} ButtonStyle;

static bool UI_Button(Rectangle r, const char *label, ButtonStyle s) {
    bool hover = UI_Hovered(r);
    UI_Panel(r, hover ? s.fillHover : s.fill,
        (UI_Border){
            .color = s.border,
            .left   = (float)s.borderL,
            .right  = (float)s.borderR,
            .top    = (float)s.borderT,
            .bottom = (float)s.borderB,
        },
        s.cornerRadius);
    UI_DrawTextCentered(label, r, s.fontSize, s.letterSpacing,
        hover ? s.textHoverColor : s.textColor);
    return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

// =====================================================================
// Scene 1: Main Menu
// =====================================================================

static const char *MAIN_MENU_ITEMS[] = {
    "NEW GAME", "CONTINUE", "CHARACTERS", "SETTINGS", "QUIT",
};
static const char *MAIN_MENU_SUBS[] = {
    "begin a fresh save",
    "last save:  ch.4  17h 22m",
    "3 saved heroes",
    "graphics  audio  controls",
    "return to desktop",
};
#define MAIN_MENU_COUNT 5

void Scene_MainMenu(AppState *state, Rectangle area) {
    (void)state;
    DrawRectangleRec(area, COL_BG_DEEP);

    float leftW = area.width * 0.55f;
    Rectangle leftBox  = { area.x,          area.y, leftW,                  area.height };
    Rectangle rightBox = { area.x + leftW,  area.y, area.width - leftW,     area.height };

    // ----- Left column -----
    {
        float padL = 80, padR = 40, padT = 90;
        float x = leftBox.x + padL;
        float y = leftBox.y + padT;
        float maxW = leftBox.width - padL - padR;

        UI_DrawText("AETHERFALL", x, y, 84, 8, COL_ACCENT);
        y += 84 + 18;
        UI_DrawText("a forgotten realms revival", x, y, 18, 4, COL_TEXT_DIM);
        y += 18 + 24 + 18;

        DrawRectangleRec((Rectangle){ x, y, 72, 4 }, COL_ACCENT);
        y += 4 + 8 + 18;

        float yEnd = UI_DrawTextWrapped(
            "Three centuries after the Sundering, the old roads are open again. "
            "Few who walk them return. Fewer still remember why.",
            x, y, fminf(460, maxW), 14, 22, 0, COL_TEXT_DIM);
        (void)yEnd;

        // News strip pinned near the bottom of the left column.
        Rectangle news = {
            x, leftBox.y + leftBox.height - 40 - 14 - 18 - 12 - 12,
            fminf(460, maxW), 14 + 18 + 11 + 12 + 12,
        };
        UI_Panel(news, COL_PANEL,
            (UI_Border){ .color = COL_BORDER, .left = 3 }, 4);
        UI_DrawText("PATCH 0.4 - HOLLOW PEAKS",
            news.x + 14, news.y + 12, 11, 3, COL_ACCENT);
        UI_DrawTextWrapped(
            "New zone, balance pass for ranger, fixed the cave geometry bug.",
            news.x + 14, news.y + 12 + 11 + 6,
            news.width - 28, 12, 18, 0, COL_TEXT_DIM);
    }

    // ----- Right column: menu items -----
    {
        float padL = 40, padR = 100, padT = 40 + 24;
        float x = rightBox.x + padL;
        float y = rightBox.y + padT;
        float w = rightBox.width - padL - padR;
        float rowH = 64;
        float rowGap = 8;

        for (int i = 0; i < MAIN_MENU_COUNT; i++) {
            Rectangle row = { x, y, w, rowH };
            bool hover = UI_Hovered(row);
            UI_Panel(row, hover ? COL_PANEL_HOVER : COL_PANEL, (UI_Border){0}, 2);
            DrawRectangleRec(
                (Rectangle){ row.x, row.y, hover ? 6.0f : 2.0f, row.height },
                hover ? COL_ACCENT : COL_BORDER);

            Rectangle num = { row.x + 24, row.y + (row.height - 36) / 2.0f, 36, 36 };
            UI_Panel(num, hover ? COL_ACCENT : COL_BG_DEEP,
                (UI_Border){ .color = COL_BORDER, .left=1,.right=1,.top=1,.bottom=1 }, 0);
            UI_DrawTextCentered(UI_Fmt("0%d", i + 1), num, 14, 1,
                hover ? COL_BG_DEEP : COL_TEXT_DIM);

            float tx = num.x + num.width + 18;
            UI_DrawText(MAIN_MENU_ITEMS[i], tx, row.y + 16, 22, 4, COL_TEXT);
            UI_DrawText(MAIN_MENU_SUBS[i],  tx, row.y + 42, 11, 2, COL_TEXT_FAINT);

            y += rowH + rowGap;
        }

        const char *hint = "[ press 2 to view characters ]";
        int hw = UI_TextWidth(hint, 11, 2);
        UI_DrawText(hint, x + w - hw, rightBox.y + rightBox.height - 40 - 11, 11, 2, COL_TEXT_FAINT);
    }
}

// =====================================================================
// Scene 2: Character select  (+ new-hero modal form)
// =====================================================================

typedef struct {
    const char *name;
    const char *klass;
    int level;
    int hp, hpMax, mp, mpMax;
    int strength, dexterity, intellect, vitality;
    const char *bio;
    Color accent;
} CharacterSlot;

static const CharacterSlot CHARACTER_PRESETS[] = {
    {
        "BREA OF VARN", "Rune Paladin",
        14, 412, 480, 92, 160, 18, 11, 14, 17,
        "Bound to a half-remembered oath. Wields auroral light with the weight of a question.",
        { 230, 196, 110, 255 },
    },
    {
        "KAERIS", "Shadowblade",
        12, 280, 320, 140, 220, 9, 19, 16, 11,
        "Trained in three cities, exiled from all of them. Quiet, precise, never twice in the same alley.",
        { 168, 142, 220, 255 },
    },
    {
        "OREN BLACKMOOR", "Stormcaller",
        13, 240, 290, 360, 420, 8, 12, 21, 10,
        "Speaks to the air and listens carefully. The air has opinions and it does not always agree.",
        { 110, 196, 230, 255 },
    },
    {
        "VENN ASH", "Beastwarden",
        11, 360, 400, 80, 140, 16, 17, 10, 15,
        "Walks with a wolf called nothing. Knows the names of seventeen northern berries and which ones lie.",
        { 156, 220, 122, 255 },
    },
};
#define PRESET_COUNT    ((int)(sizeof(CHARACTER_PRESETS)/sizeof(CHARACTER_PRESETS[0])))
#define MAX_CHARACTERS  16

static CharacterSlot g_characters[MAX_CHARACTERS];
static int           g_character_count = 0;
static char          g_custom_names[MAX_CHARACTERS][24];
static int           g_custom_used = 0;

static const char *NEW_HERO_CLASSES[] = {
    "Wayfarer", "Knight", "Stormcaller", "Shadowblade", "Beastwarden",
};
#define NEW_HERO_CLASS_COUNT ((int)(sizeof(NEW_HERO_CLASSES)/sizeof(NEW_HERO_CLASSES[0])))

static const Color NEW_HERO_ACCENTS[] = {
    { 230, 196, 110, 255 },
    { 196,  82,  82, 255 },
    { 110, 196, 230, 255 },
    { 168, 142, 220, 255 },
    { 156, 220, 122, 255 },
};
#define NEW_HERO_ACCENT_COUNT ((int)(sizeof(NEW_HERO_ACCENTS)/sizeof(NEW_HERO_ACCENTS[0])))

static const char *NEW_HERO_BIO =
    "Newly forged. Untested. Brave or foolish; the long road will sort which.";

static void EnsureCharactersInitialized(void) {
    if (g_character_count > 0) return;
    for (int i = 0; i < PRESET_COUNT; i++) g_characters[i] = CHARACTER_PRESETS[i];
    g_character_count = PRESET_COUNT;
}

static void HeroFormCommit(AppState *state) {
    while (state->newHeroNameLen > 0 && state->newHeroNameBuf[state->newHeroNameLen - 1] == ' ') {
        state->newHeroNameBuf[--state->newHeroNameLen] = '\0';
    }
    if (state->newHeroNameLen == 0) return;
    if (g_character_count >= MAX_CHARACTERS) return;

    int slot = g_custom_used++;
    if (slot >= MAX_CHARACTERS) slot = MAX_CHARACTERS - 1;
    int n = state->newHeroNameLen;
    if (n > (int)sizeof(g_custom_names[0]) - 1) n = sizeof(g_custom_names[0]) - 1;
    memcpy(g_custom_names[slot], state->newHeroNameBuf, n);
    g_custom_names[slot][n] = '\0';

    CharacterSlot c = {
        .name = g_custom_names[slot],
        .klass = NEW_HERO_CLASSES[state->newHeroClass],
        .level = 1,
        .hp = 100, .hpMax = 100,
        .mp =  40, .mpMax =  40,
        .strength  = state->newHeroStats[0],
        .dexterity = state->newHeroStats[1],
        .intellect = state->newHeroStats[2],
        .vitality  = state->newHeroStats[3],
        .bio = NEW_HERO_BIO,
        .accent = NEW_HERO_ACCENTS[state->newHeroAccent],
    };
    g_characters[g_character_count] = c;
    state->selectedCharacter = g_character_count;
    g_character_count++;
    state->newHeroFormOpen = false;
}

// Plain ASCII text-input edge handling. raygui has GuiTextBox but it
// captures focus differently; clay-ui's form types directly so we mirror it.
static void HeroFormConsumeTextInput(AppState *state) {
    if (IsKeyPressed(KEY_BACKSPACE) && state->newHeroNameLen > 0) {
        state->newHeroNameBuf[--state->newHeroNameLen] = '\0';
    }
    if (IsKeyDown(KEY_BACKSPACE)) {
        static float held = 0;
        held += GetFrameTime();
        if (held > 0.4f) {
            held -= 0.05f;
            if (state->newHeroNameLen > 0)
                state->newHeroNameBuf[--state->newHeroNameLen] = '\0';
        }
    }
    int ch;
    while ((ch = GetCharPressed()) != 0) {
        if (ch >= 32 && ch < 127 &&
            state->newHeroNameLen < (int)sizeof(state->newHeroNameBuf) - 1) {
            state->newHeroNameBuf[state->newHeroNameLen++] = (char)ch;
            state->newHeroNameBuf[state->newHeroNameLen] = '\0';
        }
    }
}

void Scene_Character(AppState *state, Rectangle area) {
    EnsureCharactersInitialized();
    if (state->selectedCharacter < 0 || state->selectedCharacter >= g_character_count) {
        state->selectedCharacter = 0;
    }
    const CharacterSlot *sel = &g_characters[state->selectedCharacter];

    DrawRectangleRec(area, COL_BG_DEEP);
    float padX = 32, padTop = 28, padBot = 28;
    float x = area.x + padX;
    float y = area.y + padTop;
    float innerW = area.width - padX * 2;
    float innerH = area.height - padTop - padBot;

    // Title row
    UI_DrawText("CHOOSE YOUR HERO", x, y, 26, 6, COL_ACCENT);
    const char *slotStr = UI_Fmt("slot %d of %d",
        state->selectedCharacter + 1, g_character_count);
    int sw = UI_TextWidth(slotStr, 12, 2);
    UI_DrawText(slotStr, x + innerW - sw, y + 12, 12, 2, COL_TEXT_FAINT);
    y += 26 + 6;
    DrawRectangleRec((Rectangle){ x, y, 64, 3 }, COL_ACCENT);
    y += 3 + 18;

    float remainingH = (area.y + padTop + innerH) - y;
    float gap = 22;
    float listW = 320;
    Rectangle listArea   = { x,                       y, listW,                          remainingH };
    Rectangle detailArea = { x + listW + gap,         y, innerW - listW - gap,           remainingH };

    // ----- Left list -----
    {
        float ly = listArea.y;
        for (int i = 0; i < g_character_count; i++) {
            Rectangle card = { listArea.x, ly, listArea.width, 80 };
            const CharacterSlot *c = &g_characters[i];
            bool active = (state->selectedCharacter == i);
            bool hover  = UI_Hovered(card);

            Color bg = active ? COL_PANEL_HOVER : (hover ? COL_PANEL : COL_BG);
            float bw = active ? 2.0f : 1.0f;
            UI_Panel(card, bg,
                (UI_Border){
                    .color = active ? c->accent : COL_BORDER,
                    .left = bw, .right = bw, .top = bw, .bottom = bw,
                }, 4);

            // Portrait
            Rectangle portrait = { card.x + 14, card.y + 12, 56, 56 };
            UI_Panel(portrait, COL_BG_DEEP,
                (UI_Border){ .color = c->accent, .left=2,.right=2,.top=2,.bottom=2 }, 2);
            UI_DrawTextCentered(UI_Fmt("%c", c->name[0]), portrait, 28, 2, c->accent);

            float tx = portrait.x + portrait.width + 14;
            UI_DrawText(c->name,                tx, card.y + 14, 14, 2, COL_TEXT);
            UI_DrawText(c->klass,               tx, card.y + 34, 11, 1, COL_TEXT_DIM);
            UI_DrawText(UI_Fmt("LV %d", c->level), tx, card.y + 52, 10, 2, COL_ACCENT);

            if (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !state->newHeroFormOpen) {
                state->selectedCharacter = i;
            }

            ly += 80 + 10;
        }

        // "+ new hero" row at the bottom of the list area
        Rectangle newSlot = { listArea.x, listArea.y + listArea.height - 44,
                              listArea.width, 44 };
        bool nh = UI_Hovered(newSlot);
        UI_Panel(newSlot, nh ? COL_PANEL : COL_BG,
            (UI_Border){ .color = COL_BORDER, .left=1,.right=1,.top=1,.bottom=1 }, 2);
        UI_DrawTextCentered("+   NEW HERO", newSlot, 13, 4,
            nh ? COL_ACCENT : COL_TEXT_FAINT);

        if (nh && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !state->newHeroFormOpen
            && g_character_count < MAX_CHARACTERS) {
            state->newHeroFormOpen   = true;
            state->newHeroNameLen    = 0;
            state->newHeroNameBuf[0] = '\0';
            state->newHeroClass      = 0;
            state->newHeroAccent     = 0;
            state->newHeroStats[0] = state->newHeroStats[1] =
            state->newHeroStats[2] = state->newHeroStats[3] = 10;
        }
    }

    // ----- Right detail -----
    {
        UI_Panel(detailArea, COL_PANEL,
            (UI_Border){ .color = sel->accent, .top = 2 }, 4);
        float padIn = 28;
        float dx = detailArea.x + 32;
        float dy = detailArea.y + 28;
        float dw = detailArea.width - 32 - 32;

        // Big banner
        Rectangle bigP = { dx, dy, 110, 110 };
        UI_Panel(bigP, COL_BG_DEEP,
            (UI_Border){ .color = sel->accent, .left=3,.right=3,.top=3,.bottom=3 }, 0);
        UI_DrawTextCentered(UI_Fmt("%c", sel->name[0]), bigP, 64, 2, sel->accent);

        UI_DrawText(sel->name, bigP.x + bigP.width + 16, bigP.y + 36, 32, 4, COL_TEXT);
        UI_DrawText(UI_Fmt("LEVEL %d  -  %s", sel->level, sel->klass),
            bigP.x + bigP.width + 16, bigP.y + 76, 13, 3, sel->accent);

        float by = bigP.y + bigP.height + 16;
        by = UI_DrawTextWrapped(sel->bio, dx, by, dw, 13, 22, 0, COL_TEXT_DIM);
        by += 8;

        // Resources (HP, MP)
        StatBar("HEALTH", (float)sel->hp, (float)sel->hpMax, COL_HP, (Rectangle){ dx, by, dw, 28 });
        by += 36;
        StatBar("MANA",   (float)sel->mp, (float)sel->mpMax, COL_MP, (Rectangle){ dx, by, dw, 28 });
        by += 36 + 4;
        DrawRectangleRec((Rectangle){ dx, by, dw, 1 }, COL_BORDER);
        by += 1 + 8;

        // Stat grid (4 boxes in a row)
        int gridGap = 12;
        float boxW = (dw - gridGap * 3) / 4.0f;
        const char *labels[4] = { "STR", "DEX", "INT", "VIT" };
        int stats[4] = { sel->strength, sel->dexterity, sel->intellect, sel->vitality };
        for (int s = 0; s < 4; s++) {
            Rectangle b = { dx + (boxW + gridGap) * s, by, boxW, 70 };
            UI_Panel(b, COL_BG,
                (UI_Border){ .color = COL_BORDER, .left=1,.right=1,.top=1,.bottom=1 }, 2);
            UI_DrawText(labels[s], b.x + 12, b.y + 10, 11, 3, COL_TEXT_FAINT);
            UI_DrawText(UI_Fmt("%d", stats[s]), b.x + 12, b.y + 28, 28, 1, COL_TEXT);
        }

        // Action buttons at the bottom of the detail panel
        float btnY = detailArea.y + detailArea.height - 28 - 40;
        // Delete
        const char *delLabel = "DELETE";
        int delW = UI_TextWidth(delLabel, 12, 4) + 44;
        Rectangle delBtn = { dx, btnY, (float)delW, 40 };
        bool delHover = UI_Hovered(delBtn);
        UI_Panel(delBtn, delHover ? COL_DANGER : COL_BG,
            (UI_Border){ .color = COL_DANGER, .left=1,.right=1,.top=1,.bottom=1 }, 0);
        UI_DrawTextCentered(delLabel, delBtn, 12, 4,
            delHover ? COL_TEXT : COL_DANGER);

        // Enter the world
        Rectangle entBtn = { dx + dw - 220, btnY, 220, 40 };
        bool entHover = UI_Hovered(entBtn);
        UI_Panel(entBtn, entHover ? COL_ACCENT : COL_ACCENT_DIM, (UI_Border){0}, 0);
        UI_DrawTextCentered("ENTER THE WORLD  >", entBtn, 13, 4, COL_BG_DEEP);
    }

    if (state->newHeroFormOpen) NewHeroForm(state);
}

static void NewHeroForm(AppState *state) {
    UI_SetModalActive(true);
    HeroFormConsumeTextInput(state);
    if (IsKeyPressed(KEY_ESCAPE)) {
        state->newHeroFormOpen = false;
        UI_SetModalActive(false);
        return;
    }

    Color accent = NEW_HERO_ACCENTS[state->newHeroAccent];

    // Full-screen dim
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    DrawRectangleRec((Rectangle){ 0, 0, (float)sw, (float)sh }, (Color){ 6, 8, 14, 210 });

    float cardW = 560, cardH = 520;
    Rectangle card = {
        (sw - cardW) / 2.0f, (sh - cardH) / 2.0f, cardW, cardH,
    };
    UI_Panel(card, COL_BG,
        (UI_Border){ .color = accent, .top = 3 }, 2);

    float padX = 32;
    float x = card.x + padX;
    float y = card.y + 28;
    float w = card.width - padX * 2;

    // Header
    UI_DrawText("FORGE A NEW HERO", x, y, 24, 6, accent);  y += 24 + 4;
    UI_DrawText("a new soul on the long road", x, y, 10, 3, COL_TEXT_FAINT);
    y += 10 + 18;

    // Name input field
    UI_DrawText("NAME", x, y, 10, 3, COL_TEXT_DIM);
    y += 10 + 6;
    Rectangle nameBox = { x, y, w, 36 };
    UI_Panel(nameBox, COL_BG_DEEP,
        (UI_Border){ .color = accent, .bottom = 2 }, 0);
    {
        bool empty = (state->newHeroNameLen == 0);
        bool cursorOn = (((int)(state->time * 2.0f)) & 1) != 0;
        const char *shown = empty ? "type a name..." : state->newHeroNameBuf;
        UI_DrawText(shown, nameBox.x + 12, nameBox.y + (nameBox.height - 16) / 2.0f,
            16, 1, empty ? COL_TEXT_FAINT : COL_TEXT);
        if (!empty && cursorOn) {
            int tw = UI_TextWidth(state->newHeroNameBuf, 16, 1);
            UI_DrawText("_", nameBox.x + 12 + tw, nameBox.y + (nameBox.height - 16) / 2.0f,
                16, 0, accent);
        }
    }
    y += 36 + 18;

    // Class stepper
    {
        UI_DrawText("CLASS", x, y + 9, 10, 3, COL_TEXT_DIM);
        Rectangle prev = { x + 80, y, 30, 30 };
        Rectangle next = { x + w - 30, y, 30, 30 };
        Rectangle label = { prev.x + 30 + 14, y, next.x - (prev.x + 30 + 14) - 14, 30 };

        bool ph = UI_Hovered(prev), nh = UI_Hovered(next);
        UI_Panel(prev, ph ? accent : COL_BG_DEEP,
            (UI_Border){ .color = COL_BORDER, .left=1,.right=1,.top=1,.bottom=1 }, 0);
        UI_DrawTextCentered("<", prev, 16, 0, ph ? COL_BG_DEEP : COL_TEXT_DIM);
        UI_Panel(next, nh ? accent : COL_BG_DEEP,
            (UI_Border){ .color = COL_BORDER, .left=1,.right=1,.top=1,.bottom=1 }, 0);
        UI_DrawTextCentered(">", next, 16, 0, nh ? COL_BG_DEEP : COL_TEXT_DIM);
        UI_DrawTextCentered(NEW_HERO_CLASSES[state->newHeroClass], label, 16, 2, accent);

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (ph) state->newHeroClass =
                (state->newHeroClass - 1 + NEW_HERO_CLASS_COUNT) % NEW_HERO_CLASS_COUNT;
            if (nh) state->newHeroClass = (state->newHeroClass + 1) % NEW_HERO_CLASS_COUNT;
        }
    }
    y += 30 + 18;

    // Accent swatches
    {
        UI_DrawText("ACCENT", x, y + 7, 10, 3, COL_TEXT_DIM);
        float sx = x + 80;
        for (int a = 0; a < NEW_HERO_ACCENT_COUNT; a++) {
            bool active = (state->newHeroAccent == a);
            float sz = active ? 30 : 26;
            Rectangle sw = { sx, y + (30 - sz) / 2.0f, sz, sz };
            float radius = sz / 2.0f;
            DrawCircle((int)(sw.x + radius), (int)(sw.y + radius), radius, NEW_HERO_ACCENTS[a]);
            if (active) {
                DrawCircleLines((int)(sw.x + radius), (int)(sw.y + radius), radius, COL_TEXT);
                DrawCircleLines((int)(sw.x + radius), (int)(sw.y + radius), radius - 1, COL_TEXT);
            }
            if (UI_Hovered(sw) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                state->newHeroAccent = a;
            }
            sx += sz + 14;
        }
    }
    y += 30 + 18;

    // Stats header
    UI_DrawText("STATS", x, y, 10, 3, COL_TEXT_DIM);
    {
        int total = state->newHeroStats[0] + state->newHeroStats[1] +
                    state->newHeroStats[2] + state->newHeroStats[3];
        const char *t = UI_Fmt("total %d  /  start 40", total);
        int tw = UI_TextWidth(t, 10, 2);
        UI_DrawText(t, x + w - tw, y, 10, 2, COL_TEXT_FAINT);
    }
    y += 10 + 6;

    const char *statLabels[4] = { "STR", "DEX", "INT", "VIT" };
    for (int s = 0; s < 4; s++) {
        UI_DrawText(statLabels[s], x + 4, y + 10, 12, 3, COL_TEXT_DIM);
        Rectangle dec = { x + 50, y + 3, 26, 26 };
        Rectangle inc = { x + 50 + 26 + 10 + 40 + 10, y + 3, 26, 26 };
        Rectangle val = { dec.x + 26 + 10, y + 3, 40, 26 };

        bool dh = UI_Hovered(dec), ih = UI_Hovered(inc);
        UI_Panel(dec, dh ? accent : COL_BG_DEEP,
            (UI_Border){ .color = COL_BORDER, .left=1,.right=1,.top=1,.bottom=1 }, 0);
        UI_DrawTextCentered("-", dec, 14, 0, dh ? COL_BG_DEEP : COL_TEXT_DIM);
        UI_DrawTextCentered(UI_Fmt("%d", state->newHeroStats[s]), val, 18, 0, COL_TEXT);
        UI_Panel(inc, ih ? accent : COL_BG_DEEP,
            (UI_Border){ .color = COL_BORDER, .left=1,.right=1,.top=1,.bottom=1 }, 0);
        UI_DrawTextCentered("+", inc, 14, 0, ih ? COL_BG_DEEP : COL_TEXT_DIM);

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (dh && state->newHeroStats[s] > 8)  state->newHeroStats[s]--;
            if (ih && state->newHeroStats[s] < 20) state->newHeroStats[s]++;
        }

        // Mini bar
        Rectangle mini = { inc.x + 26 + 14, y + 12, x + w - (inc.x + 26 + 14), 6 };
        UI_Panel(mini, COL_BG_DEEP,
            (UI_Border){ .color = COL_BORDER, .left=1,.right=1,.top=1,.bottom=1 }, 2);
        float pct = (state->newHeroStats[s] - 8) / 12.0f;
        if (pct < 0) pct = 0;
        if (pct > 1) pct = 1;
        DrawRectangleRec(
            (Rectangle){ mini.x, mini.y, mini.width * pct, mini.height }, accent);

        y += 32;
    }
    y += 8;
    DrawRectangleRec((Rectangle){ x, y, w, 1 }, COL_BORDER);
    y += 1 + 16;

    // Buttons
    Rectangle cancelBtn = { x, y, 120, 38 };
    bool ch = UI_Hovered(cancelBtn);
    UI_Panel(cancelBtn, ch ? COL_PANEL_HOVER : COL_BG,
        (UI_Border){ .color = COL_BORDER, .left=1,.right=1,.top=1,.bottom=1 }, 0);
    UI_DrawTextCentered("CANCEL", cancelBtn, 12, 4, COL_TEXT_DIM);

    bool canForge = (state->newHeroNameLen > 0);
    Rectangle forgeBtn = { x + w - 220, y, 220, 38 };
    bool fh = UI_Hovered(forgeBtn);
    UI_Panel(forgeBtn,
        !canForge ? COL_BG : (fh ? accent : NEW_HERO_ACCENTS[state->newHeroAccent]),
        (UI_Border){ .color = canForge ? accent : COL_BORDER, .left=1,.right=1,.top=1,.bottom=1 }, 0);
    UI_DrawTextCentered("FORGE THE HERO  >", forgeBtn, 13, 4,
        canForge ? COL_BG_DEEP : COL_TEXT_FAINT);

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (ch) { state->newHeroFormOpen = false; UI_SetModalActive(false); return; }
        if (fh && canForge) { HeroFormCommit(state); UI_SetModalActive(false); return; }
    }

    UI_SetModalActive(false);
}

// =====================================================================
// Scene 3: Inventory
// =====================================================================

typedef struct {
    const char *name;
    const char *type;
    int qty;
    int tier;
    const char *flavor;
} Item;

static const char *EQUIP_SLOTS[] = {
    "HEAD","CLOAK","CHEST","HANDS","WAIST","LEGS","FEET",
    "MAIN HAND","OFF HAND","RING I","RING II","AMULET",
};
static const char *EQUIP_VALUES[] = {
    "Helm of the Pale Watch","Travelworn Mantle","Brigandine of Saints",
    "Ironweave Gauntlets","Buckler Belt","Greaves of Vahn","Boots of the Long Road",
    "Sunsilver Longsword","Aegis of Morn","Ring of the Quiet Hour",
    "-- empty --","Verdant Sigil",
};
#define EQUIP_COUNT ((int)(sizeof(EQUIP_SLOTS)/sizeof(EQUIP_SLOTS[0])))

static const Item INV_ITEMS[] = {
    { "Greatsword of Dawn",   "Two-Handed", 1,  3, "It hums faintly at sunrise. The sound of it is older than any tongue still spoken." },
    { "Pale Health Tonic",    "Consumable", 12, 0, "Restores 120 HP over 6s. Tastes of salt and pennies." },
    { "Verdant Sigil",        "Amulet",     1,  2, "+8 INT, +4 VIT. Glows faintly near old growth." },
    { "Bone Arrow",           "Ammunition", 84, 0, "Hand-fletched. Pulls a little left." },
    { "Sunsilver Longsword",  "One-Handed", 1,  4, "Legendary. Returned, somehow, from beneath Hollow Peaks." },
    { "Ration",               "Consumable", 6,  0, "Hard biscuit, smoked fish, a little salt." },
    { "Throwing Knife",       "Throwable",  9,  1, "Balanced for the off-hand. +12 DEX while equipped." },
    { "Manaroot Vial",        "Consumable", 4,  1, "Restores 80 MP instantly. Use sparingly: it leaves an ache." },
    { "Ironweave Gauntlets",  "Hands",      1,  2, "+12 ARM. Tightens of its own accord in a fight." },
    { "Letter to Aren",       "Quest",      1,  2, "Sealed in green wax. Do not open. (You have opened it.)" },
    { "Wolf Pelt",            "Material",   3,  0, "Tradeable. Worth more in Varnholm than here." },
    { "Lockpick",             "Tool",       7,  0, "Snaps on a 1." },
    { "Tomb-Glass Shard",     "Material",   2,  3, "Cold to the touch. The reflection arrives a moment late." },
    { "Ember Dust",           "Reagent",    14, 1, "Component for fire weaving. Keep away from breath." },
    { "Cracked Compass",      "Trinket",    1,  0, "Points west. Always west. You are not always going west." },
    { "Helm of the Pale Watch","Head",      1,  3, "Once worn by the last sentinel of Morn. He came down the hill alone." },
    { "Travelworn Mantle",    "Cloak",      1,  1, "+6 ARM. Smells faintly of cedar smoke." },
    { "Brigandine of Saints", "Chest",      1,  3, "Each rivet is etched with a name. None of them yours." },
    { "Buckler Belt",         "Waist",      1,  1, "+4 ARM, +2 DEX." },
    { "Greaves of Vahn",      "Legs",       1,  2, "Worn by a saint who walked the long road twice." },
};
#define INV_COUNT ((int)(sizeof(INV_ITEMS)/sizeof(INV_ITEMS[0])))

static int ItemCategory(const char *type) {
    if (!strcmp(type,"Two-Handed") || !strcmp(type,"One-Handed") ||
        !strcmp(type,"Throwable")  || !strcmp(type,"Ammunition")) return 1;
    if (!strcmp(type,"Head") || !strcmp(type,"Cloak") || !strcmp(type,"Chest") ||
        !strcmp(type,"Hands")|| !strcmp(type,"Waist") || !strcmp(type,"Legs")  ||
        !strcmp(type,"Feet") || !strcmp(type,"Off Hand")||
        !strcmp(type,"Amulet")||!strcmp(type,"Ring"))                   return 2;
    if (!strcmp(type,"Consumable")) return 3;
    if (!strcmp(type,"Quest"))      return 4;
    return 5;
}
static bool ItemMatchesFilter(const Item *it, int filter) {
    return filter == 0 || ItemCategory(it->type) == filter;
}
static Color TierColor(int tier) {
    switch (tier) {
        case 0: return COL_TEXT_DIM;
        case 1: return (Color){ 156, 220, 122, 255 };
        case 2: return (Color){ 110, 196, 230, 255 };
        case 3: return (Color){ 196, 130, 230, 255 };
        case 4: default: return COL_ACCENT;
    }
}
static const char *TierLabel(int tier) {
    switch (tier) {
        case 0: return "COMMON";
        case 1: return "UNCOMMON";
        case 2: return "RARE";
        case 3: return "EPIC";
        case 4: default: return "LEGENDARY";
    }
}

void Scene_Inventory(AppState *state, Rectangle area) {
    if (state->selectedInventoryItem < 0) state->selectedInventoryItem = 0;
    if (state->selectedInventoryItem >= INV_COUNT) state->selectedInventoryItem = INV_COUNT - 1;

    DrawRectangleRec(area, COL_BG_DEEP);
    float pad = 28;
    float x = area.x + pad, y = area.y + 26;
    float w = area.width - pad * 2;
    float h = area.height - 26 - 26;

    float gap = 18;
    float equipW = 240, detailW = 280;
    float listW  = w - equipW - detailW - gap * 2;

    Rectangle equipArea  = { x,                            y, equipW,  h };
    Rectangle listArea   = { x + equipW + gap,             y, listW,   h };
    Rectangle detailArea = { x + equipW + listW + gap*2,   y, detailW, h };

    // ----- Equip column -----
    {
        UI_Panel(equipArea, COL_PANEL,
            (UI_Border){ .color = COL_ACCENT, .top = 2 }, 4);
        float ex = equipArea.x + 18;
        float ey = equipArea.y + 18;
        float ew = equipArea.width - 36;
        UI_DrawText("EQUIPMENT", ex, ey, 16, 4, COL_ACCENT);  ey += 16 + 6;
        UI_DrawText("brea of varn  -  rune paladin", ex, ey, 11, 2, COL_TEXT_FAINT);
        ey += 11 + 10;

        for (int i = 0; i < EQUIP_COUNT; i++) {
            Rectangle row = { ex, ey, ew, 28 };
            bool empty = (EQUIP_VALUES[i][0] == '-');
            bool hover = UI_Hovered(row);
            UI_Panel(row, hover ? COL_PANEL_HOVER : COL_BG, (UI_Border){0}, 2);
            DrawCircle((int)(row.x + 10 + 3), (int)(row.y + row.height / 2.0f), 3,
                empty ? COL_TEXT_FAINT : COL_ACCENT);
            UI_DrawText(EQUIP_SLOTS[i],  row.x + 26, row.y + (row.height - 10) / 2.0f, 10, 3, COL_TEXT_FAINT);
            UI_DrawText(EQUIP_VALUES[i], row.x + 26 + 70, row.y + (row.height - 11) / 2.0f,
                11, 0, empty ? COL_TEXT_FAINT : COL_TEXT);
            ey += 28 + 4;
        }

        // Aggregate stats footer
        float footerH = 12 + 10 + 6 + 10;
        Rectangle agg = { ex, equipArea.y + equipArea.height - 18 - footerH - 4,
                          ew, footerH + 16 };
        UI_Panel(agg, COL_BG_DEEP,
            (UI_Border){ .color = COL_BORDER, .left=1,.right=1,.top=1,.bottom=1 }, 3);
        UI_DrawText("ARMOR  142   |   DPS  64   |   WEIGHT  38 / 80",
            agg.x + 12, agg.y + 10, 10, 2, COL_TEXT_DIM);
        UI_DrawText("set bonus:  warden's vigil (2/4)",
            agg.x + 12, agg.y + 10 + 12, 10, 1, COL_ACCENT);
    }

    // ----- List column (filter pills + scroll list) -----
    {
        float lx = listArea.x, ly = listArea.y, lw = listArea.width;
        UI_DrawText("INVENTORY", lx + 4, ly, 16, 4, COL_ACCENT);
        const char *info = UI_Fmt("%d items   .   13.4 / 30.0 kg", INV_COUNT);
        int iw = UI_TextWidth(info, 11, 2);
        UI_DrawText(info, lx + lw - iw - 4, ly + 4, 11, 2, COL_TEXT_FAINT);
        ly += 16 + 10 + 10;

        // Filter pills
        const char *pills[] = { "ALL", "WEAPONS", "ARMOR", "CONSUM.", "QUEST", "MISC" };
        int pillCount = (int)(sizeof(pills)/sizeof(pills[0]));
        float px = lx + 4;
        for (int p = 0; p < pillCount; p++) {
            int tw = UI_TextWidth(pills[p], 10, 2);
            Rectangle pill = { px, ly, (float)tw + 24, 24 };
            bool active = (state->inventoryFilter == p);
            bool hover  = UI_Hovered(pill);
            UI_Panel(pill,
                active ? COL_ACCENT : (hover ? COL_PANEL_HOVER : COL_PANEL),
                (UI_Border){0}, 2);
            UI_DrawTextCentered(pills[p], pill, 10, 2, active ? COL_BG_DEEP : COL_TEXT_DIM);
            if (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                state->inventoryFilter = p;
            }
            px += pill.width + 8;
        }
        ly += 24 + 10;

        // Scrollable list
        float scrollH = listArea.y + listArea.height - ly;
        Rectangle scroll = { lx, ly, lw, scrollH };
        UI_Panel(scroll, COL_PANEL, (UI_Border){0}, 4);

        // Compute total content height for visible rows
        int visibleCount = 0;
        for (int i = 0; i < INV_COUNT; i++) {
            if (ItemMatchesFilter(&INV_ITEMS[i], state->inventoryFilter)) visibleCount++;
        }
        float rowH = 46;
        float totalH = visibleCount * rowH + 4;
        float maxScroll = totalH - scroll.height;
        if (maxScroll < 0) maxScroll = 0;

        // Mouse wheel scroll
        if (UI_Hovered(scroll)) {
            float wheel = GetMouseWheelMove();
            state->inventoryScroll -= wheel * 32.0f;
        }
        if (state->inventoryScroll < 0) state->inventoryScroll = 0;
        if (state->inventoryScroll > maxScroll) state->inventoryScroll = maxScroll;

        BeginScissorMode((int)scroll.x, (int)scroll.y, (int)scroll.width, (int)scroll.height);
        state->hoveredInventoryItem = -1;
        float ry = scroll.y + 2 - state->inventoryScroll;
        for (int i = 0; i < INV_COUNT; i++) {
            const Item *it = &INV_ITEMS[i];
            if (!ItemMatchesFilter(it, state->inventoryFilter)) continue;
            Rectangle row = { scroll.x + 2, ry, scroll.width - 4, rowH };
            // Only handle hover if row is on-screen
            bool onScreen = (row.y + row.height >= scroll.y && row.y <= scroll.y + scroll.height);
            bool hover = onScreen && UI_Hovered(row) && UI_Hovered(scroll);
            bool selected = (state->selectedInventoryItem == i);

            Color bg = selected ? COL_PANEL_HOVER : (hover ? COL_BG : (Color){0,0,0,0});
            UI_Panel(row, bg, (UI_Border){0}, 0);
            if (selected) {
                DrawRectangleRec((Rectangle){ row.x, row.y, 4, row.height }, TierColor(it->tier));
            }
            DrawCircle((int)(row.x + 14 + 4), (int)(row.y + row.height / 2.0f), 4, TierColor(it->tier));

            UI_DrawText(it->name, row.x + 14 + 14, row.y + 10, 13, 0, TierColor(it->tier));
            UI_DrawText(it->type, row.x + 14 + 14, row.y + 28, 10, 2, COL_TEXT_FAINT);
            const char *qty = UI_Fmt("x%d", it->qty);
            int qw = UI_TextWidth(qty, 13, 1);
            UI_DrawText(qty, row.x + row.width - qw - 14, row.y + 16, 13, 1, COL_TEXT_DIM);

            if (hover) state->hoveredInventoryItem = i;
            if (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                state->selectedInventoryItem = i;
            }

            ry += rowH;
        }
        EndScissorMode();

        // Scrollbar indicator on the right edge
        if (maxScroll > 0) {
            float thumbH = scroll.height * (scroll.height / totalH);
            if (thumbH < 24) thumbH = 24;
            float t = state->inventoryScroll / maxScroll;
            float thumbY = scroll.y + (scroll.height - thumbH) * t;
            DrawRectangleRec((Rectangle){ scroll.x + scroll.width - 4, thumbY, 3, thumbH },
                COL_BORDER);
        }
    }

    // ----- Detail column -----
    {
        const Item *d = &INV_ITEMS[state->selectedInventoryItem];
        UI_Panel(detailArea, COL_PANEL,
            (UI_Border){ .color = TierColor(d->tier), .top = 3 }, 4);
        float dpad = 22;
        float dx = detailArea.x + dpad;
        float dy = detailArea.y + dpad;
        float dw = detailArea.width - dpad * 2;

        Rectangle icon = { dx, dy, dw, 120 };
        UI_Panel(icon, COL_BG_DEEP,
            (UI_Border){ .color = TierColor(d->tier), .left=2,.right=2,.top=2,.bottom=2 }, 3);
        UI_DrawTextCentered(UI_Fmt("%c", d->name[0]), icon, 72, 0, TierColor(d->tier));
        dy += 120 + 14;

        UI_DrawText(d->name, dx, dy, 20, 2, COL_TEXT);
        dy += 20 + 14;

        UI_DrawText(TierLabel(d->tier), dx, dy, 10, 3, TierColor(d->tier));
        int tw = UI_TextWidth(TierLabel(d->tier), 10, 3);
        UI_DrawText("|", dx + tw + 10, dy, 10, 0, COL_BORDER);
        UI_DrawText(d->type, dx + tw + 22, dy, 10, 2, COL_TEXT_DIM);
        dy += 10 + 14;

        DrawRectangleRec((Rectangle){ dx, dy, dw, 1 }, COL_BORDER);
        dy += 1 + 14;

        dy = UI_DrawTextWrapped(d->flavor, dx, dy, dw, 12, 20, 0, COL_TEXT_DIM);

        // Action buttons stacked at bottom
        const char *actions[3] = { "EQUIP", "USE", "DROP" };
        float by = detailArea.y + detailArea.height - dpad - 3 * 34 - 2 * 8;
        for (int a = 0; a < 3; a++) {
            Rectangle btn = { dx, by, dw, 34 };
            bool hover = UI_Hovered(btn);
            Color bg, bord, txt;
            if (a == 0) {
                bg   = hover ? COL_ACCENT : COL_ACCENT_DIM;
                bord = COL_ACCENT;
                txt  = COL_BG_DEEP;
            } else {
                bg   = hover ? COL_PANEL_HOVER : COL_BG;
                bord = COL_BORDER;
                txt  = COL_TEXT_DIM;
            }
            UI_Panel(btn, bg,
                (UI_Border){ .color = bord, .left=1,.right=1,.top=1,.bottom=1 }, 0);
            UI_DrawTextCentered(actions[a], btn, 12, 4, txt);
            by += 34 + 8;
        }
    }
}

// =====================================================================
// Scene 4: Settings
// =====================================================================

static const char *SETTINGS_TABS[SETTINGS_TAB_COUNT] = {
    "GRAPHICS", "AUDIO", "CONTROLS",
};

static void RowToggle(int id, const char *label, const char *help, bool *value, Rectangle row) {
    (void)id;
    bool hover = UI_Hovered(row);
    UI_Panel(row, hover ? COL_PANEL_HOVER : COL_PANEL,
        (UI_Border){ .color = COL_BORDER, .bottom = 1 }, 2);
    UI_DrawText(label, row.x + 16, row.y + 14, 14, 2, COL_TEXT);
    UI_DrawText(help,  row.x + 16, row.y + 36, 10, 1, COL_TEXT_FAINT);

    Rectangle pill = { row.x + row.width - 16 - 46,
                       row.y + (row.height - 22) / 2.0f, 46, 22 };
    UI_Panel(pill, *value ? COL_ACCENT : COL_BG_DEEP,
        (UI_Border){
            .color = *value ? COL_ACCENT : COL_BORDER,
            .left=1,.right=1,.top=1,.bottom=1,
        }, 11);
    Rectangle knob = {
        pill.x + (*value ? pill.width - 16 - 3 : 3),
        pill.y + 3, 16, 16,
    };
    DrawCircle((int)(knob.x + 8), (int)(knob.y + 8), 8, COL_BG_DEEP);

    if (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) *value = !*value;
}

static void RowSlider(int id, const char *label, float *value, Rectangle row) {
    (void)id;
    bool hover = UI_Hovered(row);
    UI_Panel(row, hover ? COL_PANEL_HOVER : COL_PANEL,
        (UI_Border){ .color = COL_BORDER, .bottom = 1 }, 0);
    UI_DrawText(label, row.x + 16, row.y + (row.height - 14) / 2.0f, 14, 2, COL_TEXT);

    float trackX = row.x + 16 + 170;
    float pctW   = 60 + 18;
    float trackW = row.x + row.width - 16 - pctW - trackX;
    Rectangle track = { trackX, row.y + (row.height - 8) / 2.0f, trackW, 8 };
    UI_Panel(track, COL_BG_DEEP,
        (UI_Border){ .color = COL_BORDER, .left=1,.right=1,.top=1,.bottom=1 }, 3);
    float v = *value;
    if (v < 0) v = 0;
    if (v > 1) v = 1;
    if (v * trackW >= 1) {
        Rectangle fill = { track.x, track.y, trackW * v, 8 };
        UI_Panel(fill, COL_ACCENT, (UI_Border){0}, 3);
    }
    const char *pct = UI_Fmt("%3d%%", (int)(v * 100.0f + 0.5f));
    int pw = UI_TextWidth(pct, 13, 1);
    UI_DrawText(pct,
        row.x + row.width - 16 - pw,
        row.y + (row.height - 13) / 2.0f,
        13, 1, COL_ACCENT);

    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && UI_Hovered(track)) {
        float t = (GetMousePosition().x - track.x) / track.width;
        if (t < 0) t = 0;
        if (t > 1) t = 1;
        *value = t;
    }
}

static void RowStepper(int id, const char *label, int *value, int min, int max,
                       const char **options, Rectangle row) {
    (void)id;
    bool hover = UI_Hovered(row);
    UI_Panel(row, hover ? COL_PANEL_HOVER : COL_PANEL,
        (UI_Border){ .color = COL_BORDER, .bottom = 1 }, 0);
    UI_DrawText(label, row.x + 16, row.y + (row.height - 14) / 2.0f, 14, 2, COL_TEXT);

    Rectangle right = { row.x + row.width - 16 - 28, row.y + (row.height - 28) / 2.0f, 28, 28 };
    Rectangle valBox= { right.x - 14 - 140, right.y, 140, 28 };
    Rectangle left  = { valBox.x - 14 - 28, right.y, 28, 28 };

    bool lh = UI_Hovered(left), rh = UI_Hovered(right);
    UI_Panel(left, lh ? COL_ACCENT : COL_BG,
        (UI_Border){ .color = COL_BORDER, .left=1,.right=1,.top=1,.bottom=1 }, 0);
    UI_DrawTextCentered("<", left, 14, 0, lh ? COL_BG_DEEP : COL_TEXT_DIM);
    UI_DrawTextCentered(options[*value], valBox, 13, 2, COL_ACCENT);
    UI_Panel(right, rh ? COL_ACCENT : COL_BG,
        (UI_Border){ .color = COL_BORDER, .left=1,.right=1,.top=1,.bottom=1 }, 0);
    UI_DrawTextCentered(">", right, 14, 0, rh ? COL_BG_DEEP : COL_TEXT_DIM);

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (lh && *value > min) (*value)--;
        if (rh && *value < max) (*value)++;
    }
}

void Scene_Settings(AppState *state, Rectangle area) {
    DrawRectangleRec(area, COL_BG_DEEP);
    float padX = 36, padTop = 32, padBot = 28;
    float x = area.x + padX;
    float y = area.y + padTop;
    float w = area.width - padX * 2;

    UI_DrawText("SETTINGS", x, y, 30, 8, COL_ACCENT);
    y += 30 + 8;
    DrawRectangleRec((Rectangle){ x, y, 80, 3 }, COL_ACCENT);
    y += 3 + 22;

    // Tabs
    {
        float tabW = 140;
        float tabGap = 4;
        Rectangle tabsRow = { x, y, w, 40 };
        for (int t = 0; t < SETTINGS_TAB_COUNT; t++) {
            Rectangle tab = { x + (tabW + tabGap) * t, y, tabW, 40 };
            bool active = (state->settingsTab == (SettingsTab)t);
            bool hover  = UI_Hovered(tab);
            UI_Panel(tab,
                active ? COL_PANEL : (hover ? COL_BG : (Color){0,0,0,0}),
                (UI_Border){ .color = COL_ACCENT, .bottom = active ? 3.0f : 0.0f }, 0);
            UI_DrawTextCentered(SETTINGS_TABS[t], tab, 13, 4,
                active ? COL_ACCENT : (hover ? COL_TEXT : COL_TEXT_DIM));
            if (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                state->settingsTab = (SettingsTab)t;
            }
        }
        DrawRectangleRec((Rectangle){ x, tabsRow.y + tabsRow.height - 1, w, 1 }, COL_BORDER);
        y += 40 + 22;
    }

    // Body
    float bodyTop = y;
    float footerH = 12 + 38;
    float bodyH   = area.y + area.height - padBot - footerH - 12 - bodyTop;
    {
        switch (state->settingsTab) {
            case SETTINGS_TAB_GRAPHICS: {
                static const char *QUALITY_OPTS[] = {
                    "Potato", "Low", "Medium", "High", "Ultra",
                };
                float ry = bodyTop;
                RowStepper(0, "Quality Preset", &state->qualityPreset, 0, 4, QUALITY_OPTS,
                    (Rectangle){ x, ry, w, 56 }); ry += 56;
                RowToggle(1, "Fullscreen",    "Borderless on macOS. Restart not required.",
                    &state->fullscreen, (Rectangle){ x, ry, w, 60 }); ry += 60;
                RowToggle(2, "Vertical sync", "Cap framerate to display refresh rate.",
                    &state->vsync,      (Rectangle){ x, ry, w, 60 }); ry += 60;
                RowToggle(3, "Screen shake",  "Camera responds to nearby impacts.",
                    &state->screenShake,(Rectangle){ x, ry, w, 60 }); ry += 60;
                break;
            }
            case SETTINGS_TAB_AUDIO: {
                float ry = bodyTop;
                RowSlider(10, "Master volume", &state->masterVolume, (Rectangle){ x, ry, w, 50 }); ry += 50;
                RowSlider(11, "Music volume",  &state->musicVolume,  (Rectangle){ x, ry, w, 50 }); ry += 50;
                RowSlider(12, "Sound effects", &state->sfxVolume,    (Rectangle){ x, ry, w, 50 }); ry += 50;
                RowToggle(13, "Subtitles", "Display spoken dialogue as captions.",
                    &state->subtitles, (Rectangle){ x, ry, w, 60 }); ry += 60;
                break;
            }
            case SETTINGS_TAB_CONTROLS: {
                static const char *KEYS_LBL[] = {
                    "Move forward","Move back","Strafe left","Strafe right",
                    "Jump / dodge","Interact","Inventory","Quick map",
                };
                static const char *KEYS_BIND[] = {
                    "W","S","A","D","SPACE","E","I","M",
                };
                int n = (int)(sizeof(KEYS_LBL)/sizeof(KEYS_LBL[0]));
                float ry = bodyTop;
                float rowH = (bodyH - 4) / n;
                if (rowH > 52) rowH = 52;
                for (int k = 0; k < n; k++) {
                    Rectangle row = { x, ry, w, rowH };
                    bool hover = UI_Hovered(row);
                    UI_Panel(row, hover ? COL_PANEL_HOVER : COL_PANEL,
                        (UI_Border){ .color = COL_BORDER, .bottom = 1 }, 0);
                    UI_DrawText(KEYS_LBL[k], row.x + 16, row.y + (row.height - 13) / 2.0f, 13, 2, COL_TEXT);

                    Rectangle bind = { row.x + row.width - 16 - 120,
                                       row.y + (row.height - 32) / 2.0f, 120, 32 };
                    UI_Panel(bind, COL_BG_DEEP,
                        (UI_Border){ .color = COL_BORDER, .left=1,.right=1,.top=1,.bottom=1 }, 2);
                    UI_DrawTextCentered(KEYS_BIND[k], bind, 13, 3, COL_ACCENT);
                    ry += rowH;
                }
                break;
            }
            default: break;
        }
    }

    // Footer action row
    {
        float fy = area.y + area.height - padBot - 38;
        ButtonStyle defStyle = {
            .fill = COL_PANEL, .fillHover = COL_PANEL_HOVER,
            .border = COL_BORDER, .borderL=1, .borderR=1, .borderT=1, .borderB=1,
            .fontSize = 11, .letterSpacing = 3,
            .textColor = COL_TEXT_DIM, .textHoverColor = COL_TEXT_DIM,
        };
        ButtonStyle cancelStyle = defStyle;
        cancelStyle.fill = COL_BG;
        cancelStyle.fontSize = 12; cancelStyle.letterSpacing = 4;

        UI_Button((Rectangle){ x,         fy, 160, 38 }, "RESTORE DEFAULTS", defStyle);
        UI_Button((Rectangle){ x + w - 160 - 12 - 120, fy, 120, 38 }, "CANCEL", cancelStyle);

        Rectangle apply = { x + w - 160, fy, 160, 38 };
        bool ah = UI_Hovered(apply);
        UI_Panel(apply, ah ? COL_ACCENT : COL_ACCENT_DIM, (UI_Border){0}, 0);
        UI_DrawTextCentered("APPLY", apply, 13, 6, COL_BG_DEEP);
    }
}

// =====================================================================
// Scene 5: HUD / Pause
// =====================================================================

void Scene_HUD(AppState *state, Rectangle area) {
    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_P)) {
        state->paused = !state->paused;
    }

    DrawRectangleRec(area, (Color){ 24, 18, 18, 255 });

    // Top strip
    Rectangle top = { area.x, area.y, area.width, 60 };
    {
        float tx = top.x + 24, ty = top.y + 16;
        // World banner
        const char *world = "HOLLOW PEAKS  -  CINDERHOLD";
        int ww = UI_TextWidth(world, 12, 4);
        Rectangle banner = { tx, ty, (float)ww + 20, 28 };
        UI_Panel(banner, COL_PANEL,
            (UI_Border){ .color = COL_ACCENT, .left = 3 }, 0);
        UI_DrawText(world, banner.x + 10, banner.y + 8, 12, 4, COL_ACCENT);

        // Objective on the right
        const char *obj = "OBJECTIVE:  find the bell-keeper's notes";
        int ow = UI_TextWidth(obj, 11, 1);
        Rectangle objBox = { top.x + top.width - 24 - ow - 30, ty, (float)ow + 30, 28 };
        UI_Panel(objBox, COL_PANEL, (UI_Border){0}, 2);
        DrawCircle((int)(objBox.x + 12), (int)(objBox.y + objBox.height / 2.0f), 4, COL_GOLD);
        UI_DrawText(obj, objBox.x + 22, objBox.y + 9, 11, 1, COL_TEXT_DIM);
    }

    // Play area placeholder (visible behind everything)
    {
        Rectangle play = { area.x, top.y + top.height, area.width, area.height - top.height - 200 };
        UI_DrawTextCentered("[ play area ]", play, 14, 4, (Color){ 70, 56, 56, 220 });
    }

    // Bottom HUD strip
    {
        float bh = 200;
        Rectangle bot = { area.x + 22, area.y + area.height - bh, area.width - 44, bh - 14 };
        float bx = bot.x, by = bot.y;

        // Resources card (HP, MP, XP)
        Rectangle res = { bx, by + 8, 260, bh - 22 };
        UI_Panel(res, COL_PANEL,
            (UI_Border){ .color = COL_BORDER, .left=1,.right=1,.top=1,.bottom=1 }, 3);
        Rectangle av = { res.x + 14, res.y + 12, 34, 34 };
        UI_Panel(av, COL_BG_DEEP,
            (UI_Border){ .color = COL_ACCENT, .left=2,.right=2,.top=2,.bottom=2 }, 0);
        UI_DrawTextCentered("B", av, 18, 0, COL_ACCENT);
        UI_DrawText("BREA OF VARN",         av.x + av.width + 12, av.y + 4, 12, 2, COL_TEXT);
        UI_DrawText("LV 14  -  Rune Paladin", av.x + av.width + 12, av.y + 22, 10, 1, COL_TEXT_FAINT);

        float ry = res.y + 12 + 38 + 4;
        StatBar("HP", state->playerHp * 480.0f, 480.0f, COL_HP,
            (Rectangle){ res.x + 14, ry, res.width - 28, 28 });
        ry += 36;
        StatBar("MP", state->playerMp * 160.0f, 160.0f, COL_MP,
            (Rectangle){ res.x + 14, ry, res.width - 28, 28 });
        ry += 36;
        StatBar("XP", 0.31f * 1000.0f, 1000.0f, COL_XP,
            (Rectangle){ res.x + 14, ry, res.width - 28, 28 });

        // Hotbar
        {
            const char *slots[] = { "SWD","BOW","PTN","RUN","RUN","DAG","---","---" };
            const char *keys[]  = { "1","2","3","4","5","6","7","8" };
            int n = 8;
            float slotSize = 58;
            float gap = 6;
            float totalW = slotSize * n + gap * (n - 1);
            float sx = bx + 260 + (bot.width - 260 - 160 - totalW) / 2.0f;
            float sy = bot.y + bot.height - slotSize - 8;
            for (int s = 0; s < n; s++) {
                Rectangle b = { sx + (slotSize + gap) * s, sy, slotSize, slotSize };
                bool empty = (slots[s][0] == '-');
                UI_Panel(b, empty ? COL_BG : COL_PANEL,
                    (UI_Border){
                        .color = empty ? COL_BORDER : COL_ACCENT_DIM,
                        .left=1,.right=1,.top=1,.bottom=1,
                    }, 0);
                UI_DrawTextCentered(slots[s],
                    (Rectangle){ b.x, b.y + 8, b.width, 24 },
                    14, 2, empty ? COL_TEXT_FAINT : COL_TEXT);
                UI_DrawTextCentered(keys[s],
                    (Rectangle){ b.x, b.y + b.height - 16, b.width, 12 },
                    9, 1, COL_TEXT_FAINT);
            }
        }

        // Minimap
        {
            Rectangle map = { bot.x + bot.width - 160, bot.y + bot.height - 160, 160, 160 };
            UI_Panel(map, COL_PANEL,
                (UI_Border){ .color = COL_BORDER, .left=1,.right=1,.top=1,.bottom=1 }, 2);
            UI_DrawText("MAP", map.x + 8, map.y + 8, 9, 3, COL_ACCENT);
            UI_DrawText("N",   map.x + map.width - 14, map.y + 8, 9, 0, COL_TEXT_FAINT);
            Rectangle grid = { map.x + 8, map.y + 24, map.width - 16, map.height - 32 };
            UI_Panel(grid, COL_BG_DEEP,
                (UI_Border){ .color = COL_BORDER, .left=1,.right=1,.top=1,.bottom=1 }, 0);
            DrawCircle((int)(grid.x + grid.width / 2.0f),
                       (int)(grid.y + grid.height / 2.0f), 4, COL_ACCENT);
        }
    }

    // Pause overlay or hint
    if (state->paused) {
        UI_SetModalActive(true);
        int sw = GetScreenWidth(), sh = GetScreenHeight();
        DrawRectangleRec((Rectangle){ 0, 0, (float)sw, (float)sh }, (Color){ 8, 8, 14, 200 });
        float cardW = 380;
        float cardH = 36 + 14 + 11 + 10 + 5 * 40 + 4 * 8 + 16 + 10 + 24 + 24;
        Rectangle card = { (sw - cardW) / 2.0f, (sh - cardH) / 2.0f, cardW, cardH };
        UI_Panel(card, COL_BG,
            (UI_Border){ .color = COL_ACCENT, .top = 3 }, 2);
        float px = card.x + 32, py = card.y + 28;
        UI_DrawTextCentered("PAUSED", (Rectangle){ card.x, py, card.width, 36 }, 36, 12, COL_ACCENT);
        py += 36 + 8;
        UI_DrawTextCentered("the world holds its breath",
            (Rectangle){ card.x, py, card.width, 11 }, 11, 3, COL_TEXT_FAINT);
        py += 11 + 10;

        const char *items[] = { "RESUME", "SAVE GAME", "LOAD", "SETTINGS", "QUIT TO MENU" };
        for (int i = 0; i < 5; i++) {
            Rectangle btn = { px, py, card.width - 64, 40 };
            bool hover = CheckCollisionPointRec(GetMousePosition(), btn);
            UI_Panel(btn, hover ? COL_PANEL_HOVER : COL_PANEL,
                (UI_Border){
                    .color = hover ? COL_ACCENT : COL_BORDER,
                    .left=1,.right=1,.top=1,.bottom=1,
                }, 0);
            UI_DrawTextCentered(items[i], btn, 13, 5, hover ? COL_ACCENT : COL_TEXT);
            py += 40 + 8;
            if (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && i == 0) {
                state->paused = false;
            }
        }
        py += 4;
        UI_DrawTextCentered("[esc] or [p] to resume",
            (Rectangle){ card.x, py, card.width, 10 }, 10, 2, COL_TEXT_FAINT);
        UI_SetModalActive(false);
    } else {
        const char *hint = "[esc/p] PAUSE";
        int hw = UI_TextWidth(hint, 11, 3);
        Rectangle h = { area.x + 24, area.y + area.height - 14 - 24, (float)hw + 20, 24 };
        UI_Panel(h, COL_PANEL,
            (UI_Border){ .color = COL_BORDER, .left=1,.right=1,.top=1,.bottom=1 }, 0);
        UI_DrawText(hint, h.x + 10, h.y + 7, 11, 3, COL_TEXT_DIM);
    }
}
