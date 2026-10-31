#ifndef ROIMMUI_H
#define ROIMMUI_H

#include <stdlib.h>
#include <stdbool.h>

typedef struct UDim2 {
    float xs, xo, ys, yo; // x scale x offset y scale y offset
} UDim2;

#ifndef ROIMMUI_NO_VECTOR2 // for raylib 
typedef struct Vector2 {
    float x, y;
} Vector2;
#endif


typedef struct Color3 {
    float r, g, b;
} Color3;

typedef enum {
    GuiContext, Frame, TextLabel, TextButton
} RoImmuiClass;

typedef enum {
    TextAlignmentXCenter, TextAlignmentXLeft, TextAlignmentXRight
} EnumTextXAlignment;

typedef enum {
    TextAlignmentYCenter, TextAlignmentYTop, TextAlignmentYBottom
} EnumTextYAlignment;

typedef struct UiInstance UiInstance;

struct UiInstance {
    RoImmuiClass Class; // class
    void* UserData;

    // Hierarchy stuff
    UiInstance* Parent;
    UiInstance* FirstChild;
    UiInstance* LastChild;
    UiInstance* Next;
    UiInstance* Previous;

    // GuiContext
    Vector2 ViewportSize;
    Vector2 MousePosition;
    bool MouseDown;
    float (*MeasureText)(const char* text, float size);
    void (*DrawRect)(Vector2 pos, Vector2 size, Color3 color, float transparency);
    void (*DrawText)(Vector2 pos, const char* text, float size, Color3 color);
    bool PrevMouseDown;       // internal
    UiInstance* ActiveButton; // internal: the button the mouse went down on
    UiInstance* HoveredButton; // internal: the button under the mouse

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
    bool TextScaled;
    EnumTextXAlignment TextXAlignment;
    EnumTextYAlignment TextYAlignment;
    void (*MouseButton1Click)(UiInstance* self);
    void (*MouseEnter)(UiInstance* self);
    void (*MouseLeave)(UiInstance* self);
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
    if (cls == TextLabel || cls == TextButton) {
        i->Size = (UDim2) { 0, 200, 0, 50 };
    } else {
        i->Size = (UDim2) { 0, 100, 0, 100 };  
    }
    i->BackgroundColor = (Color3) { 255, 255, 255 };
    i->TextColor = (Color3) { 27, 42, 53 };
    i->TextSize = 14;

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
    UiInstance* root = i;
    while (root->Parent) {
        root = root->Parent;
    }
    if (root->HoveredButton == i) {
        root->HoveredButton = NULL;
    }
    if (root->ActiveButton == i) {
        root->ActiveButton = NULL;
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

static bool Contains(UiInstance* n, Vector2 p) {
    return p.x >= n->AbsolutePosition.x && p.x < n->AbsolutePosition.x + n->AbsoluteSize.x && p.y >= n->AbsolutePosition.y && p.y < n->AbsolutePosition.y + n->AbsoluteSize.y;
}

static void GuiHitTest(UiInstance* gui, UiInstance* n, UiInstance** hit) {
    if (!n->Visible) return; // hides the whole subtree
    if (n->Class == TextButton && Contains(n, gui->MousePosition)) {
        *hit = n;
    }

    for (UiInstance* c = n->FirstChild; c; c = c->Next) {
        GuiHitTest(gui, c, hit);
    }
}

static Color3 GuiBackground(UiInstance* gui, UiInstance* n) {
    Color3 c = n->BackgroundColor;
    if (n->Class != TextButton || n != gui->HoveredButton) {
        return c;
    }
    float f = (n == gui->ActiveButton && gui->MouseDown) ? 0.75f : 0.9f; // pressed : hovered
    return (Color3) { c.r * f, c.g * f, c.b * f };
}

static float GuiResolveTextSize(UiInstance* gui, UiInstance* n) {
    if (!n->TextScaled || !n->Text) return n->TextSize;

    int lo = 1;
    int hi = (int) n->AbsoluteSize.y;
    if (hi > 100) {
        hi = 100;
    }
    if (hi < lo) return (float) lo;

    while (lo < hi) {
        int mid = (lo + hi + 1) / 2;
        if (gui->MeasureText(n->Text, (float) mid) <= n->AbsoluteSize.x) {
            lo = mid;
        } else {
            hi = mid - 1;
        }
    }
    return (float) lo;
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
        case TextButton:
            if (!n->Visible) return; // hides the whole subtree
            gui->DrawRect(n->AbsolutePosition, n->AbsoluteSize, GuiBackground(gui, n), n->BackgroundTransparency);
            break;
    }

    if ((n->Class == TextLabel || n->Class == TextButton) && n->Text) {
        float ts = GuiResolveTextSize(gui, n);
        float w = gui->MeasureText(n->Text, ts);

        float x;
        switch (n->TextXAlignment) {
            case TextAlignmentXLeft: {
                x = n->AbsolutePosition.x;
                break;
            }
            case TextAlignmentXRight: {
                x = n->AbsolutePosition.x + n->AbsoluteSize.x - w;
                break;
            }
            case TextAlignmentXCenter:
            default:
                x = n->AbsolutePosition.x + (n->AbsoluteSize.x - w) * 0.5f;
                break;
        }
        
        float y;
        switch (n->TextYAlignment) {
            case TextAlignmentYTop: {
                y = n->AbsolutePosition.y;
                break;
            }
            case TextAlignmentYBottom: {
                y = n->AbsolutePosition.y + n->AbsoluteSize.y - ts;
                break;
            }
            case TextAlignmentYCenter:
            default:
                y = n->AbsolutePosition.y + (n->AbsoluteSize.y - ts) * 0.5f;
                break;
        }
        Vector2 pos = { x, y };
        gui->DrawText(pos, n->Text, ts, n->TextColor);
    }

    for (UiInstance* c = n->FirstChild; c; c = c->Next) {
        GuiRender(gui, c);
    }
}

void GuiStepContext(UiInstance* gui) {
    gui->AbsolutePosition = (Vector2){ 0, 0 };
    gui->AbsoluteSize = gui->ViewportSize;
    GuiLayout(gui);
    
    // input: who is the mouse over?
    UiInstance* hover = NULL;
    GuiHitTest(gui, gui, &hover);

    UiInstance* previous = gui->HoveredButton;
    gui->HoveredButton = hover;
    if (previous != hover) {
        if (previous && previous->MouseLeave) {
            previous->MouseLeave(previous);
        }
        hover = gui->HoveredButton;
        if (hover && hover->MouseEnter) {
            hover->MouseEnter(hover);
        }
    }

    bool pressed = gui->MouseDown && !gui->PrevMouseDown;
    bool released = !gui->MouseDown && gui->PrevMouseDown;
    gui->PrevMouseDown = gui->MouseDown;

    if (pressed) {
        gui->ActiveButton = gui->HoveredButton;
    }

    if (released) {
        UiInstance* a = gui->ActiveButton;
        gui->ActiveButton = NULL;

        if (a && a == gui->HoveredButton && a->MouseButton1Click) {
            a->MouseButton1Click(a);
        }
    }
    GuiRender(gui, gui);
}


#endif // ROMMUI_IMPLEMENTATION

#endif // ROIMMUI_H