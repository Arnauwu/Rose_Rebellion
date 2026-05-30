#include "LanguageManager.h"
#include "Log.h"

LanguageManager::LanguageManager() : Module(), currentLanguage(Language::CATALAN)
{
    name = "LanguageManager";
}

LanguageManager::~LanguageManager() {}

bool LanguageManager::Start()
{
    InitializeDefaultStrings();
    return true;
}

bool LanguageManager::Update(float dt)
{
    return true;
}

bool LanguageManager::CleanUp()
{
    englishStrings.clear();
    catalanStrings.clear();
    languageChangeCallbacks.clear();
    return true;
}

void LanguageManager::SetLanguage(Language lang)
{
    if (currentLanguage != lang) {
        currentLanguage = lang;
        LOG("Language changed to: %s", currentLanguage == Language::ENGLISH ? "English" : "Catalan");

        // Notificar a todos los observadores registrados
        for (const auto& callback : languageChangeCallbacks) {
            callback(currentLanguage);
        }
    }
}

std::string LanguageManager::GetString(const std::string& key) const
{
    if (currentLanguage == Language::ENGLISH) {
        auto it = englishStrings.find(key);
        if (it != englishStrings.end()) {
            return it->second;
        }
    }
    else if (currentLanguage == Language::CATALAN) {
        auto it = catalanStrings.find(key);
        if (it != catalanStrings.end()) {
            return it->second;
        }
    }
    return key;
}

void LanguageManager::RegisterLanguageChangeCallback(std::function<void(Language)> callback)
{
    languageChangeCallbacks.push_back(callback);
}

void LanguageManager::InitializeDefaultStrings()
{
    // ENGLISH
    englishStrings["BTN_PLAY"] = "Start Game";
    englishStrings["BTN_CONTINUE"] = "Load Game";
    englishStrings["BTN_SETTINGS"] = "Settings";
    englishStrings["BTN_EXIT"] = "Quit Game";
    englishStrings["SLD_MUSIC"] = "Music Volume";
    englishStrings["SLD_FX"] = "FX Volume";
    englishStrings["CHK_FULLSCREEN"] = "Fullscreen";
    englishStrings["BTN_BACK"] = "BACK";
    englishStrings["BTN_ENGLISH"] = "English";
    englishStrings["BTN_CATALAN"] = "Catalan";

    // CATALAN
    catalanStrings["BTN_PLAY"] = "Comen\xc3\xa7" "ar Joc";
    catalanStrings["BTN_CONTINUE"] = "Carregar Joc";
    catalanStrings["BTN_SETTINGS"] = "Configuraci\xc3\xb3";
    catalanStrings["BTN_EXIT"] = "Sortir";
    catalanStrings["SLD_MUSIC"] = "Volum M\xc3\xbasica";
    catalanStrings["SLD_FX"] = "Volum Efectes";
    catalanStrings["CHK_FULLSCREEN"] = "Pantalla Completa";
    catalanStrings["BTN_BACK"] = "ENRERE";
    catalanStrings["BTN_ENGLISH"] = "English";
    catalanStrings["BTN_CATALAN"] = "Catal\xc3\xa0";
}