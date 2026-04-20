#include "IntroScene.h"
#include "Engine.h"
#include "Input.h"
#include "SceneManager.h"
#include "Window.h"

IntroScene::IntroScene() : SceneBase() {}
IntroScene::~IntroScene() {}

bool IntroScene::Start() {
	introTimer = 30000.0f; // 3 seconds of intro
	if (introTexture == nullptr) {
		introTexture = Engine::GetInstance().textures->Load("Assets/Textures/UI/Intro/Intro.jpg");
	}
	return true;
}

bool IntroScene::Update(float dt) {
	introTimer -= dt;
	if (introTexture != nullptr) {
		int screenW = Engine::GetInstance().render->camera.w;
		int screenH = Engine::GetInstance().render->camera.h;

		SDL_Rect fullScreenRect = { 0, 0, screenW, screenH };

		//  Dibujamos la textura forzándola a ocupar ese rectángulo
		Engine::GetInstance().render->DrawTextureScaled(introTexture, fullScreenRect);
	}

	// Skip intro logic
	if (introTimer <= 0.0f || Engine::GetInstance().input->GetKey(SDL_SCANCODE_SPACE) == KEY_DOWN) {
		Engine::GetInstance().sceneManager->ChangeScene(SceneID::MENU);
	}

	return true;
}

bool IntroScene::CleanUp() {
	if (introTexture != nullptr) {
		Engine::GetInstance().textures->UnLoad(introTexture);
		introTexture = nullptr;
	}
	return true;
}