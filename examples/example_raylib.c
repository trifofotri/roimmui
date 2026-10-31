#include <raylib.h>
#define ROIMMUI_IMPLEMENTATION
#include "../roimmui.h"

static float Measure(const char* text, float size) {
    return (float)MeasureText(text, (int)size);
}

static void Rect(Vector2 pos, Vector2 size, Color3 c, float transparency) {
    DrawRectangleV(pos, size, (Color){ (unsigned char)c.r, (unsigned char)c.g, (unsigned char)c.b, (unsigned char)((1.0f - transparency) * 255) });
}

static void Text(Vector2 pos, const char* text, float size, Color3 c) {
    DrawText(text, (int)pos.x, (int)pos.y, (int)size, (Color){ (unsigned char)c.r, (unsigned char)c.g, (unsigned char)c.b, 255 });
}

int main(void) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    InitWindow(800, 400, "roimmui example");
    SetTargetFPS(60);

    UiInstance* gui = Instance_New(GuiContext);
    gui->MeasureText = Measure;
    gui->DrawRect = Rect;
    gui->DrawText = Text;

    UiInstance* text = Instance_New(TextLabel);
    text->Text = "Hello World!";
    SetParent(text, gui);

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground((Color){ 20, 20, 24, 255 });    // clear FIRST, then draw the gui

        gui->ViewportSize = (Vector2){ (float)GetScreenWidth(), (float)GetScreenHeight() };
        GuiStepContext(gui);

        EndDrawing();
    }

    Instance_Destroy(gui);
    CloseWindow();
    return 0;
}