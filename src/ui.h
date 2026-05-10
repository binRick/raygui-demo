#ifndef RAYGUI_DEMO_UI_H
#define RAYGUI_DEMO_UI_H

#include "raylib.h"
#include <stdbool.h>

typedef enum {
    SCENE_MAIN_MENU = 0,
    SCENE_CHARACTER,
    SCENE_INVENTORY,
    SCENE_SETTINGS,
    SCENE_HUD,
    SCENE_COUNT
} SceneId;

typedef enum {
    SETTINGS_TAB_GRAPHICS = 0,
    SETTINGS_TAB_AUDIO,
    SETTINGS_TAB_CONTROLS,
    SETTINGS_TAB_COUNT
} SettingsTab;

typedef struct {
    SceneId scene;
    bool paused;

    int selectedCharacter;
    int selectedInventoryItem;
    int hoveredInventoryItem;
    int inventoryFilter;
    float inventoryScroll;
    SettingsTab settingsTab;

    bool newHeroFormOpen;
    char newHeroNameBuf[24];
    int  newHeroNameLen;
    int  newHeroClass;
    int  newHeroStats[4];
    int  newHeroAccent;

    float musicVolume;
    float sfxVolume;
    float masterVolume;
    bool vsync;
    bool fullscreen;
    bool subtitles;
    bool screenShake;
    int qualityPreset;

    float time;
    float playerHp;
    float playerMp;
} AppState;

extern const Color COL_BG_DEEP;
extern const Color COL_BG;
extern const Color COL_PANEL;
extern const Color COL_PANEL_HOVER;
extern const Color COL_BORDER;
extern const Color COL_ACCENT;
extern const Color COL_ACCENT_DIM;
extern const Color COL_DANGER;
extern const Color COL_GOLD;
extern const Color COL_TEXT;
extern const Color COL_TEXT_DIM;
extern const Color COL_TEXT_FAINT;
extern const Color COL_HP;
extern const Color COL_MP;
extern const Color COL_XP;

void UI_BeginFrame(void);
const char *UI_Fmt(const char *fmt, ...);

// Geometry helpers — clay-ui's "padding/childGap" semantics, manually.
typedef struct { float x, y, w, h; } RectF;

// Text utilities respecting raylib's default font.
int  UI_TextWidth(const char *text, int fontSize, int letterSpacing);
void UI_DrawText(const char *text, float x, float y, int fontSize, int letterSpacing, Color color);
void UI_DrawTextCentered(const char *text, Rectangle box, int fontSize, int letterSpacing, Color color);

// Panel primitives: filled rect, optional border (per-side widths), optional rounded corner radius.
typedef struct {
    Color color;
    float left, right, top, bottom;
} UI_Border;
void UI_Panel(Rectangle r, Color fill, UI_Border border, float cornerRadius);
void UI_AccentBar(Rectangle r, Color color);

// Hover helper — true if the mouse is inside r and a modal isn't blocking.
bool UI_Hovered(Rectangle r);

// Wrapped text drawing inside a max-width box; returns the y past the last line.
float UI_DrawTextWrapped(const char *text, float x, float y, float maxWidth,
                         int fontSize, int lineHeight, int letterSpacing, Color color);

// Reusable widgets matching clay-ui's look.
void Sidebar(AppState *state, Rectangle area);
void Footer(Rectangle area);
void StatBar(const char *label, float value, float maxValue, Color fillColor, Rectangle area);

// Scenes.
void Scene_MainMenu (AppState *state, Rectangle area);
void Scene_Character(AppState *state, Rectangle area);
void Scene_Inventory(AppState *state, Rectangle area);
void Scene_Settings (AppState *state, Rectangle area);
void Scene_HUD      (AppState *state, Rectangle area);

#endif
