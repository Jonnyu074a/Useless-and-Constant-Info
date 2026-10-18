#include "SettingsLayer.hpp"
#include "../../include/LabelConfig.hpp"
#include "../../include/UACI.hpp"
#include "../../include/InfoOverlay.hpp"
#include <Geode/ui/ColorPickPopup.hpp>
#include <fmt/format.h>
#include <algorithm>
#include <cmath>

using namespace geode::prelude;
using namespace uaci;

// ═════════════════════════════════════════════════════════════════════════════
//  Persistence helpers
// ═════════════════════════════════════════════════════════════════════════════

// Save everything + refresh the overlay (the reload-listener is suppressed during save()).
static void commit() {
    g_config.save();
    refreshOverlay();
}

// Cheap single-key save used while a color picker is being dragged.
static void saveColor(const char* key, ccColor3B c) {
    g_suppressReload = true;
    (void)Mod::get()->setSettingValue<ccColor3B>(key, c);
    g_suppressReload = false;
    refreshOverlay();
}

// ═════════════════════════════════════════════════════════════════════════════
//  UI building blocks
// ═════════════════════════════════════════════════════════════════════════════

static constexpr float ROW_W = 340.f;

static CCScale9Sprite* rowBg(float w, float h) {
    auto bg = CCScale9Sprite::create("square02_001.png");
    bg->setContentSize(CCSize(w, h));
    bg->setAnchorPoint(CCPoint(0, 0));
    bg->setColor(ccColor3B{0, 0, 0});
    bg->setOpacity(90);
    return bg;
}

// Round "i" button. Title/description travel inside the node as user objects and
// are shown by SettingsLayer::onInfo in a popup.
static CCMenuItemSpriteExtra* infoButton(CCObject* tgt, SEL_MenuHandler sel,
                                         const char* title, const char* desc) {
    auto spr = CCSprite::createWithSpriteFrameName("GJ_infoIcon_001.png");
    if (!spr) return nullptr;
    spr->setScale(0.5f);
    auto btn = CCMenuItemSpriteExtra::create(spr, tgt, sel);
    btn->setUserObject("uaci-title", CCString::create(title));
    btn->setUserObject("uaci-desc",  CCString::create(desc));
    return btn;
}

// Gold title with an underline and an optional "i" button on the right.
static CCNode* sectionHeader(const char* text, CCObject* tgt = nullptr,
                             SEL_MenuHandler infoSel = nullptr,
                             const char* infoTitle = nullptr, const char* infoDesc = nullptr) {
    auto n = CCNode::create();
    n->setContentSize(CCSize(ROW_W, 24.f));

    auto l = CCLabelBMFont::create(text, "goldFont.fnt");
    l->setScale(0.5f);
    l->setAnchorPoint(CCPoint(0, .5f));
    l->setPosition(CCPoint(4, 13));
    n->addChild(l);

    auto line = CCLayerColor::create(ccColor4B{255, 255, 255, 45}, ROW_W, 1.f);
    line->setPosition(CCPoint(0, 1));
    n->addChild(line);

    if (tgt && infoSel && infoTitle && infoDesc) {
        if (auto b = infoButton(tgt, infoSel, infoTitle, infoDesc)) {
            auto m = CCMenu::create();
            m->setPosition(CCPoint(0, 0));
            b->setPosition(CCPoint(ROW_W - 14.f, 13));
            m->addChild(b);
            n->addChild(m);
        }
    }
    return n;
}

// Row panel: label on the left, optional info button, toggle on the right.
static CCNode* toggleRow(const char* label, bool val, CCObject* tgt, SEL_MenuHandler sel,
                         int tag, SEL_MenuHandler infoSel = nullptr, const char* desc = nullptr) {
    auto row = CCNode::create();
    row->setContentSize(CCSize(ROW_W, 30.f));
    row->addChild(rowBg(ROW_W, 30.f));

    auto l = CCLabelBMFont::create(label, "bigFont.fnt");
    l->setAnchorPoint(CCPoint(0, .5f));
    l->setPosition(CCPoint(12, 15));
    l->limitLabelWidth((infoSel && desc) ? 240.f : 270.f, 0.32f, 0.1f);
    row->addChild(l);

    auto m = CCMenu::create();
    m->setPosition(CCPoint(0, 0));

    auto tog = CCMenuItemToggler::createWithStandardSprites(tgt, sel, 0.6f);
    tog->toggle(val);
    tog->setTag(tag);
    tog->setPosition(CCPoint(ROW_W - 22.f, 15));
    m->addChild(tog);

    if (infoSel && desc) {
        if (auto b = infoButton(tgt, infoSel, label, desc)) {
            b->setPosition(CCPoint(ROW_W - 52.f, 15));
            m->addChild(b);
        }
    }
    row->addChild(m);
    return row;
}

// Row panel with a color swatch button (the swatch IS the button; no text overlap).
static CCNode* colorRow(const char* label, ccColor3B col, CCObject* tgt, SEL_MenuHandler sel,
                        CCScale9Sprite*& innerOut) {
    auto row = CCNode::create();
    row->setContentSize(CCSize(ROW_W, 32.f));
    row->addChild(rowBg(ROW_W, 32.f));

    auto l = CCLabelBMFont::create(label, "bigFont.fnt");
    l->setAnchorPoint(CCPoint(0, .5f));
    l->setPosition(CCPoint(12, 16));
    l->limitLabelWidth(150.f, 0.32f, 0.1f);
    row->addChild(l);

    auto hint = CCLabelBMFont::create("tap to edit", "chatFont.fnt");
    hint->setScale(0.4f);
    hint->setColor(ccColor3B{150, 150, 150});
    hint->setAnchorPoint(CCPoint(1, .5f));
    hint->setPosition(CCPoint(ROW_W - 76.f, 16));
    row->addChild(hint);

    auto outer = CCScale9Sprite::create("square02_001.png");
    outer->setContentSize(CCSize(56.f, 24.f));
    outer->setColor(ccColor3B{255, 255, 255});
    auto inner = CCScale9Sprite::create("square02_001.png");
    inner->setContentSize(CCSize(50.f, 18.f));
    inner->setAnchorPoint(CCPoint(.5f, .5f));
    inner->setPosition(CCPoint(28, 12));
    inner->setColor(col);
    inner->setOpacity(255);
    outer->addChild(inner);
    innerOut = inner;

    auto btn = CCMenuItemSpriteExtra::create(outer, tgt, sel);
    btn->setPosition(CCPoint(ROW_W - 34.f, 16));
    auto m = CCMenu::create();
    m->setPosition(CCPoint(0, 0));
    m->addChild(btn);
    row->addChild(m);
    return row;
}

static CCMenuItemSpriteExtra* textBtn(const char* lbl, CCObject* tgt, SEL_MenuHandler sel,
                                      int tag, bool active, float scale = 0.44f) {
    auto spr = ButtonSprite::create(lbl, "chatFont.fnt",
               active ? "GJ_button_02.png" : "GJ_button_04.png", scale);
    auto btn = CCMenuItemSpriteExtra::create(spr, tgt, sel);
    btn->setTag(tag);
    return btn;
}

// Tab button: rounded panel + icon + label. Active tab is highlighted in cyan.
static CCMenuItemSpriteExtra* tabButton(const char* iconFrame, const char* label,
                                        CCObject* tgt, SEL_MenuHandler sel, int tag, bool active) {
    auto bg = CCScale9Sprite::create("square02_001.png");
    bg->setContentSize(CCSize(88.f, 26.f));
    bg->setColor(active ? ccColor3B{20, 150, 200} : ccColor3B{20, 22, 45});
    bg->setOpacity(active ? 255 : 210);

    float textX = 44.f;
    if (auto icon = CCSprite::createWithSpriteFrameName(iconFrame)) {
        icon->setScale(0.27f);
        icon->setPosition(CCPoint(15, 13));
        bg->addChild(icon);
        textX = 54.f;
    }
    auto l = CCLabelBMFont::create(label, "bigFont.fnt");
    l->limitLabelWidth(54.f, 0.3f, 0.1f);
    l->setPosition(CCPoint(textX, 13));
    bg->addChild(l);

    auto btn = CCMenuItemSpriteExtra::create(bg, tgt, sel);
    btn->setTag(tag);
    return btn;
}

// CCMenu with an EXPLICIT size (CCMenu::create() defaults to the whole window size,
// which would make every row/column layout enormous).
static CCMenu* rowMenu(float w, float h, float gap, bool wrap = false) {
    auto m = CCMenu::create();
    m->setContentSize(CCSize(w, h));
    auto layout = RowLayout::create();
    layout->setGap(gap);
    layout->setAxisAlignment(AxisAlignment::Center);
    layout->setAutoScale(false);
    layout->setGrowCrossAxis(wrap);
    if (wrap) layout->setCrossAxisReverse(true);   // first row on top
    m->setLayout(layout);
    return m;
}

// Wraps a menu inside a rounded panel.
static CCNode* panelWith(CCMenu* menu, float h) {
    auto node = CCNode::create();
    node->setContentSize(CCSize(ROW_W, h));
    node->addChild(rowBg(ROW_W, h));
    menu->ignoreAnchorPointForPosition(false);
    menu->setAnchorPoint(CCPoint(.5f, .5f));
    menu->setPosition(CCPoint(ROW_W / 2.f, h / 2.f));
    node->addChild(menu);
    return node;
}

// ═════════════════════════════════════════════════════════════════════════════
//  Drag-to-reposition layer
//  Lives in OverlayManager (above everything), swallows all touches, has its own
//  "Done" button (no CCMenu → no touch-priority conflicts).
// ═════════════════════════════════════════════════════════════════════════════

namespace {

class DragLayer : public CCLayer {
    bool     m_dragging  = false;
    bool     m_pressDone = false;
    CCPoint  m_startTouch;
    CCPoint  m_startPos;
    CCNode*  m_doneBtn   = nullptr;

    CCRect doneRect() const {
        auto r = m_doneBtn->boundingBox();
        return CCRect(r.origin.x - 8, r.origin.y - 8, r.size.width + 16, r.size.height + 16);
    }

    void finish() {
        Ref<DragLayer> self(this);   // keep alive while we remove ourselves
        this->removeFromParent();
        if (auto* p = SettingsLayer::create()) p->show();
    }

public:
    static void open() {
        auto* om = OverlayManager::get();
        if (!om) return;
        if (auto* ov = getOverlay()) ov->setHidden(false);   // must be visible to drag it
        if (auto* l = DragLayer::create()) om->addChild(l, 999990);
    }

    static DragLayer* create() {
        auto r = new DragLayer();
        if (r->init()) { r->autorelease(); return r; }
        delete r; return nullptr;
    }

    bool init() override {
        if (!CCLayer::init()) return false;
        this->setID("uaci-drag");
        auto ws = CCDirector::sharedDirector()->getWinSize();

        auto dim = CCLayerColor::create(ccColor4B{0, 0, 0, 90});
        this->addChild(dim, 0);

        auto hint = CCLabelBMFont::create("Drag the overlay anywhere", "bigFont.fnt");
        hint->setScale(0.45f);
        hint->setPosition(CCPoint(ws.width / 2.f, ws.height / 2.f + 24.f));
        this->addChild(hint, 1);

        auto done = ButtonSprite::create("Done", "goldFont.fnt", "GJ_button_01.png", 0.8f);
        done->setPosition(CCPoint(ws.width / 2.f, ws.height / 2.f - 14.f));
        this->addChild(done, 1);
        m_doneBtn = done;

        this->setTouchEnabled(true);
        this->setKeypadEnabled(true);
        return true;
    }

    void registerWithTouchDispatcher() override {
        CCDirector::sharedDirector()->getTouchDispatcher()->addTargetedDelegate(this, -600, true);
    }

    void keyBackClicked() override { finish(); }

    bool ccTouchBegan(CCTouch* t, CCEvent*) override {
        auto loc = t->getLocation();
        if (doneRect().containsPoint(loc)) { m_pressDone = true; return true; }

        if (auto* ov = getOverlay()) {
            if (ov->getWorldRect().containsPoint(loc)) {
                m_dragging   = true;
                m_startTouch = loc;
                m_startPos   = ov->getPosition();
            }
        }
        return true;   // swallow everything while in drag mode
    }

    void ccTouchMoved(CCTouch* t, CCEvent*) override {
        if (!m_dragging) return;
        if (auto* ov = getOverlay())
            ov->setPosition(m_startPos + (t->getLocation() - m_startTouch));
    }

    void ccTouchEnded(CCTouch* t, CCEvent*) override {
        if (m_pressDone) {
            m_pressDone = false;
            if (doneRect().containsPoint(t->getLocation())) finish();
            return;
        }
        if (m_dragging) {
            m_dragging = false;
            if (auto* ov = getOverlay()) {
                ov->storeOffsetsFromPosition();   // keeps the selected corner
                commit();                          // persists offset-x / offset-y too
            }
        }
    }

    void ccTouchCancelled(CCTouch*, CCEvent*) override {
        m_dragging = m_pressDone = false;
    }
};

} // namespace

// ═════════════════════════════════════════════════════════════════════════════
//  SettingsLayer
// ═════════════════════════════════════════════════════════════════════════════

static SettingsLayer* s_instance = nullptr;
static constexpr float VIEW_W = 360.f;
static constexpr float VIEW_H = 205.f;

SettingsLayer* SettingsLayer::create() {
    if (s_instance) return nullptr;
    auto p = new SettingsLayer();
    if (p->init()) { p->autorelease(); s_instance = p; return p; }
    delete p;
    return nullptr;
}

SettingsLayer::~SettingsLayer() {
    if (s_instance == this) s_instance = nullptr;
}

bool SettingsLayer::init() {
    if (!Popup::init(400.f, 290.f)) return false;
    this->setID("uaci-settings");
    this->setTitle("Useless And Constant info", "goldFont.fnt", 0.55f, 18.f);

    // Tabs
    m_tabMenu = CCMenu::create();
    m_tabMenu->setContentSize(CCSize(380.f, 28.f));
    m_tabMenu->ignoreAnchorPointForPosition(false);
    m_tabMenu->setAnchorPoint(CCPoint(.5f, .5f));
    auto tl = RowLayout::create();
    tl->setGap(5.f);
    tl->setAutoScale(false);
    m_tabMenu->setLayout(tl);
    m_tabMenu->setPosition(CCPoint(m_size.width / 2.f, m_size.height - 48.f));
    m_mainLayer->addChild(m_tabMenu, 2);

    // Scroll area, sits below the tabs inside the 400 x 290 panel
    m_scroll = ScrollLayer::create(CCSize(VIEW_W, VIEW_H));
    m_scroll->setPosition(CCPoint((m_size.width - VIEW_W) / 2.f, 16.f));
    m_mainLayer->addChild(m_scroll, 2);

    loadTab(0);
    return true;
}

// ── Tabs ──────────────────────────────────────────────────────────────────────

void SettingsLayer::buildTabs() {
    struct TabDef { const char* icon; const char* name; };
    static const TabDef tabs[] = {
        {"GJ_optionsBtn_001.png", "General"},
        {"GJ_statsBtn_001.png",   "Labels"},
        {"GJ_garageBtn_001.png",  "Visual"},
        {"GJ_createBtn_001.png",  "Builder"},
    };
    m_tabMenu->removeAllChildren();
    for (int i = 0; i < 4; i++)
        m_tabMenu->addChild(tabButton(tabs[i].icon, tabs[i].name, this,
                                      menu_selector(SettingsLayer::onTab), i, i == m_tab));
    m_tabMenu->updateLayout();
}

void SettingsLayer::loadTab(int t, bool keepScroll) {
    // Remember how far from the top we were scrolled (so choosing an option doesn't
    // throw the user back to the top).
    float offsetFromTop = 0.f;
    if (keepScroll) {
        float oldH = m_scroll->m_contentLayer->getContentSize().height;
        offsetFromTop = (VIEW_H - oldH) - m_scroll->m_contentLayer->getPositionY();
    }

    m_tab        = t;
    m_fmtInput   = nullptr;
    m_preview    = nullptr;
    m_textSwatch = nullptr;
    m_bgSwatch   = nullptr;

    buildTabs();
    m_scroll->m_contentLayer->removeAllChildren();

    auto content = CCNode::create();
    content->setAnchorPoint(CCPoint(0, 0));
    content->setContentSize(CCSize(VIEW_W, 10.f));
    switch (t) {
        case 0: buildGeneral(content); break;
        case 1: buildLabels(content);  break;
        case 2: buildVisual(content);  break;
        case 3: buildBuilder(content); break;
    }

    auto col = ColumnLayout::create();
    col->setGap(5.f);
    col->setAxisReverse(true);
    col->setAxisAlignment(AxisAlignment::End);
    col->setCrossAxisAlignment(AxisAlignment::Center);
    col->setAutoGrowAxis(VIEW_H);
    col->setAutoScale(false);
    content->setLayout(col);
    content->updateLayout();

    m_scroll->m_contentLayer->addChild(content);
    float h = std::max(content->getContentSize().height, VIEW_H);
    m_scroll->m_contentLayer->setContentSize(CCSize(VIEW_W, h));
    content->setPosition(CCPoint(0, h - content->getContentSize().height));

    if (keepScroll) {
        float lo = std::min(VIEW_H - h, 0.f);
        float y  = std::clamp((VIEW_H - h) - offsetFromTop, lo, 0.f);
        m_scroll->m_contentLayer->setPositionY(y);
    } else {
        m_scroll->moveToTop();
    }
}

// Rebuilding a tab destroys the buttons of the old one — including the button whose
// callback is running. Defer to the next frame to avoid use-after-free.
void SettingsLayer::reloadTabSoon(int t, bool keepScroll) {
    Loader::get()->queueInMainThread([self = Ref<SettingsLayer>(this), t, keepScroll]() {
        self->loadTab(t, keepScroll);
    });
}

void SettingsLayer::onTab(CCObject* s) {
    int t = static_cast<CCNode*>(s)->getTag();
    if (t == m_tab) return;
    reloadTabSoon(t, false);
}

// Info popup for any "i" button.
void SettingsLayer::onInfo(CCObject* s) {
    auto* node = static_cast<CCNode*>(s);
    auto* t = static_cast<CCString*>(node->getUserObject("uaci-title"));
    auto* d = static_cast<CCString*>(node->getUserObject("uaci-desc"));
    if (!t || !d) return;
    std::string title = t->getCString();
    std::string desc  = d->getCString();
    if (desc.size() > 240)
        FLAlertLayer::create(nullptr, title.c_str(), desc, "OK", nullptr, 380.f, true, 230.f, 1.f)->show();
    else
        FLAlertLayer::create(title.c_str(), desc, "OK")->show();
}

// ── TAB: GENERAL ─────────────────────────────────────────────────────────────

void SettingsLayer::buildGeneral(CCNode* c) {
    auto& cfg  = g_config;
    auto  tgl  = menu_selector(SettingsLayer::onToggle);
    auto  info = menu_selector(SettingsLayer::onInfo);

    c->addChild(sectionHeader("Overlay"));
    c->addChild(toggleRow("Enable overlay", cfg.enabled, this, tgl, 0, info,
        "Master switch. Turns the whole HUD on or off."));
    c->addChild(toggleRow("Hide when entering a level", cfg.hideOnEnterLevel, this, tgl, 1, info,
        "Hides the overlay as soon as you start playing a level. It comes back when you leave."));
    c->addChild(toggleRow("Hide when leaving a level", cfg.hideOnExitLevel, this, tgl, 2, info,
        "Hides the overlay when you quit a level (menus, editor...) until you enter a level again."));
    c->addChild(toggleRow("FPS color coding", cfg.colorCodingFPS, this, tgl, 3, info,
        "Colors the text by performance: green above 144 FPS, yellow above 60 and red below. Overrides the text color."));
    c->addChild(toggleRow("Vertical layout", cfg.layoutMode == "Vertical", this, tgl, 4, info,
        "Stacks every label on its own line instead of one long line."));

    c->addChild(sectionHeader("Position", this, info, "Position",
        "Pick the corner or edge the overlay is anchored to. Use 'Drag to reposition' to fine tune it; the offset is saved."));
    auto posMenu = rowMenu(320.f, 56.f, 4.f, true);
    for (int i = 0; i < 6; i++)
        posMenu->addChild(textBtn(POSITIONS[i], this, menu_selector(SettingsLayer::onPosition),
                                  i, cfg.posMode == POSITIONS[i], 0.36f));
    posMenu->updateLayout();
    c->addChild(panelWith(posMenu, 64.f));

    c->addChild(sectionHeader("Fine tuning", this, info, "Drag to reposition",
        "Closes this window so you can drag the overlay anywhere on screen. Tap 'Done' to save the new position."));
    auto dragMenu = rowMenu(320.f, 28.f, 5.f);
    dragMenu->addChild(textBtn("Drag to reposition", this,
                               menu_selector(SettingsLayer::onStartDrag), 0, false, 0.46f));
    dragMenu->updateLayout();
    c->addChild(panelWith(dragMenu, 36.f));

    auto ver = Mod::get()->getVersion();
    std::string verText = fmt::format("v{}.{}.{}", ver.getMajor(), ver.getMinor(), ver.getPatch());
    c->addChild(sectionHeader("About", this, info, "Useless And Constant info",
        fmt::format("Version {}\nA customizable real-time HUD: FPS/TPS, CPS, accuracy, level info, "
                    "time, mods and more.\n\nUse the Builder tab to choose exactly what is shown.", verText).c_str()));
}

// ── TAB: LABELS ──────────────────────────────────────────────────────────────

void SettingsLayer::buildLabels(CCNode* c) {
    auto& cfg  = g_config;
    auto  sel  = menu_selector(SettingsLayer::onLabelToggle);
    auto  info = menu_selector(SettingsLayer::onInfo);

    c->addChild(sectionHeader("FPS / TPS"));
    c->addChild(toggleRow("Show FPS", cfg.fps.showFPS, this, sel, 100, info,
        "Frames per second, measured in real time."));
    c->addChild(toggleRow("Show TPS", cfg.fps.showTPS, this, sel, 101, info,
        "Physics ticks per second. Measured inside levels; outside levels it matches FPS."));
    c->addChild(toggleRow("Show together (60/240)", cfg.fps.showTogether, this, sel, 102, info,
        "Shows FPS and TPS in a single label, like 60/240."));
    c->addChild(toggleRow("Show target Hz", cfg.fps.showTarget, this, sel, 103, info,
        "Appends your target refresh rate to the FPS label, like 60/240/60."));

    c->addChild(sectionHeader("CPS"));
    c->addChild(toggleRow("Show CPS", cfg.cps.enabled, this, sel, 110, info,
        "Clicks per second: jump presses counted over the last second while playing."));

    c->addChild(sectionHeader("Accuracy"));
    c->addChild(toggleRow("Show accuracy", cfg.accuracy.enabled, this, sel, 120, info,
        "Percentage of frames in the current level where you were not dead or touching a hazard."));
    c->addChild(toggleRow("Only after a noclip hit", cfg.accuracy.requireNoclip, this, sel, 121, info,
        "Hides accuracy until a hazard collision happens without dying (noclip). Keeps it out of normal play."));

    c->addChild(sectionHeader("Deaths"));
    c->addChild(toggleRow("Show deaths", cfg.deaths.showDeaths, this, sel, 130, info,
        "Shows the death counter."));
    c->addChild(toggleRow("Session deaths", cfg.deaths.showSession, this, sel, 131, info,
        "On: deaths since you opened the game. Off: total attempts of the current level."));

    c->addChild(sectionHeader("Mods"));
    c->addChild(toggleRow("Only count active mods", cfg.mods.onlyActive, this, sel, 200, info,
        "Counts only loaded mods instead of every installed mod."));

    c->addChild(sectionHeader("OS (Android)", this, info, "OS label",
        "Auto shows 'Android' (with 32/64 bits if enabled in Geode's settings). "
        "GooglePlayRC is for the Play Store build."));
    auto aMenu = rowMenu(320.f, 28.f, 5.f);
    const char* aOpts[] = {"Auto", "Android", "GooglePlayRC"};
    for (int i = 0; i < 3; i++)
        aMenu->addChild(textBtn(aOpts[i], this, menu_selector(SettingsLayer::onAndroidOverride),
                                i, cfg.os.androidOverride == i, 0.42f));
    aMenu->updateLayout();
    c->addChild(panelWith(aMenu, 36.f));

    c->addChild(sectionHeader("User"));
    c->addChild(toggleRow("Show name", cfg.user.showName, this, sel, 400, info,
        "On: your username. Off: your account ID."));

    c->addChild(sectionHeader("Date format"));
    auto dMenu = rowMenu(320.f, 28.f, 5.f);
    const char* dfmts[] = {"dd/mm/yy", "dd/mm/yyyy", "Full text"};
    for (int i = 0; i < 3; i++)
        dMenu->addChild(textBtn(dfmts[i], this, menu_selector(SettingsLayer::onDateFormat),
                                i, cfg.date.format == i, 0.42f));
    dMenu->updateLayout();
    c->addChild(panelWith(dMenu, 36.f));

    c->addChild(sectionHeader("Time"));
    c->addChild(toggleRow("24-hour clock", cfg.time.is24h, this, sel, 500, info,
        "Off shows the time in 12-hour format with AM/PM."));
    c->addChild(toggleRow("Show seconds", cfg.time.showSeconds, this, sel, 501, info,
        "Adds seconds to the clock."));

    c->addChild(sectionHeader("Session"));
    c->addChild(toggleRow("Show seconds", cfg.session.showSeconds, this, sel, 600, info,
        "Adds seconds to the session timer (time since you opened the game)."));

    c->addChild(sectionHeader("Level"));
    c->addChild(toggleRow("Show 'by Author'", cfg.lvl.showAuthor, this, sel, 700, info,
        "Appends the level creator's name to the level name."));
}

// ── TAB: VISUAL ───────────────────────────────────────────────────────────────

void SettingsLayer::buildVisual(CCNode* c) {
    auto& cfg  = g_config;
    auto  info = menu_selector(SettingsLayer::onInfo);

    c->addChild(sectionHeader("Font", this, info, "Font", "Tap a font to apply it instantly."));
    const char* fLabels[] = {"Chat", "Gold", "Big", "GJ01"};
    auto fontMenu = rowMenu(320.f, 40.f, 6.f);
    for (int i = 0; i < 4; i++) {
        auto bg = CCScale9Sprite::create("square02_001.png");
        bg->setContentSize(CCSize(72.f, 34.f));
        bg->setColor(cfg.fontType == FONTS[i] ? ccColor3B{30, 110, 40} : ccColor3B{25, 25, 60});
        bg->setOpacity(230);
        auto prev = CCLabelBMFont::create(fLabels[i], FONTS[i]);
        prev->setScale(0.42f);
        prev->setPosition(CCPoint(36, 17));
        bg->addChild(prev);
        auto btn = CCMenuItemSpriteExtra::create(bg, this, menu_selector(SettingsLayer::onFont));
        btn->setTag(i);
        fontMenu->addChild(btn);
    }
    fontMenu->updateLayout();
    c->addChild(panelWith(fontMenu, 48.f));

    c->addChild(sectionHeader("Scale"));
    auto scaleMenu = rowMenu(320.f, 28.f, 4.f);
    const float sv[] = {0.3f, 0.4f, 0.45f, 0.5f, 0.6f, 0.7f, 0.8f};
    const char* sl[] = {"0.3", "0.4", "0.45", "0.5", "0.6", "0.7", "0.8"};
    for (int i = 0; i < 7; i++)
        scaleMenu->addChild(textBtn(sl[i], this, menu_selector(SettingsLayer::onScale),
                                    i, std::abs(cfg.textScale - sv[i]) < 0.01f, 0.40f));
    scaleMenu->updateLayout();
    c->addChild(panelWith(scaleMenu, 36.f));

    c->addChild(sectionHeader("Colors", this, info, "Colors",
        "Tap a swatch to open the color picker. Changes apply live. "
        "Text color is ignored while FPS color coding is on."));
    c->addChild(colorRow("Text color", cfg.textColor, this,
                         menu_selector(SettingsLayer::onPickTextColor), m_textSwatch));
    c->addChild(colorRow("Background color", cfg.bgColor, this,
                         menu_selector(SettingsLayer::onPickBgColor), m_bgSwatch));

    c->addChild(toggleRow("Show background", cfg.showBg, this,
                          menu_selector(SettingsLayer::onToggle), 10, info,
                          "Draws a rounded translucent box behind the text for readability."));
}

// ── TAB: BUILDER ─────────────────────────────────────────────────────────────

void SettingsLayer::buildBuilder(CCNode* c) {
    auto& cfg  = g_config;
    auto  info = menu_selector(SettingsLayer::onInfo);

    if (!m_draftInit) {
        m_draft     = cfg.formatStr;
        m_sep       = cfg.separator.empty() ? std::string(" | ") : cfg.separator;
        m_draftInit = true;
    }

    c->addChild(sectionHeader("Label builder", this, info, "Tag reference",
        "{fps} FPS (+TPS if shown together)\n{tps} TPS\n{cps} Clicks per second\n"
        "{gd} GD version\n{geode} Geode version\n{mods} Mod count\n{os} Operating system\n"
        "{user} Username / ID\n{mh} Mod menu (MegaHack, ...)\n{date} Date\n{time} Clock\n"
        "{session} Time since opening the game\n{session_total} Total play time\n"
        "{lvl} Level or screen name\n{lvlid} Level ID\n{att} Attempts\n{best} Best %\n"
        "{progress} Current %\n{practice} Practice mode tag\n{accuracy} Accuracy\n{deaths} Deaths\n\n"
        "The separator splits the format into parts; any part whose tags are all empty is hidden."));

    m_fmtInput = TextInput::create(380.f, "Format string...");
    m_fmtInput->setScale(0.85f);
    m_fmtInput->setMaxCharCount(300);
    m_fmtInput->setString(m_draft, false);
    m_fmtInput->setCallback([this](std::string const& s) {
        m_draft = s;
        refreshPreview();
    });
    c->addChild(m_fmtInput);

    m_preview = CCLabelBMFont::create("Preview: ...", "chatFont.fnt");
    m_preview->setScale(0.35f);
    m_preview->setColor(ccColor3B{180, 220, 255});
    m_preview->setID("uaci-preview");
    c->addChild(m_preview);

    // Tag buttons (5 per row → 5 rows)
    const std::vector<std::pair<std::string, std::string>> tags = {
        {"{fps}","FPS"},{"{tps}","TPS"},{"{cps}","CPS"},{"{gd}","GD"},
        {"{geode}","Geode"},{"{mods}","Mods"},{"{os}","OS"},{"{user}","User"},
        {"{mh}","Menu"},{"{date}","Date"},{"{time}","Time"},{"{session}","Session"},
        {"{session_total}","Total"},{"{lvl}","Level"},{"{lvlid}","ID"},
        {"{att}","Att"},{"{best}","Best"},{"{progress}","Prog"},
        {"{practice}","Pract"},{"{accuracy}","Acc"},{"{deaths}","Deaths"},
    };
    m_tagVals.clear();
    auto tagMenu = rowMenu(320.f, 136.f, 4.f, true);
    for (int i = 0; i < static_cast<int>(tags.size()); i++) {
        m_tagVals.push_back(tags[i].first);
        auto bg = CCScale9Sprite::create("square02_001.png");
        bg->setContentSize(CCSize(58.f, 24.f));
        bg->setColor(ccColor3B{25, 25, 65});
        bg->setOpacity(230);
        auto nl = CCLabelBMFont::create(tags[i].second.c_str(), "chatFont.fnt");
        nl->setScale(0.30f); nl->setPosition(CCPoint(29, 15)); nl->setColor(ccColor3B{200, 220, 255});
        bg->addChild(nl);
        auto cl = CCLabelBMFont::create(tags[i].first.c_str(), "chatFont.fnt");
        cl->setScale(0.22f); cl->setPosition(CCPoint(29, 6)); cl->setColor(ccColor3B{110, 140, 190});
        bg->addChild(cl);
        auto btn = CCMenuItemSpriteExtra::create(bg, this, menu_selector(SettingsLayer::onBuilderTag));
        btn->setTag(i);
        tagMenu->addChild(btn);
    }
    tagMenu->updateLayout();
    c->addChild(panelWith(tagMenu, 144.f));

    c->addChild(sectionHeader("Separator", this, info, "Separator",
        "Tags you tap are appended using this separator. Apply saves both the format and the separator."));
    auto ctrl = rowMenu(320.f, 52.f, 5.f, true);
    const char* seps[] = {" | ", " - ", " / "};
    for (int i = 0; i < 3; i++)
        ctrl->addChild(textBtn(seps[i], this, menu_selector(SettingsLayer::onBuilderSep),
                               i, m_sep == seps[i], 0.42f));
    ctrl->addChild(textBtn("Clear", this, menu_selector(SettingsLayer::onBuilderClear), 0, false, 0.42f));
    ctrl->addChild(textBtn("Apply", this, menu_selector(SettingsLayer::onBuilderApply), 0, true,  0.42f));
    ctrl->updateLayout();
    c->addChild(panelWith(ctrl, 60.f));

    c->addChild(sectionHeader("Presets"));
    auto presetMenu = rowMenu(320.f, 28.f, 6.f);
    const char* pNames[] = {"Minimalist", "Standard", "Developer"};
    for (int i = 0; i < 3; i++)
        presetMenu->addChild(textBtn(pNames[i], this, menu_selector(SettingsLayer::onPreset),
                                     i, false, 0.44f));
    presetMenu->updateLayout();
    c->addChild(panelWith(presetMenu, 36.f));

    refreshPreview();
}

// ── Callbacks ─────────────────────────────────────────────────────────────────

void SettingsLayer::onToggle(CCObject* s) {
    int  tag = static_cast<CCNode*>(s)->getTag();
    bool val = !static_cast<CCMenuItemToggler*>(s)->isToggled();   // callback runs before the flip
    auto& c  = g_config;
    switch (tag) {
        case 0:  c.enabled          = val; break;
        case 1:  c.hideOnEnterLevel = val; break;
        case 2:  c.hideOnExitLevel  = val; break;
        case 3:  c.colorCodingFPS   = val; break;
        case 4:  c.layoutMode       = val ? "Vertical" : "Horizontal"; break;
        case 10: c.showBg           = val; break;
    }
    commit();   // applied live
}

void SettingsLayer::onPosition(CCObject* s) {
    int i = static_cast<CCNode*>(s)->getTag();
    if (i >= 0 && i < 6) {
        g_config.posMode = POSITIONS[i];
        commit();
        reloadTabSoon(0);
    }
}

void SettingsLayer::onStartDrag(CCObject*) {
    if (!g_config.enabled) {
        FLAlertLayer::create("Overlay disabled", "Enable the overlay first.", "OK")->show();
        return;
    }
    this->keyBackClicked();   // closes this popup
    DragLayer::open();
}

void SettingsLayer::onAndroidOverride(CCObject* s) {
    g_config.os.androidOverride = static_cast<CCNode*>(s)->getTag();
    commit();
    reloadTabSoon(1);
}

void SettingsLayer::onLabelToggle(CCObject* s) {
    int  tag = static_cast<CCNode*>(s)->getTag();
    bool val = !static_cast<CCMenuItemToggler*>(s)->isToggled();
    auto& c  = g_config;
    switch (tag) {
        case 100: c.fps.showFPS             = val; break;
        case 101: c.fps.showTPS             = val; break;
        case 102: c.fps.showTogether        = val; break;
        case 103: c.fps.showTarget          = val; break;
        case 110: c.cps.enabled             = val; break;
        case 120: c.accuracy.enabled        = val; break;
        case 121: c.accuracy.requireNoclip  = val; break;
        case 130: c.deaths.showDeaths       = val; break;
        case 131: c.deaths.showSession      = val; break;
        case 200: c.mods.onlyActive         = val; break;
        case 400: c.user.showName           = val; break;
        case 500: c.time.is24h              = val; break;
        case 501: c.time.showSeconds        = val; break;
        case 600: c.session.showSeconds     = val; break;
        case 700: c.lvl.showAuthor          = val; break;
    }
    commit();
}

void SettingsLayer::onDateFormat(CCObject* s) {
    g_config.date.format = static_cast<CCNode*>(s)->getTag();
    commit();
    reloadTabSoon(1);
}

void SettingsLayer::onFont(CCObject* s) {
    int i = static_cast<CCNode*>(s)->getTag();
    if (i >= 0 && i < 4) {
        g_config.fontType = FONTS[i];
        commit();
        reloadTabSoon(2);
    }
}

void SettingsLayer::onScale(CCObject* s) {
    static const float sv[] = {0.3f, 0.4f, 0.45f, 0.5f, 0.6f, 0.7f, 0.8f};
    int i = static_cast<CCNode*>(s)->getTag();
    if (i >= 0 && i < 7) {
        g_config.textScale = sv[i];
        commit();
        reloadTabSoon(2);
    }
}

// ── Colors (Geode's ColorPickPopup: closable, proper touch handling) ──────────

void SettingsLayer::updateSwatches() {
    if (m_textSwatch) m_textSwatch->setColor(g_config.textColor);
    if (m_bgSwatch)   m_bgSwatch->setColor(g_config.bgColor);
}

void SettingsLayer::onPickTextColor(CCObject*) {
    auto* popup = ColorPickPopup::create(g_config.textColor);
    if (!popup) return;
    Ref<SettingsLayer> self(this);
    popup->setCallback([self](ccColor4B const& c) {
        g_config.textColor = ccColor3B{c.r, c.g, c.b};
        saveColor("text-color", g_config.textColor);
        self->updateSwatches();
    });
    popup->show();
}

void SettingsLayer::onPickBgColor(CCObject*) {
    auto* popup = ColorPickPopup::create(g_config.bgColor);
    if (!popup) return;
    Ref<SettingsLayer> self(this);
    popup->setCallback([self](ccColor4B const& c) {
        g_config.bgColor = ccColor3B{c.r, c.g, c.b};
        saveColor("bg-color", g_config.bgColor);
        self->updateSwatches();
    });
    popup->show();
}

// ── Builder ───────────────────────────────────────────────────────────────────

void SettingsLayer::onBuilderTag(CCObject* s) {
    int i = static_cast<CCNode*>(s)->getTag();
    if (!m_fmtInput || i < 0 || i >= static_cast<int>(m_tagVals.size())) return;
    if (!m_draft.empty()) m_draft += m_sep;
    m_draft += m_tagVals[i];
    m_fmtInput->setString(m_draft, false);
    refreshPreview();
}

void SettingsLayer::onBuilderSep(CCObject* s) {
    static const char* seps[] = {" | ", " - ", " / "};
    int i = static_cast<CCNode*>(s)->getTag();
    if (i >= 0 && i < 3) {
        m_sep = seps[i];
        reloadTabSoon(3);   // re-highlights the chosen separator; draft is preserved
    }
}

void SettingsLayer::onBuilderClear(CCObject*) {
    m_draft.clear();
    if (m_fmtInput) m_fmtInput->setString("", false);
    refreshPreview();
}

void SettingsLayer::onBuilderApply(CCObject*) {
    g_config.formatStr = m_draft;
    g_config.separator = m_sep;   // the separator is what splits the format into segments
    commit();
    FLAlertLayer::create("Applied", "Format string saved and applied!", "OK")->show();
}

void SettingsLayer::onPreset(CCObject* s) {
    static const char* presets[] = {
        "{fps} | {progress}",
        "{fps} | {time} | {lvl} | {progress}",
        "{fps} | {tps} | {geode} | {mods} | {os}"
    };
    int i = static_cast<CCNode*>(s)->getTag();
    if (i >= 0 && i < 3) {
        m_draft = presets[i];
        m_sep   = " | ";
        reloadTabSoon(3);
    }
}

void SettingsLayer::refreshPreview() {
    if (!m_preview) return;
    std::string text = renderFormat(m_draft, m_sep, false, sampleTags());
    m_preview->setString(("Preview: " + text).c_str());

    // Keep the preview inside the panel
    m_preview->setScale(0.35f);
    float w = m_preview->getContentSize().width * 0.35f;
    if (w > 340.f) m_preview->setScale(0.35f * 340.f / w);
}
