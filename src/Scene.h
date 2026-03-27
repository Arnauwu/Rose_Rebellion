#pragma once

#include "Module.h"
#include "Player.h"

enum class SceneID {
	INTRO,
	MENU,
	CASTLE,
	LEVEL_TRANSITION,
	LEVEL2,
	WIN,
	GAMEOVER
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

public:

	// Public Variables
	SceneID currentScene = SceneID::INTRO; // Start in INTRO
	SceneID lastLevelPlayed = SceneID::CASTLE;

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

	float introTimer = 0.0f;
	float transitionTimer = 0.0f;
	float gameOverTimer = 0.0f;
	float winTimer = 0.0f;

};