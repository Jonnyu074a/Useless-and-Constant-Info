#pragma once
#include <Geode/Geode.hpp>
#include <string>
#include "UACI.hpp"

using namespace geode::prelude;

namespace uaci {

inline constexpr const char* POSITIONS[6] = {
    "Top-Left","Top-Right","Bottom-Left","Bottom-Right","Top-Center","Bottom-Center"
};
inline constexpr const char* FONTS[4] = {"chatFont.fnt","goldFont.fnt","bigFont.fnt","gjFont01.fnt"};

struct FPSConfig      { bool showFPS=true, showTPS=true, showTogether=true, showTarget=false; };
struct CPSConfig      { bool enabled=true; };
struct AccuracyConfig { bool enabled=true, requireNoclip=false; };
struct ModsConfig     { bool onlyActive=false; };
struct OSConfig       { bool showBits=false; int androidOverride=0; }; // 0=auto,1=Android,2=GooglePlayRC
struct UserConfig     { bool showName=true; };
struct DateConfig     { int  format=0; };  // 0=dd/mm/yy  1=dd/mm/yyyy  2=full
struct TimeConfig     { bool is24h=true, showSeconds=true; };
struct SessionConfig  { bool showSeconds=true; };
struct LvlConfig      { bool showAuthor=true; };
struct DeathsConfig   { bool showDeaths=true, showSession=true; };

struct OverlayConfig {
    bool        enabled          = true;
    std::string posMode          = "Bottom-Right";
    int         offsetX          = 5;
    int         offsetY          = 5;
    std::string fontType         = "chatFont.fnt";
    float       textScale        = 0.45f;
    ccColor3B   textColor        = {255,255,255};
    int         textOpacity      = 255;
    bool        showBg           = true;
    ccColor3B   bgColor          = {0,0,0};
    int         bgOpacity        = 130;
    int         bgPadding        = 8;
    float       updateRate       = 0.25f;
    bool        hideOnEnterLevel = false;
    bool        hideOnExitLevel  = false;
    bool        colorCodingFPS   = false;
    std::string layoutMode       = "Horizontal";
    std::string separator        = " | ";
    std::string formatStr        =
        "{fps} | {gd} | {geode} | {mods} | {os} | {user} | {mh} | "
        "{date} | {time} | {session} | {session_total} | {lvl} | "
        "{lvlid} | {att} | {best} | {progress} | {practice} | "
        "{cps} | {accuracy} | {deaths}";

    FPSConfig      fps;
    CPSConfig      cps;
    AccuracyConfig accuracy;
    ModsConfig     mods;
    OSConfig       os;
    UserConfig     user;
    DateConfig     date;
    TimeConfig     time;
    SessionConfig  session;
    LvlConfig      lvl;
    DeathsConfig   deaths;

    void load() {
        auto* m = Mod::get();
        #define GB(k)  m->getSettingValue<bool>(k)
        #define GS(k)  m->getSettingValue<std::string>(k)
        #define GI(k)  (int)m->getSettingValue<int64_t>(k)
        #define GF(k)  (float)m->getSettingValue<double>(k)
        #define GC(k)  m->getSettingValue<ccColor3B>(k)

        enabled           = GB("enabled");
        posMode           = GS("position-mode");
        offsetX           = GI("offset-x");
        offsetY           = GI("offset-y");
        fontType          = GS("font-type");
        textScale         = GF("text-scale");
        textColor         = GC("text-color");
        textOpacity       = GI("text-opacity");
        showBg            = GB("show-bg");
        bgColor           = GC("bg-color");
        bgOpacity         = GI("bg-opacity");
        bgPadding         = GI("bg-padding");
        updateRate        = GF("update-rate");
        hideOnEnterLevel  = GB("hide-on-enter-level");
        hideOnExitLevel   = GB("hide-on-exit-level");
        colorCodingFPS    = GB("color-coding");
        layoutMode        = GS("layout-mode");
        separator         = GS("separator");
        formatStr         = GS("format-string");

        fps.showFPS            = GB("fps-show-fps");
        fps.showTPS            = GB("fps-show-tps");
        fps.showTogether       = GB("fps-show-together");
        fps.showTarget         = GB("fps-show-target");
        cps.enabled            = GB("cps-enabled");
        accuracy.enabled       = GB("accuracy-enabled");
        accuracy.requireNoclip = GB("accuracy-require-noclip");
        mods.onlyActive        = GB("mods-only-active");
        os.showBits            = GB("os-show-bits");
        os.androidOverride     = GI("os-android-override");
        user.showName          = GB("user-show-name");
        date.format            = GI("date-format");
        time.is24h             = GB("time-24h");
        time.showSeconds       = GB("time-show-seconds");
        session.showSeconds    = GB("session-show-seconds");
        lvl.showAuthor         = GB("lvl-show-author");
        deaths.showDeaths      = GB("deaths-show");
        deaths.showSession     = GB("deaths-show-session");

        #undef GB
        #undef GS
        #undef GI
        #undef GF
        #undef GC
    }

    // Writes EVERY setting. The "any setting changed" listener is suppressed while
    // saving, so it can no longer overwrite values that have not been written yet.
    void save() const {
        struct Guard {
            Guard()  { g_suppressReload = true;  }
            ~Guard() { g_suppressReload = false; }
        } guard;

        auto* m = Mod::get();
        #define SB(k,v) (void)m->setSettingValue<bool>(k, (v))
        #define SS(k,v) (void)m->setSettingValue<std::string>(k, (v))
        #define SI(k,v) (void)m->setSettingValue<int64_t>(k, static_cast<int64_t>(v))
        #define SF(k,v) (void)m->setSettingValue<double>(k, static_cast<double>(v))
        #define SC(k,v) (void)m->setSettingValue<ccColor3B>(k, (v))

        SB("enabled",                 enabled);
        SS("position-mode",           posMode);
        SI("offset-x",                offsetX);
        SI("offset-y",                offsetY);
        SS("font-type",               fontType);
        SF("text-scale",              textScale);
        SC("text-color",              textColor);
        SI("text-opacity",            textOpacity);
        SB("show-bg",                 showBg);
        SC("bg-color",                bgColor);
        SI("bg-opacity",              bgOpacity);
        SI("bg-padding",              bgPadding);
        SF("update-rate",             updateRate);
        SB("hide-on-enter-level",     hideOnEnterLevel);
        SB("hide-on-exit-level",      hideOnExitLevel);
        SB("color-coding",            colorCodingFPS);
        SS("layout-mode",             layoutMode);
        SS("separator",               separator);
        SS("format-string",           formatStr);

        SB("fps-show-fps",            fps.showFPS);
        SB("fps-show-tps",            fps.showTPS);
        SB("fps-show-together",       fps.showTogether);
        SB("fps-show-target",         fps.showTarget);
        SB("cps-enabled",             cps.enabled);
        SB("accuracy-enabled",        accuracy.enabled);
        SB("accuracy-require-noclip", accuracy.requireNoclip);
        SB("mods-only-active",        mods.onlyActive);
        SB("os-show-bits",            os.showBits);
        SI("os-android-override",     os.androidOverride);
        SB("user-show-name",          user.showName);
        SI("date-format",             date.format);
        SB("time-24h",                time.is24h);
        SB("time-show-seconds",       time.showSeconds);
        SB("session-show-seconds",    session.showSeconds);
        SB("lvl-show-author",         lvl.showAuthor);
        SB("deaths-show",             deaths.showDeaths);
        SB("deaths-show-session",     deaths.showSession);

        #undef SB
        #undef SS
        #undef SI
        #undef SF
        #undef SC
    }
};

inline OverlayConfig g_config;

} // namespace uaci
