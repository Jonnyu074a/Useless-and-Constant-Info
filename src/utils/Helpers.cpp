#include "Helpers.hpp"
#include <Geode/utils/general.hpp>
#include <fmt/format.h>
#include <ctime>
#include <chrono>
#include <cmath>
#include <cctype>
#include <vector>
#include <algorithm>

using namespace geode::prelude;

namespace uaci {

// ── Time ──────────────────────────────────────────────────────────────────────

std::string formatTime(bool is24h, bool showSeconds) {
    auto t  = std::time(nullptr);
    auto tm = geode::localtime(t);
    char buf[32];
    std::strftime(buf, sizeof(buf),
        is24h ? (showSeconds ? "%H:%M:%S" : "%H:%M")
              : (showSeconds ? "%I:%M:%S %p" : "%I:%M %p"),
        &tm);
    return buf;
}

std::string formatDate(int style) {
    auto t  = std::time(nullptr);
    auto tm = geode::localtime(t);
    char buf[64];
    switch (style) {
        case 1: std::strftime(buf, sizeof(buf), "%d/%m/%Y", &tm); return buf;
        case 2: {
            static const char* months[] = {
                "January","February","March","April","May","June",
                "July","August","September","October","November","December"
            };
            int mon = std::clamp(tm.tm_mon, 0, 11);
            return fmt::format("{} {} {}", tm.tm_mday, months[mon], 1900 + tm.tm_year);
        }
        default: std::strftime(buf, sizeof(buf), "%d/%m/%y", &tm); return buf;
    }
}

std::string formatSession(std::time_t start, bool showSeconds) {
    int64_t e = static_cast<int64_t>(std::time(nullptr) - start);
    if (e < 0) e = 0;
    int64_t h = e / 3600, m = (e % 3600) / 60, s = e % 60;
    if (h > 0) return showSeconds ? fmt::format("{}h {}m {}s", h, m, s) : fmt::format("{}h {}m", h, m);
    if (m > 0) return showSeconds ? fmt::format("{}m {}s", m, s)        : fmt::format("{}m", m);
    return fmt::format("{}s", s);
}

std::string formatTotalSession() {
    int64_t t = getTotalPlaySeconds();
    int64_t h = t / 3600, m = (t % 3600) / 60;
    return h > 0 ? fmt::format("{}h {}m", h, m) : fmt::format("{}m", m);
}

// ── Format renderer ───────────────────────────────────────────────────────────

static std::string trimSpaces(std::string s) {
    size_t a = 0, b = s.size();
    while (a < b && (s[a] == ' ' || s[a] == '\t')) a++;
    while (b > a && (s[b - 1] == ' ' || s[b - 1] == '\t')) b--;
    return s.substr(a, b - a);
}

static std::string renderSegment(std::string const& seg, TagMap const& vals) {
    std::string out;
    size_t i = 0;
    while (i < seg.size()) {
        if (seg[i] == '{') {
            size_t e = seg.find('}', i);
            if (e == std::string::npos) {          // unmatched '{' → keep literally
                out.append(seg, i, std::string::npos);
                break;
            }
            auto it = vals.find(seg.substr(i + 1, e - i - 1));
            if (it != vals.end()) out += it->second; // unknown tags are dropped
            i = e + 1;
        } else {
            out += seg[i++];
        }
    }
    return out;
}

std::string renderFormat(std::string const& fmtStr, std::string const& sep,
                         bool vertical, TagMap const& vals) {
    std::vector<std::string> segments;
    if (sep.empty()) {
        segments.push_back(fmtStr);
    } else {
        size_t p = 0;
        while (true) {
            size_t q = fmtStr.find(sep, p);
            if (q == std::string::npos) { segments.push_back(fmtStr.substr(p)); break; }
            segments.push_back(fmtStr.substr(p, q - p));
            p = q + sep.size();
        }
    }

    std::string out;
    for (auto& seg : segments) {
        std::string r = trimSpaces(renderSegment(seg, vals));
        if (r.empty()) continue;
        if (!out.empty()) out += vertical ? std::string("\n") : sep;
        out += r;
    }
    return out;
}

TagMap sampleTags() {
    return {
        {"fps", "60/240"}, {"tps", ""}, {"cps", "3 CPS"},
        {"gd", "GD: 2.2081"}, {"geode", "Geode: 5.0.0"},
        {"mods", "Mods: 76"}, {"os", "Android64"},
        {"user", "User: jonnyuun"}, {"mh", "MegaHack v9.0"},
        {"date", "01/03/26"}, {"time", "17:04:56"},
        {"session", "1h 20m"}, {"session_total", "10h 5m"},
        {"lvl", "Stereo Madness by RobTop"}, {"lvlid", "ID: 1"},
        {"att", "Att: 42"}, {"best", "Best: 87%"}, {"progress", "54%"},
        {"practice", "[Practice]"}, {"accuracy", "97.3%"}, {"deaths", "Deaths: 5"},
    };
}


// ── Screen detection ──────────────────────────────────────────────────────────

// "GJShopLayer" -> "GJ Shop", "LevelBrowserLayer" -> "Level Browser", "mh::ModMenu" -> "Mod Menu"
static std::string prettyClassName(std::string raw) {
    static const std::pair<const char*, const char*> known[] = {
        {"MenuLayer", "Main Menu"},         {"CreatorLayer", "Creator Menu"},
        {"GJGarageLayer", "Icon Kit"},      {"LevelSelectLayer", "Level Select"},
        {"LevelEditorLayer", "Editor"},     {"LevelBrowserLayer", "Level Browser"},
        {"LevelInfoLayer", "Level Info"},   {"EditLevelLayer", "Edit Level"},
        {"ModsLayer", "Geode Mods"},        {"LoadingLayer", "Loading"},
    };

    auto pos = raw.rfind("::");
    if (pos != std::string::npos) raw = raw.substr(pos + 2);
    for (auto& [k, v] : known) if (raw == k) return v;

    for (const char* suffix : {"Layer", "Popup", "Scene"}) {
        std::string sf = suffix;
        if (raw.size() > sf.size() && raw.compare(raw.size() - sf.size(), sf.size(), sf) == 0) {
            raw.erase(raw.size() - sf.size());
            break;
        }
    }
    if (raw.empty()) return "Menu";

    std::string out;
    for (size_t i = 0; i < raw.size(); i++) {
        char c = raw[i];
        if (i > 0 && std::isupper(static_cast<unsigned char>(c))) {
            char p = raw[i - 1];
            char n = i + 1 < raw.size() ? raw[i + 1] : 'A';
            if (std::islower(static_cast<unsigned char>(p)) ||
               (std::isupper(static_cast<unsigned char>(p)) && std::islower(static_cast<unsigned char>(n))))
                out += ' ';
        }
        out += c;
    }
    return out;
}

static CCNode* topAlert(CCNode* parent) {
    if (!parent || !parent->getChildren()) return nullptr;
    CCNode* top = nullptr;
    for (auto* child : CCArrayExt<CCNode*>(parent->getChildren())) {
        if (!child) continue;
        std::string id = std::string(child->getID());
        if (id.rfind("uaci-", 0) == 0) continue;            // ignore our own windows
        if (typeinfo_cast<FLAlertLayer*>(child)) top = child; // later children are drawn on top
    }
    return top;
}

std::string detectScreenName() {
    static std::string last = "Menu";

    auto* scene = CCDirector::sharedDirector()->getRunningScene();
    if (!scene) return last;

    // Mid-transition: keep the previous name instead of flickering.
    if (std::string(geode::cocos::getObjectName(scene)).rfind("CCTransition", 0) == 0)
        return last;

    // 1) Top-most popup: GD alerts, Geode popups and other mods' menus (MegaHack...).
    CCNode* node = topAlert(OverlayManager::get());
    if (!node) node = topAlert(scene);

    // 2) Otherwise the main layer of the scene.
    if (!node && scene->getChildren() && scene->getChildren()->count() > 0)
        node = static_cast<CCNode*>(scene->getChildren()->objectAtIndex(0));

    if (node) last = prettyClassName(std::string(geode::cocos::getObjectName(node)));
    return last;
}

// ── FPS ───────────────────────────────────────────────────────────────────────

int getTargetFPS() {
    double iv = CCDirector::sharedDirector()->getAnimationInterval();
    return iv > 0 ? static_cast<int>(std::round(1.0 / iv)) : 60;
}

// ── Mod menus (locally installed only: no network, no false positives) ───────

std::string getModMenuDisplay() {
    static std::string cached;
    static bool        found    = false;
    static double      lastScan = -1e9;

    if (found) return cached;
    double now = nowSeconds();
    if (now - lastScan < 5.0) return cached;   // retry every 5 s until one shows up
    lastScan = now;

    for (auto& id : KNOWN_MOD_MENUS) {
        auto* mod = Loader::get()->getLoadedMod(id);
        if (!mod) continue;
        auto v = mod->getVersion();
        cached = fmt::format("{} v{}.{}.{}", std::string(mod->getName()),
                             v.getMajor(), v.getMinor(), v.getPatch());
        found = true;
        break;
    }
    return cached;
}

} // namespace uaci
