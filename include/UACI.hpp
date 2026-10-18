#pragma once
#include <Geode/Geode.hpp>
#include <string>
#include <vector>
#include <deque>
#include <chrono>
#include <ctime>
#include <cstdint>
#include <utility>

using namespace geode::prelude;

namespace uaci {

// Reads GD "SeedValueRSV"-like wrappers (.value()) or plain ints, whichever the bindings use.
template <class T>
inline int valInt(T& v) {   // non-const: SeedValue*::value() / operator int() are not const
    if constexpr (requires { v.value(); }) return static_cast<int>(v.value());
    else return static_cast<int>(v);
}

inline double nowSeconds() {
    return std::chrono::duration<double>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

// While true, the "any setting changed" listener ignores events (we are saving
// our own values and must not reload half-written state).
inline bool g_suppressReload = false;

// ── Mod menu detection (locally installed only) ───────────────────────────────
inline const std::vector<std::string> KNOWN_MOD_MENUS = {
    "absolllute.megahack", "prevter.openhack",
    "mat.betteredit",      "cvolton.gdmegaoverlay",
};

// ── Static cache ──────────────────────────────────────────────────────────────
struct StaticCache {
    bool        ready = false;
    std::string gdVersion;
    std::string geodeVersion;
    std::string osBase;
    size_t      totalMods  = 0;
    size_t      loadedMods = 0;
    double      lastScan   = -1e9;

    void build() {
        if (ready) return;
        gdVersion    = std::string(Loader::get()->getGameVersion());
        auto v       = Loader::get()->getVersion();
        geodeVersion = fmt::format("{}.{}.{}", v.getMajor(), v.getMinor(), v.getPatch());
        #if defined(GEODE_IS_WINDOWS)
            osBase = "Windows";
        #else
            osBase = "Android";
        #endif
        ready = true;
    }

    // Re-scanned at most every 5 s so late-loading mods are counted correctly.
    size_t getModCount(bool onlyActive) {
        double now = nowSeconds();
        if (now - lastScan > 5.0) {
            auto mods  = Loader::get()->getAllMods();
            totalMods  = mods.size();
            loadedMods = 0;
            for (auto* m : mods)
                if (m->isLoaded()) loadedMods++;
            lastScan = now;
        }
        return onlyActive ? loadedMods : totalMods;
    }
};
inline StaticCache g_cache;

// ── CPS ───────────────────────────────────────────────────────────────────────
inline std::deque<double> g_clickTimestamps;

inline void recordClick() { g_clickTimestamps.push_back(nowSeconds()); }

inline int getLiveCPS() {
    double now = nowSeconds();
    while (!g_clickTimestamps.empty() && now - g_clickTimestamps.front() > 1.0)
        g_clickTimestamps.pop_front();
    return static_cast<int>(g_clickTimestamps.size());
}

// ── Physics ticks (for real TPS) ──────────────────────────────────────────────
inline uint64_t g_tickCounter = 0;

// ── Accuracy ──────────────────────────────────────────────────────────────────
// A frame counts as "dead" if the player is dead OR a death-collision happened
// that frame (this is what makes accuracy meaningful with noclip).
inline int  g_framesAlive        = 0;
inline int  g_framesTotal        = 0;
inline bool g_collisionThisFrame = false;
inline bool g_noclipSeen         = false;   // a collision happened and the player survived

inline float getAccuracy() {
    if (g_framesTotal == 0) return 100.f;
    return (static_cast<float>(g_framesAlive) / static_cast<float>(g_framesTotal)) * 100.f;
}
inline void resetAccuracy() {
    g_framesAlive = g_framesTotal = 0;
    g_collisionThisFrame = false;
    g_noclipSeen = false;
}

// ── Deaths (whole game session) ───────────────────────────────────────────────
inline int g_sessionDeaths = 0;

// ── Session persistence ───────────────────────────────────────────────────────
// g_sessionStart never changes (the "session" timer no longer resets when the
// app loses focus). Total play time = value saved at launch + this session.
inline std::time_t g_sessionStart  = std::time(nullptr);
inline int64_t     g_playBase      = 0;
inline bool        g_playBaseReady = false;

inline void initSession() {
    g_playBase      = Mod::get()->getSavedValue<int64_t>("total-play-seconds", 0);
    g_playBaseReady = true;
}

inline int64_t getTotalPlaySeconds() {
    if (!g_playBaseReady) initSession();
    return g_playBase + static_cast<int64_t>(std::time(nullptr) - g_sessionStart);
}

inline void flushPlayTime() {
    Mod::get()->setSavedValue<int64_t>("total-play-seconds", getTotalPlaySeconds());
}

} // namespace uaci
