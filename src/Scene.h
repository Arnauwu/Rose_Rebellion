#pragma once

#include "Module.h"
#include "Player.h"
#include "UIButton.h"
#include "UICheckBox.h"
#include "UISlider.h"

enum class SceneID {
	INTRO,
	MENU,
	GAME,
	WIN,
	GAMEOVER
};

enum class MenuState {
	MAIN,
	SETTINGS,
	CREDITS,

	NONE,
	PAUSE,
	PAUSE_SETTINGS,
};
struct SDL_Texture;

class Scene : public Module
{
public:

	Scene();

	// Destructor
	virtual ~Scene();

	// Called before render is available
	bool Awake();

	// Called before the first frame
	bool Start();

	// Called before all Updates
	bool PreUpdate();

	// Called each loop iteration
	bool Update(float dt);

	// Called before all Updates
	bool PostUpdate();

	// Called before quitting
	bool CleanUp();

	void LoadScene(SceneID newScene);
	void ChangeScene(SceneID newScene);
	void UnloadCurrentScene();

	void ResetLevel();

	// Return the player position
	Vector2D GetPlayerPosition();

	//Load
	void LoadMap(std::string mapFile);

	std::shared_ptr<Player> GetPlayer() const { return player; };
	void SetPlayer(std::shared_ptr<Player> p) { player = p; };

	bool setNewMap = false;

	// Handles multiple Gui Event methods
	bool OnUIMouseClickEvent(UIElement* uiElement);

public:

	// Public Variables
	SceneID currentScene = SceneID::INTRO; // Start in INTRO
	SceneID lastLevelPlayed = SceneID::GAME;

	SDL_Texture* introTexture = nullptr;
	SDL_Texture* menuBackground = nullptr;
	SDL_Texture* gameOverTexture = nullptr;
	SDL_Texture* transitionTexture = nullptr;

	std::shared_ptr<Player> player;


private:
	
	//Intro
	void LoadIntro();
	void UpdateIntro(float dt);
	void UnloadIntro();

	//Menu
	void LoadMainMenu();
	void UpdateMainMenu(float dt);
	void UnloadMainMenu();
	void HandleMainMenuUIEvents(UIElement* uiElement);
	
	//Show
	void ShowSettings(bool show);
	void ShowCredits(bool show);
	void LoadPauseUI();
	void ShowPauseMenu(bool show);
	void ShowPauseSettings(bool show);
	void HandleGameMenuUIEvents(UIElement* uiElement);

	//Levels
	void LoadCastle();
	void UpdateCastle(float dt);
	void PostUpdateCastle();
	void UnloadCastle();

	void LoadLevel2();
	void UpdateLevel2(float dt);
	void PostUpdateLevel2();
	void UnloadLevel2();

	//Game Over
	void LoadGameOver();
	void UpdateGameOver(float dt);
	void UnloadGameOver();

	//Win
	void LoadWin();
	void UpdateWin(float dt);
	void UnloadWin();

	//Transitions
	void LoadLevelTransition();
	void UpdateLevelTransition(float dt);


private:
	// Private Variables
	MenuState menuState = MenuState::MAIN;

	std::vector<std::shared_ptr<UIElement>> mainMenuElements;
	std::vector<std::shared_ptr<UIElement>> settingsMenuElements;
	std::vector<std::shared_ptr<UIElement>> gameMenuElements;

	bool exitGame = false;


	int windowW = 0;
	int windowH = 0;
	SDL_Rect screenRect = { 0, 0, 0, 0 };

	float introTimer = 0.0f;
	float transitionTimer = 0.0f;
	float gameOverTimer = 0.0f;
	float winTimer = 0.0f;

};