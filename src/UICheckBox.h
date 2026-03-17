#pragma once
#include "UIElement.h"

class UICheckBox : public UIElement {
public:
    UICheckBox(int id, SDL_Rect bounds, const char* text);
    bool Update(float dt) override;
    bool CleanUp() override;

    bool isChecked = false;
};