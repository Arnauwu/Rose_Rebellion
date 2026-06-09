#include "Npc.h"
#include "Engine.h"
#include "Textures.h"
#include "Physics.h"
#include "Input.h"
#include "Render.h"
#include "Window.h"
#include "DialogueManager.h"
#include "LanguageManager.h"
#include "GameManager.h"

#include "EntityManager.h"
#include "Log.h"

#include "tracy/Tracy.hpp"

#include <algorithm>

Npc::Npc() : Entity(EntityType::NPC) {
    name = "Npc";
    pbody = nullptr;
    texture = nullptr;
}

Npc::~Npc() {}

bool Npc::Awake() {
    return true;
}

void Npc::ConfigNPC(const std::string& texPath, const std::string& dialogID, int width, int height, const std::string& tsx) {
    this->texturePath = texPath;
    this->dialogueID = dialogID;
    this->texW = width;
    this->texH = height;

    this->tsxPath = tsx;
}

bool Npc::Start() {
    if (!texturePath.empty())
    {
        texture = Engine::GetInstance().textures->Load(texturePath.c_str());
    }
    
    if (!tsxPath.empty()) {
        std::unordered_map<int, std::string> aliases = { {0, "scared"},{8, "walk"},{16, "idle"} };

        anims.LoadFromTSX(tsxPath.c_str(), aliases);

        if (GameManager::GetInstance().gameState.knightBossKilled)
        {
            anims.SetCurrent("idle");
        }
        else
        {
            anims.SetCurrent("scared");
        }
    }

    
    //TO DO: CAMBIAR TEXTURA
    interactionBackgroundTexture = Engine::GetInstance().textures->Load("Assets/Textures/UI/Tutorial/SS_FondoTexto_Interaccion.png");
    std::unordered_map<int, std::string> interactionBackgroundAliases = { {0, "idle"} };
    interactionBackgroundAnimation.LoadFromTSX(
        "Assets/Textures/UI/Tutorial/SS_FondoTexto_Interaccion.tsx",
        interactionBackgroundAliases
    );
    interactionBackgroundAnimation.SetCurrent("idle");

    interactIcon = Engine::GetInstance().textures->Load("Assets/Textures/UI/Buttons/Flecha.png");
    zOrder = -1;
   

    pbody = Engine::GetInstance().physics->CreateRectangleSensor((int)position.getX() - texW /2, (int)position.getY() - texH /2, texW, texH, bodyType::STATIC);

    pbody->listener = this;
    pbody->ctype = ColliderType::NPC;

    return true;
}

bool Npc::Update(float dt) {
    if (!active) return true;
    ZoneScoped;

    // Si el jugador est?cerca y no est?activo
    if (isPlayerInRange && Engine::GetInstance().dialogueManager->CanInteract()) {
        // Si el jugador pulsa 'E' 
        if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_E) == KEY_DOWN) {
            Engine::GetInstance().dialogueManager->StartDialogue(dialogueID);
            Engine::GetInstance().input->ClearMouseInput(); // Limpiar inputs residuales

        }
    }

    int x, y;
    pbody->GetPosition(x, y);
    position.setX((float)x);
    position.setY((float)y);

    // Dibujamos el NPC 
    if (texture != nullptr) {
        if (!tsxPath.empty()) {
            anims.Update(dt);

            const SDL_Rect& animFrame = anims.GetCurrentFrame();

            Engine::GetInstance().render->DrawRotatedTexture(texture, x, y, &animFrame, SDL_FLIP_NONE, 1.0f);
        }
        else {
            Engine::GetInstance().render->DrawTexture(texture, x - (texW), y - (texH / 2), nullptr, 1.0f, 0.0, INT_MAX, INT_MAX);
        }
    }

    if (isPlayerInRange && Engine::GetInstance().dialogueManager->CanInteract()) {
        if (interactIcon != nullptr) {
            iconTimer += dt / 1000.0f;

            float offsetY = sin(iconTimer * 5.0f) * 10.0f;

            float fIconW = 0.0f;
            float fIconH = 0.0f;

            SDL_GetTextureSize(interactIcon, &fIconW, &fIconH);

            int iconW = (int)fIconW;
            int iconH = (int)fIconH;

            int drawX = x - (iconW / 2);
            int drawY = y - (texH / 2) - iconH - 20 + (int)offsetY;

            SDL_Rect cam = Engine::GetInstance().render->camera;
            int scale = Engine::GetInstance().window->GetScale();
            float zoomLevel = Engine::GetInstance().render->GetZoom();

            int screenX = (int)((cam.x + (drawX - iconW) * scale) * zoomLevel);
            int screenY = (int)((cam.y + drawY * scale) * zoomLevel) - 35;
            int screenW = (int)((iconW * 3 * scale) * zoomLevel);
            int screenH = (int)((iconH * scale) * zoomLevel);
            // EFECTO Shadow
       /*     SDL_Rect shadowBounds = { screenX + 2, screenY + 2, screenW, screenH };
            SDL_Color shadowColor = { 0, 0, 0, 255 };
            Engine::GetInstance().render->DrawTextCentered("PARLAR {E}", shadowBounds, shadowColor, FontType::CUERPO);*/
            
            std::string interactText = Engine::GetInstance().languageManager->GetString("INTERACT_NPC");
            SDL_Rect measuredText = Engine::GetInstance().render->MeasureText(
                interactText.c_str(), FontType::CUERPO);

            int promptW = std::max(screenW, measuredText.w + 40);
            int promptH = std::max(screenH + 20, measuredText.h + 36);
            int promptCenterX = screenX + screenW / 2;
            int promptCenterY = screenY + screenH / 2;
            SDL_Rect promptBounds = {
                promptCenterX - promptW / 2,
                promptCenterY - promptH / 2,
                promptW,
                promptH
            };

            interactionBackgroundAnimation.Update(dt);
            if (interactionBackgroundTexture != nullptr) {
                const SDL_Rect& backgroundFrame = interactionBackgroundAnimation.GetCurrentFrame();
                Engine::GetInstance().render->DrawTextureScaledSection(
                    interactionBackgroundTexture, backgroundFrame, promptBounds);
            }

            Engine::GetInstance().render->DrawTexture(
                interactIcon, drawX, drawY, nullptr, 1.0f, 0.0, INT_MAX, INT_MAX);

            SDL_Color textColor = { 45, 24, 16, 255 };
            Engine::GetInstance().render->DrawTextCentered(
                interactText.c_str(), promptBounds, textColor, FontType::CUERPO);
        }
    }

    return true;
}

bool Npc::CleanUp() {
    if (texture) Engine::GetInstance().textures->UnLoad(texture);
    if (interactionBackgroundTexture) Engine::GetInstance().textures->UnLoad(interactionBackgroundTexture);
    if (interactIcon) Engine::GetInstance().textures->UnLoad(interactIcon);
    if (pbody) {
        Engine::GetInstance().physics->DeletePhysBody(pbody);
        pbody = nullptr;
    }
    return true;
}

void Npc::OnCollision(PhysBody* physA, PhysBody* physB, b2ShapeId shapeA, b2ShapeId shapeB) {
    if (physB->ctype == ColliderType::PLAYER) {
        isPlayerInRange = true;
        LOG("Jugador en rango del NPC");
    }
}

void Npc::OnCollisionEnd(PhysBody* physA, PhysBody* physB, b2ShapeId shapeA, b2ShapeId shapeB) {
    if (physB->ctype == ColliderType::PLAYER) {
        isPlayerInRange = false;
        LOG("Jugador salio del rango del NPC");
    }
}
