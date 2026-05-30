#pragma once
#include "Module.h"
#include <map>
#include <string>
#include <functional>
#include <vector>

enum class Language {
    ENGLISH = 0,
    CATALAN = 1
};

class LanguageManager : public Module {
public:
    LanguageManager();
    virtual ~LanguageManager();

    bool Start() override;
    bool Update(float dt) override;
    bool CleanUp() override;

    // Obtener el idioma actual
    Language GetCurrentLanguage() const { return currentLanguage; }

    // Establecer idioma
    void SetLanguage(Language lang);

    // Obtener string traducido
    std::string GetString(const std::string& key) const;

    // Registrar callback para cuando cambie el idioma
    void RegisterLanguageChangeCallback(std::function<void(Language)> callback);

private:
    Language currentLanguage;
    std::map<std::string, std::string> englishStrings;
    std::map<std::string, std::string> catalanStrings;
    std::vector<std::function<void(Language)>> languageChangeCallbacks;

    void InitializeDefaultStrings();
};