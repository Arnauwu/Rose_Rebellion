#include "MenuScene.h"
#include "Engine.h"
#include "UIManager.h"
#include "SceneManager.h"
#include "GameManager.h"
#include "Input.h"

#include "Window.h"
#include "Log.h"

MenuScene::MenuScene() : SceneBase() {}
MenuScene::~MenuScene() {}

bool MenuScene::Start() {

	Engine::GetInstance().audio->PlayMusic("Assets/Audio/Music/MusicaInteriorCastillo.wav");
	uiClick = Engine::GetInstance().audio->LoadFx("Assets/Audio/Fx/DoorClosed.wav");
	uiHover = Engine::GetInstance().audio->LoadFx("Assets/Audio/Fx/DoorClosed.wav");

	if (menuBackground == nullptr) {
		menuBackground = Engine::GetInstance().textures->Load("Assets/Textures/UI/MainMenu/MainMenu.png");
	}
	if (menuBackground_S == nullptr) {
		menuBackground_S = Engine::GetInstance().textures->Load("Assets/Textures/UI/MainMenu/MainMenu_S.png");
	}
	if (frameTex == nullptr) {
		frameTex = Engine::GetInstance().textures->Load("Assets/Textures/UI/Buttons/frameTex.png");
	}
	if (flashTex == nullptr) {
		flashTex = Engine::GetInstance().textures->Load("Assets/1.png");
	}
	std::unordered_map<int, std::string> aliases = {
		{0, "bullet"}
	};
	anims.LoadFromTSX("Assets/2.tsx", aliases);
	anims.SetCurrent("bullet");


	// Textura temporal para pruebas (testear)
	texture = Engine::GetInstance().textures->Load("Assets/2.png");

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
		auto btn = uiManager->CreateUIElement(UIElementType::BUTTON, def.id, def.text, 0.5f, currentY, wPerc, hPerc, sceneObserver);
		
		if (auto* b = dynamic_cast<UIButton*>(btn.get())) {
			b->SetFrameTexture(frameTex);
			Animation* bulletAnim = anims.GetAnim("bullet");
			//b->SetFlashTexture(flashTex);
			if (bulletAnim != nullptr) {
				b->SetFlashAnimation(texture, *bulletAnim);
			}

		}
		mainButtons.push_back(btn);
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
	if (auto* b = dynamic_cast<UIButton*>(btnBack.get())) {
		b->SetFlashTexture(flashTex); 
		b->SetFrameTexture(frameTex); 
	}

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

	bool isAnyHovered = false;

	// Juntamos todos los botones activos según en qué menú estemos
	auto& activeButtons = isSettingsOpen ? settingsButtons : mainButtons;

	for (auto& btn : activeButtons) {
		// Tu UIElement llama "FOCUSED" al estado cuando el ratón está encima
		if (btn->visible && btn->state == UIElementState::FOCUSED) {
			isAnyHovered = true;

			// Si el ratón acaba de entrar en este botón
			if (lastHoveredId != btn->id) {
				Engine::GetInstance().audio->PlayFx(uiHover); // ¡Suena!
				lastHoveredId = btn->id; // Guardamos el ID
			}
		}
	}

	// Si el ratón no está encima de NINGÚN botón, reseteamos el ID
	if (!isAnyHovered) {
		lastHoveredId = -1;
	}

	return true;
}

bool MenuScene::OnUIMouseClickEvent(UIElement* uiElement) {
	Engine::GetInstance().audio->PlayFx(uiClick);
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
			LOG("Error: No se ha encontrado partida o el archivo está corrupto.");
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
	if (frameTex != nullptr) {
		Engine::GetInstance().textures->UnLoad(frameTex);
		frameTex = nullptr;
	}

	mainButtons.clear();
	settingsButtons.clear();
	return true;
}

