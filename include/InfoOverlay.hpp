#pragma once
#include <Geode/Geode.hpp>
#include <fmt/format.h>
#include <algorithm>
#include <cmath>
#include "LabelConfig.hpp"
#include "UACI.hpp"
#include "../src/utils/Helpers.hpp"

using namespace geode::prelude;

namespace uaci {

class InfoOverlay : public CCNode {
private:
    CCLabelBMFont*  m_label       = nullptr;
    CCScale9Sprite* m_bg          = nullptr;
    bool            m_hidden      = false;
    CCSize          m_lastWinSize;
    CCPoint         m_anchor      = CCPoint(1.f, 0.f);

    // FPS/TPS measured in REAL time (not scaled dt) over each update window.
    float    m_smoothFPS   = 60.f;
    float    m_smoothTPS   = 60.f;
    int      m_frames      = 0;
    double   m_windowStart = 0.0;
    uint64_t m_lastTicks   = 0;
    double   m_lastFlush   = 0.0;

    std::string m_lastText;

public:
    static InfoOverlay* create() {
        auto p = new InfoOverlay();
        if (p && p->initOverlay()) { p->autorelease(); return p; }
        delete p; return nullptr;
    }

    bool initOverlay() {
        if (!CCNode::init()) return false;
        this->setID("uaci-overlay");
        this->setZOrder(999999);

        m_bg = CCScale9Sprite::create("square02_001.png");
        m_bg->setID("uaci-bg");
        this->addChild(m_bg, 1);

        m_label = CCLabelBMFont::create("", g_config.fontType.c_str());
        m_label->setID("uaci-label");
        this->addChild(m_label, 2);

        m_lastWinSize = CCDirector::sharedDirector()->getWinSize();
        m_windowStart = nowSeconds();
        m_lastFlush   = m_windowStart;
        m_lastTicks   = g_tickCounter;

        applyVisuals();
        buildContent();
        this->scheduleUpdate();
        return true;
    }

    // ── applyVisuals: only recreates the label node when the font changes ────
    void applyVisuals() {
        if (!g_config.enabled || m_hidden) { this->setVisible(false); return; }
        this->setVisible(true);

        bool fontChanged = false;
        if (m_label) {
            const char* cur = m_label->getFntFile();
            if (!cur || std::string(cur) != g_config.fontType) {
                m_label->removeFromParent();
                m_label = CCLabelBMFont::create("", g_config.fontType.c_str());
                m_label->setID("uaci-label");
                this->addChild(m_label, 2);
                fontChanged = true;
            }
        }

        m_label->setScale(g_config.textScale);
        m_label->setColor(g_config.textColor);
        m_label->setOpacity(static_cast<GLubyte>(std::clamp(g_config.textOpacity, 0, 255)));
        m_bg->setVisible(g_config.showBg);
        m_bg->setColor(g_config.bgColor);
        m_bg->setOpacity(static_cast<GLubyte>(std::clamp(g_config.bgOpacity, 0, 255)));

        updatePosition();
        if (fontChanged) { m_lastText.clear(); buildContent(); }
        else updateBg();
    }

    void setHidden(bool h) { m_hidden = h; applyVisuals(); }
    bool isHidden() const  { return m_hidden; }

    void update(float) override {
        double now = nowSeconds();

        // Persist play time every 60 s (covers desktop closes / crashes).
        if (now - m_lastFlush > 60.0) { flushPlayTime(); m_lastFlush = now; }

        if (!g_config.enabled || m_hidden) {
            m_frames = 0; m_windowStart = now; m_lastTicks = g_tickCounter;
            return;
        }

        // Dynamic resolution detection
        auto ws = CCDirector::sharedDirector()->getWinSize();
        if (ws.width != m_lastWinSize.width || ws.height != m_lastWinSize.height) {
            m_lastWinSize = ws;
            updatePosition();
        }

        m_frames++;
        double elapsed = now - m_windowStart;
        if (elapsed >= std::max(0.05f, g_config.updateRate)) {
            m_smoothFPS = static_cast<float>(m_frames / elapsed);
            uint64_t ticks = g_tickCounter - m_lastTicks;
            // Outside a level no physics runs → TPS falls back to FPS.
            m_smoothTPS = ticks > 0 ? static_cast<float>(ticks / elapsed) : m_smoothFPS;
            m_frames = 0; m_windowStart = now; m_lastTicks = g_tickCounter;
            buildContent();
        }
    }

    void buildContent() {
        auto& c  = g_config;
        auto& sc = g_cache;
        sc.build();

        TagMap v;

        // ── {fps} / {tps} ────────────────────────────────────────────────────
        {
            int  fps = static_cast<int>(std::round(m_smoothFPS));
            int  tps = static_cast<int>(std::round(m_smoothTPS));
            bool f = c.fps.showFPS, t = c.fps.showTPS, together = c.fps.showTogether;
            std::string fv, tv;
            if (f && t && together) fv = fmt::format("{}/{}", fps, tps);
            else if (f)             fv = std::to_string(fps);
            if (t && !(f && together)) tv = std::to_string(tps);
            if (f && c.fps.showTarget) fv += fmt::format("/{}", getTargetFPS());
            v["fps"] = fv;
            v["tps"] = tv;
        }

        // ── {cps} ────────────────────────────────────────────────────────────
        v["cps"] = c.cps.enabled ? fmt::format("{} CPS", getLiveCPS()) : std::string();

        // ── {accuracy} ───────────────────────────────────────────────────────
        {
            std::string acc;
            if (c.accuracy.enabled && (!c.accuracy.requireNoclip || g_noclipSeen))
                acc = fmt::format("{:.1f}%", getAccuracy());
            v["accuracy"] = acc;
        }

        // ── Static cached values ─────────────────────────────────────────────
        v["gd"]    = fmt::format("GD: {}", sc.gdVersion);
        v["geode"] = fmt::format("Geode: {}", sc.geodeVersion);
        v["mods"]  = fmt::format("Mods: {}", sc.getModCount(c.mods.onlyActive));

        // ── {os} ─────────────────────────────────────────────────────────────
        {
            std::string osStr;
            #if defined(GEODE_IS_ANDROID)
                if      (c.os.androidOverride == 1) osStr = "Android";
                else if (c.os.androidOverride == 2) osStr = "GooglePlayRC";
                else {
                    osStr = "Android";
                    if (c.os.showBits) osStr += (sizeof(void*) == 8) ? "64" : "32";
                }
            #else
                osStr = sc.osBase;
            #endif
            v["os"] = osStr;
        }

        // ── {user} ───────────────────────────────────────────────────────────
        {
            auto gm = GameManager::sharedState();
            std::string u = "Guest";
            int uid = valInt(gm->m_playerUserID);
            if (uid > 0)
                u = c.user.showName ? std::string(gm->m_playerName) : std::to_string(uid);
            v["user"] = fmt::format("User: {}", u);
        }

        // ── {mh} / {date} / {time} / {session*} ──────────────────────────────
        v["mh"]            = getModMenuDisplay();
        v["date"]          = formatDate(c.date.format);
        v["time"]          = formatTime(c.time.is24h, c.time.showSeconds);
        v["session"]       = formatSession(g_sessionStart, c.session.showSeconds);
        v["session_total"] = formatTotalSession();

        // ── Level tags ───────────────────────────────────────────────────────
        std::string lvl, lvlid, att, best, prog, practice;
        auto pl = PlayLayer::get();
        if (pl && pl->m_level) {
            lvl = std::string(pl->m_level->m_levelName);
            if (c.lvl.showAuthor) {
                std::string author = std::string(pl->m_level->m_creatorName);
                if (!author.empty()) lvl += " by " + author;
            }
            int id = valInt(pl->m_level->m_levelID);
            if (id > 0) lvlid = fmt::format("ID: {}", id);

            att  = fmt::format("Att: {}",  valInt(pl->m_level->m_attempts));
            best = fmt::format("Best: {}%", valInt(pl->m_level->m_normalPercent));

            float p = pl->m_hasCompletedLevel
                ? 100.f
                : std::clamp(pl->getCurrentPercent(), 0.f, 100.f);
            prog = fmt::format("{}%", static_cast<int>(p));
            if (pl->m_isPracticeMode) practice = "[Practice]";
        } else if (auto* ed = LevelEditorLayer::get()) {
            lvl = ed->m_level
                ? std::string(ed->m_level->m_levelName) + " [Editor]" : std::string("Editor");
        } else {
            lvl = g_currentScreen;
        }
        v["lvl"] = lvl;       v["lvlid"] = lvlid;   v["att"] = att;
        v["best"] = best;     v["progress"] = prog; v["practice"] = practice;

        // ── {deaths} ─────────────────────────────────────────────────────────
        {
            std::string d;
            if (c.deaths.showDeaths) {
                if (c.deaths.showSession)
                    d = fmt::format("Deaths: {}", g_sessionDeaths);
                else if (pl && pl->m_level)
                    d = fmt::format("Att: {}", valInt(pl->m_level->m_attempts));
            }
            v["deaths"] = d;
        }

        // ── Render (single pass: split → fill → drop empties → join) ─────────
        std::string out = renderFormat(c.formatStr, c.separator,
                                       c.layoutMode == "Vertical", v);

        // ── FPS color coding ─────────────────────────────────────────────────
        if (c.colorCodingFPS) {
            if (m_smoothFPS > 144)     m_label->setColor({0,255,0});
            else if (m_smoothFPS > 60) m_label->setColor({255,255,0});
            else                       m_label->setColor({255,0,0});
        } else {
            m_label->setColor(c.textColor);
        }

        // Only touch the label (expensive relayout) when the text really changed.
        if (out != m_lastText) {
            m_lastText = out;
            m_label->setString(out.c_str());
            updateBg();
        }
    }

    // ── Positioning ──────────────────────────────────────────────────────────
    void updatePosition() {
        auto  ws = CCDirector::sharedDirector()->getWinSize();
        auto& c  = g_config;
        float ox = static_cast<float>(c.offsetX), oy = static_cast<float>(c.offsetY);

        bool top    = c.posMode.rfind("Top", 0) == 0;
        bool left   = c.posMode.size() >= 4 && c.posMode.compare(c.posMode.size() - 4, 4, "Left") == 0;
        bool right  = c.posMode.size() >= 5 && c.posMode.compare(c.posMode.size() - 5, 5, "Right") == 0;

        m_anchor = CCPoint(left ? 0.f : (right ? 1.f : 0.5f), top ? 1.f : 0.f);
        CCPoint pos(
            left ? ox : (right ? ws.width - ox : ws.width / 2.f + ox),
            top  ? ws.height - oy : oy
        );

        this->setPosition(pos);
        m_label->setAnchorPoint(m_anchor);
        m_bg->setAnchorPoint(m_anchor);
        updateBg();
    }

    void updateBg() {
        if (!g_config.showBg) { m_bg->setVisible(false); return; }
        m_bg->setVisible(true);
        auto  ls  = m_label->getScaledContentSize();
        float pad = static_cast<float>(g_config.bgPadding);
        CCSize bgSz = {ls.width + pad * 2, ls.height + pad * 2};
        m_bg->setContentSize(bgSz);
        CCPoint la = m_label->getAnchorPoint();
        m_label->setPosition(CCPoint(0, 0));
        m_bg->setPosition(CCPoint(
            -ls.width  * la.x + bgSz.width  * la.x - pad,
            -ls.height * la.y + bgSz.height * la.y - pad
        ));
    }

    // ── Drag support ─────────────────────────────────────────────────────────
    // Screen-space rectangle currently covered by the overlay (with a touch margin).
    CCRect getWorldRect() {
        CCSize sz = m_label->getScaledContentSize();
        if (g_config.showBg) {
            float pad = static_cast<float>(g_config.bgPadding);
            sz.width += pad * 2; sz.height += pad * 2;
        }
        CCPoint local(this->getPositionX() - sz.width * m_anchor.x,
                      this->getPositionY() - sz.height * m_anchor.y);
        CCPoint w = this->getParent() ? this->getParent()->convertToWorldSpace(local) : local;
        const float margin = 12.f;
        return CCRect(w.x - margin, w.y - margin, sz.width + margin * 2, sz.height + margin * 2);
    }

    // Converts the CURRENT node position into offsets for the CURRENT posMode
    // (keeps the corner the user selected; no more jumping to Top-Left).
    void storeOffsetsFromPosition() {
        auto  ws = CCDirector::sharedDirector()->getWinSize();
        auto  p  = this->getPosition();
        auto& c  = g_config;

        bool top   = c.posMode.rfind("Top", 0) == 0;
        bool left  = c.posMode.size() >= 4 && c.posMode.compare(c.posMode.size() - 4, 4, "Left") == 0;
        bool right = c.posMode.size() >= 5 && c.posMode.compare(c.posMode.size() - 5, 5, "Right") == 0;

        float ox = left ? p.x : (right ? ws.width - p.x : p.x - ws.width / 2.f);
        float oy = top  ? ws.height - p.y : p.y;
        c.offsetX = std::clamp(static_cast<int>(std::lround(ox)), -2000, 2000);
        c.offsetY = std::clamp(static_cast<int>(std::lround(oy)), -2000, 2000);
    }
};

inline InfoOverlay* getOverlay() {
    if (auto* om = OverlayManager::get())
        return static_cast<InfoOverlay*>(om->getChildByID("uaci-overlay"));
    return nullptr;
}

inline void refreshOverlay() {
    if (auto* ov = getOverlay()) ov->applyVisuals();
}

} // namespace uaci
