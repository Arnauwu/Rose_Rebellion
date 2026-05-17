#include "UIElement.h"

class UIItemInfoBox : public UIElement
{
public:
    UIItemInfoBox(int id, float anchorX, float anchorY, float wPerc, float hPerc, const char* text);
    virtual ~UIItemInfoBox();

    bool Update(float dt) override;
    void Draw() const override;
    bool CleanUp() override;

    void SetTextColor(SDL_Color color) { textColor = color; }
    void SetFontType(FontType font) { fontType = font; }
    void SetPadding(int p) { padding = p; }

private:
    SDL_Color textColor = { 255, 255, 255, 255 };
    FontType fontType = FontType::DIALOGUE;
    int padding = 20;
};