#include "MenuScene.h"
#include "Engine.h"
#include "UIManager.h"
#include "SceneManager.h"
#include "Window.h"
#include "Log.h"

MenuScene::MenuScene() : SceneBase() {}
MenuScene::~MenuScene() {}

bool MenuScene::Start() {

	Engine::GetInstance().audio->PlayMusic("Assets/Audio/Music/MusicaInteriorCastillo.wav");
	uiClick = Engine::GetInstance().audio->LoadFx("Assets/Audio/Fx/MusicaClicMenu.wav");

	if (menuBackground == nullptr) {
		menuBackground = Engine::GetInstance().textures->Load("Assets/Textures/UI/MainMenu/MainMenu.png");
	}
	if (menuBackground_S == nullptr) {
		menuBackground_S = Engine::GetInstance().textures->Load("Assets/Textures/UI/MainMenu/MainMenu_S.png");
	}

	auto uiManager = Engine::GetInstance().uiManager;
	Module* sceneObserver = (Module*)Engine::GetInstance().sceneManager.get();
	
	struct ButtonDef { int id; const char* text; };

	float wPerc = 0.20f; // 25% of the screen width
	float hPerc = 0.08f; // 8% of the top of the screen.
	float spacing = 0.09f; // Space between buttons 
	float currentY = 0.60f; // Hight
	
	ButtonDef mainBtnDefs[] = {
		{ (int)MenuUI_ID::BTN_PLAY,     "Start Game" },
		{ (int)MenuUI_ID::BTN_CONTINUE, "Load Game" },
		{ (int)MenuUI_ID::BTN_SETTINGS, "Settings" },
		{ (int)MenuUI_ID::BTN_EXIT,     "Quit Game" }
	};

	for (const auto& def : mainBtnDefs) {
		mainButtons.push_back(uiManager->CreateUIElement(UIElementType::BUTTON, def.id, def.text, 0.5f, currentY, wPerc, hPerc, sceneObserver));
		currentY += spacing;
	}

	// MAIN MENU SETTINGS
	float setY = 0.45f;

	auto sldMusic = Engine::GetInstance().uiManager->CreateUIElement(UIElementType::SLIDER, (int)MenuUI_ID::SLD_MUSIC, "Music Volume", 0.5f, setY, 0.3f, 0.05f, sceneObserver);
	if (auto* s = dynamic_cast<UISlider*>(sldMusic.get())) s->SetValue(Engine::GetInstance().audio->GetMusicVolume());
	settingsButtons.push_back(sldMusic);
	setY += spacing;

	auto sldFX = Engine::GetInstance().uiManager->CreateUIElement(UIElementType::SLIDER, (int)MenuUI_ID::SLD_FX, "FX Volume", 0.5f, setY, 0.3f, 0.05f, sceneObserver);
	if (auto* s = dynamic_cast<UISlider*>(sldFX.get())) s->SetValue(Engine::GetInstance().audio->GetSFXVolume());
	settingsButtons.push_back(sldFX);
	setY += spacing;

	auto chkFull = Engine::GetInstance().uiManager->CreateUIElement(UIElementType::CHECKBOX, (int)MenuUI_ID::CHK_FULLSCREEN, "Fullscreen", 0.5f, setY, 0.05f, 0.05f, sceneObserver);
	if (auto* c = dynamic_cast<UICheckBox*>(chkFull.get())) c->isChecked = Engine::GetInstance().window->IsFullscreen();
	settingsButtons.push_back(chkFull);
	setY += spacing;

	auto btnBack = Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, (int)MenuUI_ID::BTN_BACK, "BACK", 0.5f, setY, wPerc, hPerc, sceneObserver);
	settingsButtons.push_back(btnBack);

	ShowSettings(false);
	return true;
}

bool MenuScene::Update(float dt) {
	int windowW = Engine::GetInstance().render->camera.w;
	int windowH = Engine::GetInstance().render->camera.h;
	
	SDL_Rect fullScreenRect = { 0, 0, windowW, windowH };

	SDL_Texture* currentBackground = isSettingsOpen ? menuBackground_S : menuBackground; // If false = menuBackground

	// Dibujamos la textura elegida
	if (currentBackground != nullptr) {
		Engine::GetInstance().render->DrawTextureScaled(currentBackground, fullScreenRect);
	}

	return true;
}

bool MenuScene::OnUIMouseClickEvent(UIElement* uiElement) {
	Engine::GetInstance().audio->PlayFx(uiClick);
    auto sceneManager = Engine::GetInstance().sceneManager;

    switch (uiElement->id)
    {
    case (int)MenuUI_ID::BTN_PLAY:
        sceneManager->ChangeScene(SceneID::GAME);
        break;
    case (int)MenuUI_ID::BTN_CONTINUE:
        // Lógica de continuar
        break;
    case (int)MenuUI_ID::BTN_SETTINGS:
        ShowSettings(true);
        Engine::GetInstance().input->ClearMouseInput();
		return true;
        break;
    case (int)MenuUI_ID::BTN_EXIT:
        exitGame = true;
        break;
    case (int)MenuUI_ID::SLD_MUSIC:
        Engine::GetInstance().audio->SetMusicVolume(((UISlider*)uiElement)->GetValue());
        break;
    case (int)MenuUI_ID::SLD_FX:
        Engine::GetInstance().audio->SetSFXVolume(((UISlider*)uiElement)->GetValue());
        break;
    case (int)MenuUI_ID::CHK_FULLSCREEN:
        Engine::GetInstance().window->SetFullscreen(((UICheckBox*)uiElement)->isChecked);
        break;
    case (int)MenuUI_ID::BTN_BACK:
        ShowSettings(false);
        break;
    default:
        break;
    }
    return true;
}

void MenuScene::ShowSettings(bool show) {
	isSettingsOpen = show;
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
	if (menuBackground_S != nullptr) {
		Engine::GetInstance().textures->UnLoad(menuBackground_S);
		menuBackground_S = nullptr;
	}
	mainButtons.clear();
	settingsButtons.clear();
	return true;
}

