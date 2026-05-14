#include "UIItemInfoBox.h"
#include "Engine.h"
#include "Render.h"

UIItemInfoBox::UIItemInfoBox(int id, float anchorX, float anchorY, float wPerc, float hPerc, const char* text)
    : UIElement(UIElementType::ITEM_INFO_BOX, id, anchorX, anchorY, wPerc, hPerc, text)
{
    visible = false;
}

UIItemInfoBox::~UIItemInfoBox() {}

bool UIItemInfoBox::Update(float dt)
{
    if (!visible) return true;
    return true;
}

void UIItemInfoBox::Draw() const
{
    if (!visible) return;

    if (bgTexture != nullptr) {
        Engine::GetInstance().render->DrawTexture9Slice(bgTexture, bounds, 64, 64, 64, 64);
    }

    if (!text.empty()) {
        int textX = bounds.x + padding;
        int textY = bounds.y + padding;
        int maxW = bounds.w - (padding * 2);
        int maxH = bounds.h - (padding * 2);

        Engine::GetInstance().render->DrawText(
            text.c_str(),
            textX,
            textY,
            maxW,
            maxH,
            textColor,
            fontType
        );
    }
}

bool UIItemInfoBox::CleanUp()
{
    pendingToDelete = true;
    return true;
}