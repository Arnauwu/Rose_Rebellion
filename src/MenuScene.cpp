#include "MenuScene.h"
#include "Engine.h"
#include "UIManager.h"
#include "SceneManager.h"
#include "GameManager.h"
#include "Input.h"
#include "Cinematics.h"
#include "LanguageManager.h"

#include "Window.h"
#include "Log.h"

MenuScene::MenuScene() : SceneBase() {}
MenuScene::~MenuScene() {}

bool MenuScene::Start() {
	SDL_ShowCursor();
	Engine::GetInstance().cinematics->CloseVideo();
	Engine::GetInstance().audio->PlayMusic("Assets/Audio/Music/MusicaInteriorCastillo.wav");
	uiClick = Engine::GetInstance().audio->LoadFx("Assets/Audio/Fx/UI/clicButton.wav"); 
	uiHover = Engine::GetInstance().audio->LoadFx("Assets/Audio/Fx/UI/selecButton.wav");

	if (menuBackground == nullptr) {
		menuBackground = Engine::GetInstance().textures->Load("Assets/Textures/UI/MainMenu/MainMenu.png");
	}
	if (menuBackground_S == nullptr) {
		menuBackground_S = Engine::GetInstance().textures->Load("Assets/Textures/UI/MainMenu/MainMenu_S.png");
	}

	if (sliderThumbTex == nullptr) {
		sliderThumbTex = Engine::GetInstance().textures->Load("Assets/Textures/UI/Buttons/pomo.png");
	}

	if (mainFrame == nullptr) {
		mainFrame = Engine::GetInstance().textures->Load("Assets/Textures/UI/Buttons/frameTex.png");
	}
	if (mainClick == nullptr) {
		mainClick = Engine::GetInstance().textures->Load("Assets/Textures/UI/Buttons/clickAnim.png");
	}

	auto uiManager = Engine::GetInstance().uiManager;
	Module* sceneObserver = (Module*)Engine::GetInstance().sceneManager.get();
	
	struct ButtonDef { int id; const char* keyId; };

	float wPerc = 0.20f; // 25% of the screen width
	float hPerc = 0.08f; // 8% of the top of the screen.
	float spacing = 0.09f; // Space between buttons 
	float currentY = 0.60f; // Hight
	
	ButtonDef mainBtnDefs[] = {
		{ (int)MenuUI_ID::BTN_PLAY,     "BTN_PLAY" },
		{ (int)MenuUI_ID::BTN_CONTINUE, "BTN_CONTINUE" },
		{ (int)MenuUI_ID::BTN_SETTINGS, "BTN_SETTINGS" },
		{ (int)MenuUI_ID::BTN_EXIT,     "BTN_EXIT" }
	};

	for (const auto& def : mainBtnDefs) {
		std::string localizedText = Engine::GetInstance().languageManager->GetString(def.keyId);
		auto btn = uiManager->CreateUIElement(UIElementType::BUTTON, def.id, localizedText.c_str(), 0.5f, currentY, wPerc, hPerc, sceneObserver);
		
		if (auto* b = dynamic_cast<UIButton*>(btn.get())) {
			b->SetFrameTexture(mainFrame);
			b->SetClickTexture(mainClick);
			b->SetSounds(uiHover, uiClick);
		}
		
		mainButtons.push_back(btn);
		currentY += spacing;
	}

	// MAIN MENU SETTINGS
	float setY = 0.45f;

	auto sldMusic = Engine::GetInstance().uiManager->CreateUIElement(UIElementType::SLIDER, (int)MenuUI_ID::SLD_MUSIC, Engine::GetInstance().languageManager->GetString("SLD_MUSIC").c_str(), 0.5f, setY, 0.3f, 0.05f, sceneObserver);

	if (auto* s = dynamic_cast<UISlider*>(sldMusic.get())) {
		s->SetValue(Engine::GetInstance().audio->GetMusicVolume());
		s->SetThumbTexture(sliderThumbTex);
	}
	settingsButtons.push_back(sldMusic);

	setY += spacing;

	auto sldFX = Engine::GetInstance().uiManager->CreateUIElement(UIElementType::SLIDER, (int)MenuUI_ID::SLD_FX, Engine::GetInstance().languageManager->GetString("SLD_FX").c_str(), 0.5f, setY, 0.3f, 0.05f, sceneObserver);
	if (auto* s = dynamic_cast<UISlider*>(sldFX.get())) {
		s->SetThumbTexture(sliderThumbTex);
		s->SetValue(Engine::GetInstance().audio->GetSFXVolume());
	}
	settingsButtons.push_back(sldFX);
	setY += spacing;

	auto chkFull = Engine::GetInstance().uiManager->CreateUIElement(UIElementType::CHECKBOX, (int)MenuUI_ID::CHK_FULLSCREEN, Engine::GetInstance().languageManager->GetString("CHK_FULLSCREEN").c_str(), 0.5f, setY, 0.05f, 0.05f, sceneObserver);
	if (auto* c = dynamic_cast<UICheckBox*>(chkFull.get())) c->isChecked = Engine::GetInstance().window->IsFullscreen();
	settingsButtons.push_back(chkFull);
	setY += spacing;

	auto btnEnglish = Engine::GetInstance().uiManager->CreateUIElement(
		UIElementType::BUTTON, (int)MenuUI_ID::BTN_ENGLISH, "English",
		0.4f, setY, wPerc, hPerc, sceneObserver
	);
	if (auto* b = dynamic_cast<UIButton*>(btnEnglish.get())) {
		b->SetClickTexture(mainClick);
		b->SetSounds(uiHover, uiClick);
	}
	settingsButtons.push_back(btnEnglish);

	auto btnCatalan = Engine::GetInstance().uiManager->CreateUIElement(
		UIElementType::BUTTON, (int)MenuUI_ID::BTN_CATALAN, "Catal\xc3\xa0",
		0.6f, setY, wPerc, hPerc, sceneObserver
	);
	if (auto* b = dynamic_cast<UIButton*>(btnCatalan.get())) {
		b->SetClickTexture(mainClick);
		b->SetSounds(uiHover, uiClick);
	}
	settingsButtons.push_back(btnCatalan);
	setY += spacing;

	auto btnBack = Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, (int)MenuUI_ID::BTN_BACK, Engine::GetInstance().languageManager->GetString("BTN_BACK").c_str(), 0.5f, setY, wPerc, hPerc, sceneObserver);
	settingsButtons.push_back(btnBack);
	if (auto* b = dynamic_cast<UIButton*>(btnBack.get())) {
		b->SetFrameTexture(mainFrame);
		b->SetClickTexture(mainClick);
		b->SetSounds(uiHover, uiClick);
	}

	Engine::GetInstance().input->SetNavigationEnabled(true);

	Engine::GetInstance().languageManager->RegisterLanguageChangeCallback(
		[this](Language lang) { this->UpdateUILanguage(); }
	);
	UpdateUILanguage();
	ShowSettings(false);
	return true;
}

bool MenuScene::Update(float dt) {
	int screenW, screenH;
	screenW = Engine::GetInstance().window->windowWidth;

	screenH = Engine::GetInstance().window->windowHeight;


	SDL_Rect fullScreenRect = { 0, 0, screenW, screenH };

	SDL_Texture* currentBackground = isSettingsOpen ? menuBackground_S : menuBackground; // If false = menuBackground

	// Dibujamos la textura elegida
	if (currentBackground != nullptr) {
		Engine::GetInstance().render->DrawTextureScaled(currentBackground, fullScreenRect);
	}

	return true;
}

bool MenuScene::OnUIMouseClickEvent(UIElement* uiElement) {
    auto sceneManager = Engine::GetInstance().sceneManager;

    switch (uiElement->id)
    {
    case (int)MenuUI_ID::BTN_PLAY:
		GameManager::GetInstance().StartNewGame();
		Engine::GetInstance().audio->StopMusic();
		SDL_Delay(150);
        sceneManager->ChangeScene(SceneID::INTRO_CINEMATIC);
        break;
    case (int)MenuUI_ID::BTN_CONTINUE:
		if (GameManager::GetInstance().LoadGame("savegame.xml")) {
			LOG("Partida cargada con éxito. Entrando al juego...");
			SDL_Delay(150);
			sceneManager->ChangeScene(SceneID::GAME);
		}
		else {
			LOG("Error: No se ha encontrado partida o el archivo est?corrupto.");
		}
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
	case (int)MenuUI_ID::BTN_ENGLISH:
		LOG("Language changed to: English");
		Engine::GetInstance().languageManager->SetLanguage(Language::ENGLISH);
		return true;

	case (int)MenuUI_ID::BTN_CATALAN:
		LOG("Language changed to: Catalan");
		Engine::GetInstance().languageManager->SetLanguage(Language::CATALAN);
		return true;
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

void MenuScene::UpdateUILanguage() {
	auto langMgr = Engine::GetInstance().languageManager;

	// Actualizar botones principales
	if (mainButtons.size() >= 4) {
		mainButtons[0]->text = langMgr->GetString("BTN_PLAY");
		mainButtons[1]->text = langMgr->GetString("BTN_CONTINUE");
		mainButtons[2]->text = langMgr->GetString("BTN_SETTINGS");
		mainButtons[3]->text = langMgr->GetString("BTN_EXIT");
	}

	// Actualizar botones de configuración
	if (settingsButtons.size() >= 6) {
		settingsButtons[0]->text = langMgr->GetString("SLD_MUSIC");
		settingsButtons[1]->text = langMgr->GetString("SLD_FX");
		settingsButtons[2]->text = langMgr->GetString("CHK_FULLSCREEN");
		settingsButtons[3]->text = langMgr->GetString("BTN_ENGLISH");
		settingsButtons[4]->text = langMgr->GetString("BTN_CATALAN");
		settingsButtons[5]->text = langMgr->GetString("BTN_BACK");

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
	if (mainClick != nullptr) {
		Engine::GetInstance().textures->UnLoad(mainClick);
		mainClick = nullptr;
	}
	if (mainFrame != nullptr) {
		Engine::GetInstance().textures->UnLoad(mainFrame);
		mainFrame = nullptr;
	}
	Engine::GetInstance().audio->StopMusic();
	mainButtons.clear();
	settingsButtons.clear();
	return true;
}

