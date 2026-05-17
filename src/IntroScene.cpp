#include "IntroScene.h"
#include "Engine.h"
#include "Input.h"
#include "Cinematics.h"
#include "SceneManager.h"
#include "Window.h"
#include "Log.h"
#include "Audio.h"

IntroScene::IntroScene() : SceneBase() {}
IntroScene::~IntroScene() {}

bool IntroScene::Awake() {

	return true;
}
bool IntroScene::Start() {
	auto textures = Engine::GetInstance().textures;
	textures->Load("Assets/Textures/UI/MainMenu/MainMenu.png");
	textures->Load("Assets/Textures/UI/MainMenu/MainMenu_S.png");
	textures->Load("Assets/Textures/UI/Buttons/frameTex.png");
	textures->Load("Assets/Textures/UI/Buttons/pomo.png");

	Engine::GetInstance().audio->PlayMusic(nullptr);

	if (!Engine::GetInstance().cinematics->PlayVideo("Assets/Cinematics/GameIntro.mp4")) {
		LOG("Failed to play intro cinematic. Skipping to Game.");
		Engine::GetInstance().sceneManager->ChangeScene(SceneID::MENU);
	}
	return true;
}

bool IntroScene::Update(float dt) {
	auto input = Engine::GetInstance().input;

	if (!Engine::GetInstance().cinematics->IsPlaying()) {
		Engine::GetInstance().sceneManager->ChangeScene(SceneID::MENU);
	}
	return true;
}

bool IntroScene::CleanUp() {
	LOG("Freeing Intro Cinematic Scene");

	auto textures = Engine::GetInstance().textures;
	textures->UnLoad(textures->Load("Assets/Textures/UI/MainMenu/MainMenu.png"));
	textures->UnLoad(textures->Load("Assets/Textures/UI/MainMenu/MainMenu_S.png"));
	textures->UnLoad(textures->Load("Assets/Textures/UI/Buttons/frameTex.png"));
	textures->UnLoad(textures->Load("Assets/Textures/UI/Buttons/pomo.png"));

	return true;
}