#include "MenuScene.h"
#include "Engine.h"
#include "UIManager.h"
#include "SceneManager.h"
#include "Window.h"
#include "Log.h"

MenuScene::MenuScene() : SceneBase() {}
MenuScene::~MenuScene() {}

bool MenuScene::Start() {

	Module* sceneObserver = (Module*)Engine::GetInstance().sceneManager.get();

	if (menuBackground == nullptr) {
		menuBackground = Engine::GetInstance().textures->Load("Assets/Textures/UI/MainMenu/MainMenu.png");
	}
	RecalculateBackgroundScale();

	float wPerc = 0.25f; // 25% of the screen width
	float hPerc = 0.08f; // 8% of the top of the screen.
	float spacing = 0.1f;
	float currentY = 0.35f;

	// -- MAIN MENU --
	auto btnPlay = Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, 1, "Start Game", 0.5f, currentY, wPerc, hPerc, sceneObserver);
	mainButtons.push_back(btnPlay);
	currentY += spacing;

	auto btnContinue = Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, 2, "Load Game", 0.5f, currentY, wPerc, hPerc, sceneObserver);
	mainButtons.push_back(btnContinue);
	currentY += spacing;

	auto btnSettings = Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, 3, "Settings", 0.5f, currentY, wPerc, hPerc, sceneObserver);
	mainButtons.push_back(btnSettings);
	currentY += spacing;

	auto btnExit = Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, 5, "Quit Game", 0.5f, currentY, wPerc, hPerc, sceneObserver);
	mainButtons.push_back(btnExit);

	// MAIN MENU SETTINGS
	float setY = 0.35f;

	auto sldMusic = Engine::GetInstance().uiManager->CreateUIElement(UIElementType::SLIDER, 6, "Music Volume", 0.5f, setY, 0.3f, 0.05f, sceneObserver);
	if (auto* s = dynamic_cast<UISlider*>(sldMusic.get()))
		s->SetValue(Engine::GetInstance().audio->GetMusicVolume());
	settingsButtons.push_back(sldMusic);
	setY += spacing;

	auto sldFX = Engine::GetInstance().uiManager->CreateUIElement(UIElementType::SLIDER, 7, "FX Volume", 0.5f, setY, 0.3f, 0.05f, sceneObserver);
	if (auto* s = dynamic_cast<UISlider*>(sldFX.get()))
		s->SetValue(Engine::GetInstance().audio->GetSFXVolume());
	settingsButtons.push_back(sldFX);
	setY += spacing;

	auto chkFull = Engine::GetInstance().uiManager->CreateUIElement(UIElementType::CHECKBOX, 8, "Fullscreen", 0.5f, setY, 0.05f, 0.05f, sceneObserver);
	if (auto* c = dynamic_cast<UICheckBox*>(chkFull.get()))
		c->isChecked = Engine::GetInstance().window->IsFullscreen();
	settingsButtons.push_back(chkFull);
	setY += spacing;

	auto btnBack = Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, 9, "BACK", 0.5f, setY, wPerc, hPerc, sceneObserver);
	settingsButtons.push_back(btnBack);
	ShowSettings(false);
	return true;
}

bool MenuScene::Update(float dt) {
	if (menuBackground != nullptr) {
		int screenW, screenH;
		Engine::GetInstance().window->GetWindowSize(screenW, screenH);

		SDL_Rect fullScreenRect = { 0, 0, screenW, screenH };

		Engine::GetInstance().render->DrawTextureScaled(menuBackground, fullScreenRect);
	}
	return true;
}

bool MenuScene::OnUIMouseClickEvent(UIElement* uiElement) {
	auto sceneManager = Engine::GetInstance().sceneManager;

	switch (uiElement->id)
	{
		//--- MainMenu-- -
	case 1: // PLAY
		LOG("Boton Play presionado");
		//Player::ResetSavedCheckpoint();
		sceneManager->ChangeScene(SceneID::GAME);
		break;

	case 2: // CONTINUE
		LOG("Boton Continue presionado");

		break;

	case 3: // SETTINGS
		LOG("Ir a Settings");
		ShowSettings(true);
		Engine::GetInstance().input->ClearMouseInput();
		break;

	case 5: // EXIT
		LOG("Salir del juego");
		exitGame = true;
		break;


		//--- SettingMenu---
	case 6: // MUSIC SLIDER
	{
		UISlider* slider = (UISlider*)uiElement;
		float volume = slider->GetValue();

		Engine::GetInstance().audio->SetMusicVolume(volume);
		break;
	}
	case 7: // FX SLIDER
	{
		UISlider* slider = (UISlider*)uiElement;
		float volume = slider->GetValue();

		Engine::GetInstance().audio->SetSFXVolume(volume);
		break;
	}
	case 8:
	{
		UICheckBox* checkbox = (UICheckBox*)uiElement;
		Engine::GetInstance().window->SetFullscreen(checkbox->isChecked);
		RecalculateBackgroundScale();
		break;
	}
	case 9: // BACK de Settings
		ShowSettings(false);
		break;

	default:
		break;
		return true;
	}
}


void MenuScene::ShowSettings(bool show) {
	//Main Menu
	for (auto& elem : mainButtons) {
		if (show == true) {
			//If we are displaying Settings->HIDE Main Menu
			elem->visible = false;
			elem->state = UIElementState::DISABLED;
		}
		else {
			elem->visible = true;
			elem->state = UIElementState::NORMAL;
		}
	}
	//Setting Menu
	for (auto& elem : settingsButtons) {
		if (show == true) {
			elem->visible = true;
			elem->state = UIElementState::NORMAL;
		}
		else {
			elem->visible = false;
			elem->state = UIElementState::DISABLED;
		}
	}
}


bool MenuScene::CleanUp() {
	Engine::GetInstance().uiManager->CleanUp();

	if (menuBackground != nullptr) {
		Engine::GetInstance().textures->UnLoad(menuBackground);
		menuBackground = nullptr;
	}
	mainButtons.clear();
	settingsButtons.clear();
	return true;
}

void MenuScene::RecalculateBackgroundScale()
{
	if (menuBackground == nullptr) return;

	int screenW, screenH;
	Engine::GetInstance().window->GetWindowSize(screenW, screenH);

	float texW, texH;
	SDL_GetTextureSize(menuBackground, &texW, &texH);

	float engineScale = (float)Engine::GetInstance().window->GetScale();

	bgScaleX = (float)screenW / (texW * engineScale);
	bgScaleY = (float)screenH / (texH * engineScale);
}