#include <Geode/modify/CCDirector.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>
#include <Geode/modify/LevelSelectLayer.hpp>
#include <Geode/modify/GJGarageLayer.hpp>
#include <Geode/modify/CreatorLayer.hpp>
#include "../../include/UACI.hpp"
#include "../../include/LabelConfig.hpp"
#include "../../include/InfoOverlay.hpp"
#include "../ui/SettingsLayer.hpp"

using namespace geode::prelude;
using namespace uaci;

static void setOverlayHidden(bool hidden) {
    if (auto* ov = getOverlay()) ov->setHidden(hidden);
}

// ── Overlay injection + screen-name bookkeeping ───────────────────────────────

class $modify(UACIDirector, CCDirector) {
    void willSwitchToScene(CCScene* scene) {
        CCDirector::willSwitchToScene(scene);

        // No known layer announced itself for this scene → don't keep a stale name.
        if (!g_screenFresh) g_currentScreen = "Menu";
        g_screenFresh = false;

        if (auto* om = OverlayManager::get()) {
            if (!om->getChildByID("uaci-overlay")) {
                if (auto* ov = InfoOverlay::create()) om->addChild(ov);
            }
        }
    }
};

// ── MenuLayer — HUD button next to the Daily Chest ────────────────────────────

class $modify(UACIMenu, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;
        setScreen("Main Menu");

        CCNode* btnSprite = nullptr;
        if (auto* spr = CCSprite::createWithSpriteFrameName("GJ_statsBtn_001.png")) {
            spr->setScale(0.80f);
            auto lbl = CCLabelBMFont::create("HUD", "chatFont.fnt");
            lbl->setScale(0.33f);
            lbl->setPosition(CCPoint(spr->getContentSize().width / 2.f, -9.f));
            spr->addChild(lbl);
            btnSprite = spr;
        } else {
            btnSprite = ButtonSprite::create("HUD");
        }

        auto btn = CCMenuItemSpriteExtra::create(
            btnSprite, this, menu_selector(UACIMenu::onHUD));
        btn->setID("uaci-hud-button");

        // The Daily Chest lives inside "right-side-menu" (not directly in MenuLayer),
        // so add our button to that same menu and let its layout place it next to the chest.
        if (auto* side = this->getChildByID("right-side-menu")) {
            side->addChild(btn);
            side->updateLayout();
        } else {
            auto menu = CCMenu::create();
            menu->setID("uaci-hud-menu");
            menu->addChild(btn);
            auto ws = CCDirector::sharedDirector()->getWinSize();
            menu->setPosition(CCPoint(ws.width - 40.f, 40.f));
            this->addChild(menu, 10);
        }
        return true;
    }

    void onHUD(CCObject*) {
        if (auto* p = SettingsLayer::create()) p->show();   // create() returns null if already open
    }
};

// ── GJBaseGameLayer — CPS + physics ticks ─────────────────────────────────────

class $modify(UACIBase, GJBaseGameLayer) {
    void handleButton(bool down, int button, bool isPlayer1) {
        // GD: button 1 = jump, 2 = left, 3 = right
        if (down && button == 1 && PlayLayer::get()) recordClick();
        GJBaseGameLayer::handleButton(down, button, isPlayer1);
    }

    // One call per physics step → real TPS (signature verified against 2.2081 bindings).
    // If it ever misreports (e.g. half-ticks counted twice), count only when !isHalfTick.
    void processCommands(float dt, bool isHalfTick, bool isLastTick) {
        g_tickCounter++;
        GJBaseGameLayer::processCommands(dt, isHalfTick, isLastTick);
    }
};

// ── PlayLayer — accuracy, deaths, hide logic ──────────────────────────────────

class $modify(UACIPlay, PlayLayer) {
    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;
        setScreen("Level");
        g_clickTimestamps.clear();
        resetAccuracy();
        setOverlayHidden(g_config.hideOnEnterLevel);   // also un-hides if the option is off
        return true;
    }

    // Accuracy: dead frames = player dead OR a death-collision happened (noclip-aware)
    void update(float dt) {
        PlayLayer::update(dt);
        if (m_player1) {
            g_framesTotal++;
            bool dead = m_player1->m_isDead || g_collisionThisFrame;
            if (!dead) g_framesAlive++;
            g_collisionThisFrame = false;
        }
    }

    void destroyPlayer(PlayerObject* player, GameObject* obj) {
        bool wasDead = m_player1 && m_player1->m_isDead;
        if (player == m_player1 || player == m_player2) g_collisionThisFrame = true;

        PlayLayer::destroyPlayer(player, obj);

        // Count a death only if this call actually killed the player
        // (no double counting from simultaneous hazards, nothing under noclip).
        if (m_player1 && !wasDead) {
            if (m_player1->m_isDead)        g_sessionDeaths++;
            else if (player == m_player1)   g_noclipSeen = true;   // collided but survived
        }
    }

    void onQuit() {
        setOverlayHidden(g_config.hideOnExitLevel);
        PlayLayer::onQuit();
    }
};

// ── Screen tracking ───────────────────────────────────────────────────────────

class $modify(UACIEditor, LevelEditorLayer) {
    bool init(GJGameLevel* l, bool a) {
        if (!LevelEditorLayer::init(l, a)) return false;
        setScreen("Editor"); return true;
    }
};
class $modify(UACIGarage, GJGarageLayer) {
    bool init() {
        if (!GJGarageLayer::init()) return false;
        setScreen("Icon Kit"); return true;
    }
};
class $modify(UACICreator, CreatorLayer) {
    bool init() {
        if (!CreatorLayer::init()) return false;
        setScreen("Creator Menu"); return true;
    }
};
class $modify(UACILvlSel, LevelSelectLayer) {
    bool init(int page) {
        if (!LevelSelectLayer::init(page)) return false;
        setScreen("Level Select"); return true;
    }
};
