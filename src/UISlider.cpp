#include "UISlider.h"
#include "Engine.h"
#include "Render.h"

UISlider::UISlider(int id, float anchorX, float anchorY, float wPerc, float hPerc, const char* text)
    : UIElement(UIElementType::SLIDER, id, anchorX, anchorY, wPerc, hPerc, text)
{
    SetValue(0.5f);
}

void UISlider::UpdateBarAndThumb() {
    sliderBar = { bounds.x, bounds.y + (bounds.h / 2) - 2, bounds.w, 4 };
    thumb = { bounds.x, bounds.y, 20, bounds.h };
    thumb.x = sliderBar.x + (int)(value * (float)(sliderBar.w - thumb.w));
}

void UISlider::SetValue(float val) {
    if (val < 0.0f) val = 0.0f; else if (val > 1.0f) val = 1.0f;
    value = val;
    thumb.x = sliderBar.x + (int)(value * (float)(sliderBar.w - thumb.w));
}

float UISlider::GetValue() const {
    return value;
}

bool UISlider::Update(float dt) {
    if (!visible) return false;

    if (state != UIElementState::DISABLED) {
        Vector2D mousePos = Engine::GetInstance().input->GetMousePosition();

        bool mouseInside = (mousePos.getX() > bounds.x && mousePos.getX() < bounds.x + bounds.w &&
            mousePos.getY() > bounds.y && mousePos.getY() < bounds.y + bounds.h);

        if (Engine::GetInstance().input->GetMouseButtonDown(SDL_BUTTON_LEFT) == KEY_REPEAT) {
            if (mouseInside || state == UIElementState::FOCUSED) {
                state = UIElementState::FOCUSED;
                float relativeX = (float)(mousePos.getX() - sliderBar.x - (thumb.w / 2));
                float normalized = relativeX / (float)(sliderBar.w - thumb.w);
                SetValue(normalized);
                NotifyObserver();
            }
        }
        else {
            state = UIElementState::NORMAL;
        }
    }

    return true;
}

void UISlider::Draw() const {
    if (!visible) return;

    Engine::GetInstance().render->DrawRectangle(sliderBar, 200, 200, 200, 255, true, false);
    Engine::GetInstance().render->DrawRectangle(thumb, 255, 0, 0, 255, true, false);
    Engine::GetInstance().render->DrawText(text.c_str(), bounds.x, bounds.y - 20, 0, 0, { 255,255,255,255 });
}

bool UISlider::CleanUp() {
    pendingToDelete = true;
    return pendingToDelete;
}