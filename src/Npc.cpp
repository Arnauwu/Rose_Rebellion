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
    interactIcon = Engine::GetInstance().textures->Load("Assets/Textures/UI/Buttons/Flecha.png");
    zOrder = -1;
    glowTex = Engine::GetInstance().textures->Load("Assets/Textures/UI/Glow.png");
   

    pbody = Engine::GetInstance().physics->CreateRectangleSensor((int)position.getX() - texW /2, (int)position.getY() - texH /2, texW, texH, bodyType::STATIC);

    pbody->listener = this;
    pbody->ctype = ColliderType::NPC;

    return true;
}

bool Npc::Update(float dt) {
    if (!active) return true;
    ZoneScoped;

    // Si el jugador está cerca y no está activo
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

            Engine::GetInstance().render->DrawTexture(interactIcon, drawX, drawY, nullptr, 1.0f, 0.0, INT_MAX, INT_MAX);
            
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
            
            //Text
            SDL_Rect textBoundsScreen = { screenX, screenY, screenW, screenH };
            SDL_Color textColor = { 255, 255,255, 255 };
            std::string interactText = Engine::GetInstance().languageManager->GetString("INTERACT_NPC");
            Engine::GetInstance().render->DrawTextCentered(interactText.c_str(), textBoundsScreen, textColor, FontType::CUERPO);

            //Efecto glow
            if (glowTex != nullptr) {
                SDL_SetTextureBlendMode(glowTex, SDL_BLENDMODE_ADD);

                SDL_SetTextureColorMod(glowTex, 150, 150, 150);

                // Palpito del brillo
                Uint8 glowAlpha = (Uint8)(150 + sin(iconTimer * 5.0f) * 50);
                SDL_SetTextureAlphaMod(glowTex, glowAlpha);

                int glowW = iconW * 3;
                int glowH = iconH * 2.3;
                int glowX = screenX - (glowW / 2) + (screenW / 2);
                int glowY = screenY - (glowH / 2) + (screenH / 2) + 5;

                SDL_FRect dstGlow = { (float)glowX, (float)glowY, (float)glowW, (float)glowH };

                SDL_RenderTexture(Engine::GetInstance().render->renderer, glowTex, nullptr, &dstGlow);
            }
        }
    }

    return true;
}

bool Npc::CleanUp() {
    if (texture) Engine::GetInstance().textures->UnLoad(texture);
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