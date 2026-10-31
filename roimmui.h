#ifndef ROIMMUI_H
#define ROIMMUI_H

#include <stdlib.h>
#include <stdbool.h>

typedef struct UDim2 {
    float xs, xo, ys, yo; // x scale x offset y scale y offset
} UDim2;

typedef struct Vector2 {
    float x, y;
} Vector2;

typedef struct Color3 {
    float r, g, b;
} Color3;

typedef enum {
    GuiContext, Frame, TextLabel
} RoImmuiClass;

typedef struct UiInstance UiInstance;

struct UiInstance {
    RoImmuiClass Class; // class

    // Hierarchy stuff
    UiInstance* Parent;
    UiInstance* FirstChild;
    UiInstance* LastChild;
    UiInstance* Next;
    UiInstance* Previous;

    // GuiContext
    Vector2 ViewportSize;
    float (*MeasureText)(const char* text, float size);
    void (*DrawRect)(Vector2 pos, Vector2 size, Color3 color, float transparency);
    void (*DrawText)(Vector2 pos, const char* text, float size, Color3 color);
    
    // Computed by GuiStepContext()
    Vector2 AbsolutePosition;
    Vector2 AbsoluteSize;

    // Common properties
    UDim2 Position;
    UDim2 Size;
    Vector2 AnchorPoint;
    Color3 BackgroundColor;
    float BackgroundTransparency;
    bool Visible;

    // TextLabel
    const char* Text;
    Color3 TextColor;
    float TextSize;
};

UiInstance* Instance_New(RoImmuiClass cls);
void Instance_Destroy(UiInstance* i);
void SetParent(UiInstance* i, UiInstance* p);
void GuiStepContext(UiInstance* gui);

static inline UDim2 UDim2_New(float xs, float xo, float ys, float yo) {
    return (UDim2) { xs, xo, ys, yo };
}

#define ROIMMUI_IMPLEMENTATION
#ifdef ROIMMUI_IMPLEMENTATION

UiInstance* Instance_New(RoImmuiClass cls) {
    UiInstance* i = (UiInstance *) calloc(1, sizeof(UiInstance));
    if (!i) return NULL;

    // defaults
    i->Class = cls;
    i->Visible = true;
    
    i->Size = (UDim2) { 0, 100, 0, 100 };
    i->BackgroundColor = (Color3) { 40, 40, 48 };
    i->TextColor = (Color3){ 255, 255, 255 };
    i->TextSize = 18;

    return i;
}

void SetParent(UiInstance* i, UiInstance* p) {
    for (UiInstance* a = p; a; a = a->Parent) {
        if (a == i) return; /* no cycles */
    }

    if (i->Parent) {
        if (i->Previous) {
            i->Previous->Next = i->Next;
        } else {
            i->Parent->FirstChild = i->Next;
        }

        if (i->Next) {
            i->Next->Previous = i->Previous;
        } else {
            i->Parent->LastChild = i->Previous;
        }
    }

    i->Parent = p;
    i->Previous = i->Next = NULL;

    if (p) {
        i->Previous = p->LastChild;
        if (p->LastChild) {
            p->LastChild->Next = i;
        } else {
            p->FirstChild = i;
        }
        p->LastChild = i;
    }
}

void Instance_Destroy(UiInstance* i) {
    while (i->FirstChild) {
        Instance_Destroy(i->FirstChild); // each child unlinks itself from i
    }
    SetParent(i, NULL);
    free(i);
}

static Vector2 Resolve(UDim2 u, Vector2 base) {
    return (Vector2) { u.xs * base.x + u.xo, u.ys * base.y + u.yo };
}

static void GuiLayout(UiInstance* parent) {
    for (UiInstance* c = parent->FirstChild; c; c = c->Next) {
        Vector2 size = Resolve(c->Size, parent->AbsoluteSize);
        Vector2 off = Resolve(c->Position, parent->AbsoluteSize);
        c->AbsoluteSize = size;
        c->AbsolutePosition = (Vector2){ parent->AbsolutePosition.x + off.x - c->AnchorPoint.x * size.x, parent->AbsolutePosition.y + off.y - c->AnchorPoint.y * size.y };
        GuiLayout(c);
    }
}

static void GuiRender(UiInstance* gui, UiInstance* n) {
    switch (n->Class) {
        case GuiContext:
            break;
        case Frame:
        case TextLabel:
            if (!n->Visible) return; // hides the whole subtree
            gui->DrawRect(n->AbsolutePosition, n->AbsoluteSize, n->BackgroundColor, n->BackgroundTransparency);
            break;
    }

    if (n->Class == TextLabel && n->Text) {
        float w = gui->MeasureText(n->Text, n->TextSize);
        Vector2 pos = { n->AbsolutePosition.x + (n->AbsoluteSize.x - w) * 0.5f, n->AbsolutePosition.y + (n->AbsoluteSize.y - n->TextSize) * 0.5f };
        gui->DrawText(pos, n->Text, n->TextSize, n->TextColor);
    }

    for (UiInstance* c = n->FirstChild; c; c = c->Next) {
        GuiRender(gui, c);
    }
}

void GuiStepContext(UiInstance* gui) {
    gui->AbsolutePosition = (Vector2){ 0, 0 };
    gui->AbsoluteSize = gui->ViewportSize;
    GuiLayout(gui);
    GuiRender(gui, gui);
}


#endif // ROMMUI_IMPLEMENTATION

#endif // ROIMMUI_H