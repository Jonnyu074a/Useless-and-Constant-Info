#include <Geode/Geode.hpp>
#include <Geode/loader/SettingV3.hpp>
#include <Geode/modify/AppDelegate.hpp>
#include "../include/LabelConfig.hpp"
#include "../include/UACI.hpp"
#include "../include/InfoOverlay.hpp"
#include "utils/Helpers.hpp"

using namespace geode::prelude;
using namespace uaci;

$execute {
    // Load saved settings into g_config
    g_config.load();

    // Static cache (GD version, Geode version, OS string) + persisted play time
    g_cache.build();
    initSession();

    // Any setting changed from Geode's own settings screen: reload + refresh.
    // Ignored while WE are saving (g_suppressReload) so partial state is never reloaded.
    listenForAllSettingChanges([](std::string_view, std::shared_ptr<SettingV3>) {
        if (g_suppressReload) return;
        g_config.load();
        refreshOverlay();
    });
}

// Flush accumulated play time when the game goes to background
// (the overlay also flushes every 60 s as a safety net).
class $modify(AppDelegateHook, AppDelegate) {
    void applicationWillResignActive() {
        uaci::flushPlayTime();
        AppDelegate::applicationWillResignActive();
    }
};
