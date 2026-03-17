#include "UICheckBox.h"
#include "Engine.h"
#include "Render.h"

UICheckBox::UICheckBox(int id, SDL_Rect bounds, const char* text) : UIElement(UIElementType::CHECKBOX, id) {
    this->bounds = bounds;
    this->text = text;
}

bool UICheckBox::Update(float dt) {
    if (!visible) return false; 

    if (state != UIElementState::DISABLED) {
        Vector2D mousePos = Engine::GetInstance().input->GetMousePosition();
        
        if (mousePos.getX() > bounds.x && mousePos.getX() < bounds.x + bounds.w &&
            mousePos.getY() > bounds.y && mousePos.getY() < bounds.y + bounds.h) {
            
            state = UIElementState::FOCUSED;
            if (Engine::GetInstance().input->GetMouseButtonDown(SDL_BUTTON_LEFT) == KEY_UP) {
                isChecked = !isChecked; 
                NotifyObserver();       
            }
        } else {
            state = UIElementState::NORMAL;
        }
    }

    // Dibujar caja
    Engine::GetInstance().render->DrawRectangle(bounds, 150, 150, 150, 255, false, false);
    
    // Dibujar X o cuadro relleno si está activado
    if (isChecked) {
        SDL_Rect inner = { bounds.x + 4, bounds.y + 4, bounds.w - 8, bounds.h - 8 };
        Engine::GetInstance().render->DrawRectangle(inner, 0, 255, 0, 255, true, false);
    }
    
    // Dibujar Texto al lado
    Engine::GetInstance().render->DrawText(text.c_str(), bounds.x + bounds.w + 10, bounds.y, 100, bounds.h, {255,255,255,255});

    return true;
}

bool UICheckBox::CleanUp() { pendingToDelete = true; return true; }