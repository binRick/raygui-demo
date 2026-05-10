#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include "raylib.h"
#include "ui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static SceneId NavScene(AppState *state) {
    if (state->newHeroFormOpen) return state->scene;
    if (IsKeyPressed(KEY_ONE))   return SCENE_MAIN_MENU;
    if (IsKeyPressed(KEY_TWO))   return SCENE_CHARACTER;
    if (IsKeyPressed(KEY_THREE)) return SCENE_INVENTORY;
    if (IsKeyPressed(KEY_FOUR))  return SCENE_SETTINGS;
    if (IsKeyPressed(KEY_FIVE))  return SCENE_HUD;
    return state->scene;
}

static void Layout(AppState *state) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    Rectangle sidebar = { 0, 0, 220, (float)sh };
    Rectangle footer  = { sidebar.width, sh - 32, sw - sidebar.width, 32 };
    Rectangle scene   = { sidebar.width, 0,       sw - sidebar.width, sh - 32 };

    switch (state->scene) {
        case SCENE_MAIN_MENU: Scene_MainMenu (state, scene); break;
        case SCENE_CHARACTER: Scene_Character(state, scene); break;
        case SCENE_INVENTORY: Scene_Inventory(state, scene); break;
        case SCENE_SETTINGS:  Scene_Settings (state, scene); break;
        case SCENE_HUD:       Scene_HUD      (state, scene); break;
        default: break;
    }

    // Sidebar and footer overlay the scene chrome on the left / bottom.
    Sidebar(state, sidebar);
    Footer(footer);
}

int main(void) {
    // Environment-driven non-interactive screenshot mode (mirrors clay-ui).
    //   RAYGUI_SCREENSHOT=path -> save a PNG after a few frames and exit
    //   RAYGUI_SCENE=0..4      -> start in scene
    //   RAYGUI_FORM=1          -> open the new-hero form on start
    const char *screenshotPath = getenv("RAYGUI_SCREENSHOT");
    const char *sceneEnv       = getenv("RAYGUI_SCENE");
    const char *formEnv        = getenv("RAYGUI_FORM");
    int  initialScene = (sceneEnv) ? atoi(sceneEnv) : SCENE_MAIN_MENU;
    bool initialForm  = (formEnv && formEnv[0] == '1');
    if (initialScene < 0 || initialScene >= SCENE_COUNT) initialScene = SCENE_MAIN_MENU;

    AppState state = {
        .scene = (SceneId)initialScene,
        .selectedCharacter = 0,
        .selectedInventoryItem = 0,
        .hoveredInventoryItem = -1,
        .inventoryFilter = 0,
        .inventoryScroll = 0.0f,
        .settingsTab = SETTINGS_TAB_GRAPHICS,
        .musicVolume = 0.6f,
        .sfxVolume = 0.8f,
        .masterVolume = 0.9f,
        .vsync = true,
        .fullscreen = false,
        .subtitles = true,
        .screenShake = true,
        .qualityPreset = 2,
        .playerHp = 0.72f,
        .playerMp = 0.45f,
        .newHeroFormOpen = false,
        .newHeroClass = 0,
        .newHeroAccent = 0,
        .newHeroStats = { 10, 10, 10, 10 },
    };
    if (initialForm) {
        state.newHeroFormOpen = true;
        const char *demoName = "Aren of the Long Road";
        size_t len = strlen(demoName);
        if (len > sizeof(state.newHeroNameBuf) - 1) len = sizeof(state.newHeroNameBuf) - 1;
        memcpy(state.newHeroNameBuf, demoName, len);
        state.newHeroNameBuf[len] = '\0';
        state.newHeroNameLen = (int)len;
        state.newHeroClass   = 1;
        state.newHeroAccent  = 2;
        state.newHeroStats[0] = 13;
        state.newHeroStats[1] = 11;
        state.newHeroStats[2] = 14;
        state.newHeroStats[3] = 12;
    }

    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT | FLAG_WINDOW_HIGHDPI);
    InitWindow(1280, 800, "raygui Game Menus");
    SetWindowMinSize(1024, 680);
    SetTargetFPS(60);
    SetExitKey(0);  // Don't quit on ESC — used for modal/pause toggle.

    int frame = 0;
    bool shotTaken = false;
    while (!WindowShouldClose()) {
        frame++;
        UI_BeginFrame();
        state.time += GetFrameTime();
        state.scene = NavScene(&state);

        BeginDrawing();
            ClearBackground((Color){ 12, 12, 18, 255 });
            Layout(&state);
        EndDrawing();

        if (screenshotPath && !shotTaken && frame == 60) {
            TakeScreenshot(screenshotPath);
            shotTaken = true;
        }
        if (screenshotPath && shotTaken && frame == 62) {
            break;
        }
    }

    CloseWindow();
    return 0;
}
