#include "Hud.h"
#include "Engine.h"
#include "Textures.h"
#include "Render.h"
#include "SceneManager.h"
#include "Player.h"
#include "Window.h"
#include "Log.h"
#include <string>

Hud::Hud() : Module() {
    name = "hud";
}

Hud::~Hud() {}

bool Hud::Awake() { return true; }

bool Hud::Start() {
    LOG("Loading HUD");
    //lifeBarTexture = Engine::GetInstance().textures->Load("Assets/Hud/HealthBar.png");

    return true;
}

bool Hud::Update(float dt) {

    if (Engine::GetInstance().sceneManager->GetPlayer() == nullptr) return true;

    Player* player = Engine::GetInstance().sceneManager->GetPlayer();

    return true;
}

bool Hud::PostUpdate() {
    
    DrawPlayerHealthBar();
    DrawMineralIndicator();
    DrawDiamondCounter();

    return true;
}

void Hud::DrawPlayerHealthBar() {

}

void Hud::DrawDiamondCounter() {

}

void Hud::DrawMineralIndicator() {

}


bool Hud::CleanUp() {
    // Engine::GetInstance().textures->UnLoad(lifeBarTexture);

    return true;
}