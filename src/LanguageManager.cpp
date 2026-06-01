#include "LanguageManager.h"
#include "Log.h"
#include <fstream>
#include <nlohmann/json.hpp> // Asegúrate de incluir la librería JSON

using json = nlohmann::json;

LanguageManager::LanguageManager() : Module(), currentLanguage(Language::CATALAN)
{
    name = "LanguageManager";
}

LanguageManager::~LanguageManager() {}

bool LanguageManager::Start()
{
    LoadLanguageFiles();
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
    if (!isLoaded) {
        LoadLanguageFiles();
    }

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

void LanguageManager::ClearCallbacks()
{
    languageChangeCallbacks.clear();
}

void LanguageManager::LoadLanguageFiles() const
{
    if (isLoaded) return;

    std::ifstream fileEN("Assets/Settings/ui_strings_en.json");
    if (fileEN.is_open()) {
        json j;
        fileEN >> j;
        for (auto& element : j.items()) {
            englishStrings[element.key()] = element.value().get<std::string>();
        }
        LOG("UI Strings (English) cargados correctamente.");
    }
    else {
        LOG("Error: No se pudo abrir ui_strings_en.json");
    }

    std::ifstream fileCA("Assets/Settings/ui_strings_cat.json");
    if (fileCA.is_open()) {
        json j;
        fileCA >> j;
        for (auto& element : j.items()) {
            catalanStrings[element.key()] = element.value().get<std::string>();
        }
        LOG("UI Strings (Catalan) cargados correctamente.");
    }
    else {
        LOG("Error: No se pudo abrir ui_strings_cat.json");
    }

    isLoaded = true;
}