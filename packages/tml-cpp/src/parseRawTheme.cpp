#include "parseRawTheme.h"
#include "rapidjson/document.h"
#include "rapidjson/error/en.h"
#include <stdexcept>

using namespace rapidjson;

namespace tml {

IRawTheme* parseRawTheme(const std::string& content) {
    try {
        Document doc;
        doc.Parse(content.c_str());

        if (doc.HasParseError()) {
            return nullptr;
        }

        // Create IRawTheme from JSON
        auto rawTheme = new IRawTheme();

        // Get theme name
        if (doc.HasMember("name") && doc["name"].IsString()) {
            rawTheme->name = new std::string(doc["name"].GetString());
        }

        // Parse settings array (supports both "settings" and "tokenColors" keys)
        // VSCode themes use "tokenColors", TextMate themes use "settings"
        const Value* settingsArray = nullptr;
        if (doc.HasMember("tokenColors") && doc["tokenColors"].IsArray()) {
            settingsArray = &doc["tokenColors"];
        } else if (doc.HasMember("settings") && doc["settings"].IsArray()) {
            settingsArray = &doc["settings"];
        }

        if (settingsArray) {
            for (const auto& settingObj : settingsArray->GetArray()) {
                if (!settingObj.IsObject()) {
                    continue;
                }

                auto setting = new IRawThemeSetting();

                // Get name
                if (settingObj.HasMember("name") && settingObj["name"].IsString()) {
                    setting->name = new std::string(settingObj["name"].GetString());
                }

                // Get scope(s)
                if (settingObj.HasMember("scope")) {
                    const auto& scopeVal = settingObj["scope"];
                    if (scopeVal.IsString()) {
                        setting->scopeString = scopeVal.GetString();
                        auto scopeVec = new std::vector<std::string>();
                        scopeVec->push_back(setting->scopeString);
                        setting->scope = scopeVec;
                    } else if (scopeVal.IsArray()) {
                        setting->scope = new std::vector<std::string>();
                        for (const auto& scopeStr : scopeVal.GetArray()) {
                            if (scopeStr.IsString()) {
                                setting->scope->push_back(scopeStr.GetString());
                            }
                        }
                    }
                }

                // Get settings object
                if (settingObj.HasMember("settings") && settingObj["settings"].IsObject()) {
                    const auto& settingsObj = settingObj["settings"];

                    if (settingsObj.HasMember("fontStyle") && settingsObj["fontStyle"].IsString()) {
                        setting->settings.fontStyle = new std::string(settingsObj["fontStyle"].GetString());
                    }
                    if (settingsObj.HasMember("foreground") && settingsObj["foreground"].IsString()) {
                        setting->settings.foreground = new std::string(settingsObj["foreground"].GetString());
                    }
                    if (settingsObj.HasMember("background") && settingsObj["background"].IsString()) {
                        setting->settings.background = new std::string(settingsObj["background"].GetString());
                    }
                }

                rawTheme->settings.push_back(setting);
            }
        }

        // VSCode themes have editor colors in "colors" section
        // Extract editor.foreground and editor.background as default theme colors
        if (doc.HasMember("colors") && doc["colors"].IsObject()) {
            const auto& colors = doc["colors"];

            std::string* defaultForeground = nullptr;
            std::string* defaultBackground = nullptr;

            if (colors.HasMember("editor.foreground") && colors["editor.foreground"].IsString()) {
                defaultForeground = new std::string(colors["editor.foreground"].GetString());
            }
            if (colors.HasMember("editor.background") && colors["editor.background"].IsString()) {
                defaultBackground = new std::string(colors["editor.background"].GetString());
            }

            // Create a default setting with empty scope for editor colors
            if (defaultForeground || defaultBackground) {
                auto defaultSetting = new IRawThemeSetting();
                // Empty scope means this is the default
                defaultSetting->scope = nullptr;
                defaultSetting->scopeString = "";

                if (defaultForeground) {
                    defaultSetting->settings.foreground = defaultForeground;
                }
                if (defaultBackground) {
                    defaultSetting->settings.background = defaultBackground;
                }

                // Insert at beginning so it acts as the base default
                rawTheme->settings.insert(rawTheme->settings.begin(), defaultSetting);
            }
        }

        return rawTheme;
    } catch (...) {
        return nullptr;
    }
}

Theme* parseJSONTheme(const std::string& content) {
    try {
        IRawTheme* rawTheme = parseRawTheme(content);
        if (!rawTheme) {
            return nullptr;
        }

        // Create Theme from raw theme
        Theme* theme = Theme::createFromRawTheme(rawTheme);
        delete rawTheme;

        return theme;
    } catch (...) {
        return nullptr;
    }
}

} // namespace tml
