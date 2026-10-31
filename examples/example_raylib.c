#include <raylib.h>
#include <stdio.h>
#include <math.h>
#define ROIMMUI_NO_VECTOR2
#define ROIMMUI_IMPLEMENTATION
#include "../roimmui.h"

static float Measure(const char* text, float size) {
    return (float) MeasureText(text, (int) size);
}

static void Rect(Vector2 pos, Vector2 size, Color3 c, float transparency) {
    DrawRectangleV(pos, size, (Color){ (unsigned char)c.r, (unsigned char)c.g, (unsigned char)c.b, (unsigned char)((1.0f - transparency) * 255) });
}

Font f;

static void Text(Vector2 pos, const char* text, float size, Color3 c) {
    DrawTextEx(f, text, pos, size, 0.0f,
        (Color){ (unsigned char)c.r, (unsigned char)c.g, (unsigned char)c.b, 255 }
    );
}

static int counter;
char txt[32];
static void clicked(UiInstance* self) {
    counter++;
    snprintf(txt, 32, "clicked: %d", counter);
    self->Text = txt;
}

int main(void) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    InitWindow(800, 400, "roimmui example");
    SetTargetFPS(60);

    f = LoadFontEx("examples/font.ttf", 14, NULL, 0);
    SetTextureFilter(f.texture, TEXTURE_FILTER_TRILINEAR);

    UiInstance* gui = Instance_New(GuiContext);
    gui->MeasureText = Measure;
    gui->DrawRect = Rect;
    gui->DrawText = Text;

    UiInstance* header = Instance_New(TextLabel);
    header->Size = UDim2_New(1, 0, 1, 0);
    header->Text = "roimmui! A roblox inspired immediate mode ui library.";
    header->TextColor = (Color3) {255, 255, 255};
    SetParent(header, gui);

    UiInstance* text = Instance_New(TextButton);
    text->Text = "Hello World!";
    text->MouseButton1Click = clicked;
    text->Position = UDim2_New(0, 0, 0.1, 0);
    SetParent(text, gui);

    unsigned int header_hue = 0;
    while (!WindowShouldClose()) {
        header_hue += 100.0f * GetFrameTime();
        header_hue = fmodf(header_hue, 355.0f);
        
        Color c = ColorFromHSV(header_hue, 1.0f, 0.85f);
        header->BackgroundColor = (Color3) {c.r, c.g, c.b};

        BeginDrawing();
        ClearBackground((Color){ 20, 20, 24, 255 });

        gui->ViewportSize = (Vector2){ (float)GetScreenWidth(), (float)GetScreenHeight() };
        gui->MouseDown = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
        gui->MousePosition = GetMousePosition();
        GuiStepContext(gui);

        EndDrawing();
    }

    Instance_Destroy(gui);
    CloseWindow();
    return 0;
}