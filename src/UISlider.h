#pragma once
#include "UIElement.h"

class UISlider : public UIElement {
public:
    UISlider(int id, SDL_Rect bounds, const char* text);
    bool Update(float dt) override;
    bool CleanUp() override;

    void SetValue(float value); // 0.0f a 1.0f
    float GetValue() const;

private:
    SDL_Rect sliderBar;
    SDL_Rect thumb;
    float value = 0.5f;
};