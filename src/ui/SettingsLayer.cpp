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
//  Small helpers
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

static CCNode* sectionTitle(const char* t) {
    auto l = CCLabelBMFont::create(t, "goldFont.fnt");
    l->setScale(0.48f);
    l->setAnchorPoint(CCPoint(0, .5f));
    return l;
}

static CCNode* toggleRow(const char* label, bool val,
                         CCObject* tgt, SEL_MenuHandler sel, int tag = 0) {
    auto row = CCNode::create();
    row->setContentSize(CCSize(340.f, 28.f));

    auto l = CCLabelBMFont::create(label, "chatFont.fnt");
    l->setScale(0.40f);
    l->setAnchorPoint(CCPoint(0, .5f));
    l->setPosition(CCPoint(8, 14));
    row->addChild(l);

    auto tog = CCMenuItemToggler::createWithStandardSprites(tgt, sel, 0.65f);
    tog->toggle(val);
    tog->setTag(tag);
    auto m = CCMenu::create();
    m->addChild(tog);
    m->setPosition(CCPoint(318, 14));
    row->addChild(m);
    return row;
}

static CCMenuItemSpriteExtra* textBtn(const char* lbl, CCObject* tgt,
                                      SEL_MenuHandler sel, int tag,
                                      bool active, float scale = 0.44f) {
    auto spr = ButtonSprite::create(lbl, "chatFont.fnt",
               active ? "GJ_button_02.png" : "GJ_button_04.png", scale);
    auto btn = CCMenuItemSpriteExtra::create(spr, tgt, sel);
    btn->setTag(tag);
    return btn;
}

// CCMenu with an EXPLICIT size (CCMenu::create() defaults to the whole window size,
// which made every row/column layout enormous in the original).
static CCMenu* rowMenu(float w, float h, float gap, bool wrap = false) {
    auto m = CCMenu::create();
    m->setContentSize(CCSize(w, h));
    auto layout = RowLayout::create();
    layout->setGap(gap);
    layout->setAxisAlignment(AxisAlignment::Start);
    layout->setAutoScale(false);
    layout->setGrowCrossAxis(wrap);
    if (wrap) layout->setCrossAxisReverse(true);   // first row on top
    m->setLayout(layout);
    return m;
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

    // Scroll area (360 x 205, sits below the tabs inside the 400 x 290 panel)
    m_scroll = ScrollLayer::create(CCSize(360.f, 205.f));
    m_scroll->setPosition(CCPoint((m_size.width - 360.f) / 2.f, 16.f));
    m_mainLayer->addChild(m_scroll, 2);

    loadTab(0);
    return true;
}

// ── Tabs ──────────────────────────────────────────────────────────────────────

void SettingsLayer::buildTabs() {
    static const char* names[] = {"General", "Labels", "Visual", "Builder"};
    m_tabMenu->removeAllChildren();
    for (int i = 0; i < 4; i++)
        m_tabMenu->addChild(textBtn(names[i], this, menu_selector(SettingsLayer::onTab),
                                    i, i == m_tab, 0.42f));
    m_tabMenu->updateLayout();
}

void SettingsLayer::loadTab(int t) {
    m_tab        = t;
    m_fmtInput   = nullptr;
    m_preview    = nullptr;
    m_textSwatch = nullptr;
    m_bgSwatch   = nullptr;

    buildTabs();
    m_scroll->m_contentLayer->removeAllChildren();

    auto content = CCNode::create();
    content->setAnchorPoint(CCPoint(0, 0));
    content->setContentSize(CCSize(360.f, 10.f));
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
    col->setCrossAxisAlignment(AxisAlignment::Start);
    col->setAutoGrowAxis(205.f);
    col->setAutoScale(false);
    content->setLayout(col);
    content->updateLayout();

    m_scroll->m_contentLayer->addChild(content);
    float h = std::max(content->getContentSize().height, 205.f);
    m_scroll->m_contentLayer->setContentSize(CCSize(360.f, h));
    content->setPosition(CCPoint(0, h - content->getContentSize().height));
    m_scroll->moveToTop();
}

// Rebuilding a tab destroys the buttons of the old one — including the button whose
// callback is running. Defer to the next frame to avoid use-after-free.
void SettingsLayer::reloadTabSoon(int t) {
    Loader::get()->queueInMainThread([self = Ref<SettingsLayer>(this), t]() {
        self->loadTab(t);
    });
}

void SettingsLayer::onTab(CCObject* s) {
    reloadTabSoon(static_cast<CCNode*>(s)->getTag());
}

// ── TAB: GENERAL ─────────────────────────────────────────────────────────────

void SettingsLayer::buildGeneral(CCNode* c) {
    auto& cfg = g_config;
    c->addChild(sectionTitle("Overlay"));
    c->addChild(toggleRow("Enable Overlay",            cfg.enabled,          this, menu_selector(SettingsLayer::onToggle), 0));
    c->addChild(toggleRow("Hide when entering level",  cfg.hideOnEnterLevel, this, menu_selector(SettingsLayer::onToggle), 1));
    c->addChild(toggleRow("Hide when leaving level",   cfg.hideOnExitLevel,  this, menu_selector(SettingsLayer::onToggle), 2));
    c->addChild(toggleRow("FPS Color Coding",          cfg.colorCodingFPS,   this, menu_selector(SettingsLayer::onToggle), 3));
    c->addChild(toggleRow("Vertical Layout",           cfg.layoutMode == "Vertical", this, menu_selector(SettingsLayer::onToggle), 4));

    c->addChild(sectionTitle("Position"));
    auto posMenu = rowMenu(350.f, 64.f, 4.f, true);
    for (int i = 0; i < 6; i++)
        posMenu->addChild(textBtn(POSITIONS[i], this, menu_selector(SettingsLayer::onPosition),
                                  i, cfg.posMode == POSITIONS[i], 0.36f));
    posMenu->updateLayout();
    c->addChild(posMenu);

    c->addChild(sectionTitle("Fine tuning"));
    auto dragMenu = rowMenu(350.f, 30.f, 5.f);
    dragMenu->addChild(textBtn("Drag to reposition", this,
                               menu_selector(SettingsLayer::onStartDrag), 0, false, 0.44f));
    dragMenu->updateLayout();
    c->addChild(dragMenu);
}

// ── TAB: LABELS ──────────────────────────────────────────────────────────────

void SettingsLayer::buildLabels(CCNode* c) {
    auto& cfg = g_config;
    auto  sel = menu_selector(SettingsLayer::onLabelToggle);

    c->addChild(sectionTitle("FPS / TPS"));
    c->addChild(toggleRow("Show FPS",               cfg.fps.showFPS,      this, sel, 100));
    c->addChild(toggleRow("Show TPS",               cfg.fps.showTPS,      this, sel, 101));
    c->addChild(toggleRow("Show together (60/240)", cfg.fps.showTogether, this, sel, 102));
    c->addChild(toggleRow("Show target Hz",         cfg.fps.showTarget,   this, sel, 103));

    c->addChild(sectionTitle("CPS"));
    c->addChild(toggleRow("Show CPS",               cfg.cps.enabled,      this, sel, 110));

    c->addChild(sectionTitle("Accuracy"));
    c->addChild(toggleRow("Show Accuracy",          cfg.accuracy.enabled, this, sel, 120));
    c->addChild(toggleRow("Only after a noclip hit",cfg.accuracy.requireNoclip, this, sel, 121));

    c->addChild(sectionTitle("Deaths"));
    c->addChild(toggleRow("Show Deaths",            cfg.deaths.showDeaths,  this, sel, 130));
    c->addChild(toggleRow("Session deaths (off = level attempts)", cfg.deaths.showSession, this, sel, 131));

    c->addChild(sectionTitle("Mods"));
    c->addChild(toggleRow("Only count active mods", cfg.mods.onlyActive,  this, sel, 200));

    c->addChild(sectionTitle("OS - Android"));
    auto aMenu = rowMenu(350.f, 30.f, 5.f);
    const char* aOpts[] = {"Auto", "Android", "GooglePlayRC"};
    for (int i = 0; i < 3; i++)
        aMenu->addChild(textBtn(aOpts[i], this, menu_selector(SettingsLayer::onAndroidOverride),
                                i, cfg.os.androidOverride == i, 0.42f));
    aMenu->updateLayout();
    c->addChild(aMenu);

    c->addChild(sectionTitle("User"));
    c->addChild(toggleRow("Show name (off = UID)",  cfg.user.showName,    this, sel, 400));

    c->addChild(sectionTitle("Date Format"));
    auto dMenu = rowMenu(350.f, 30.f, 5.f);
    const char* dfmts[] = {"dd/mm/yy", "dd/mm/yyyy", "Full text"};
    for (int i = 0; i < 3; i++)
        dMenu->addChild(textBtn(dfmts[i], this, menu_selector(SettingsLayer::onDateFormat),
                                i, cfg.date.format == i, 0.42f));
    dMenu->updateLayout();
    c->addChild(dMenu);

    c->addChild(sectionTitle("Time"));
    c->addChild(toggleRow("24-hour clock",          cfg.time.is24h,         this, sel, 500));
    c->addChild(toggleRow("Show seconds",           cfg.time.showSeconds,   this, sel, 501));

    c->addChild(sectionTitle("Session"));
    c->addChild(toggleRow("Show seconds",           cfg.session.showSeconds,this, sel, 600));

    c->addChild(sectionTitle("Level"));
    c->addChild(toggleRow("Show 'Level by Author'", cfg.lvl.showAuthor,     this, sel, 700));
}

// ── TAB: VISUAL ───────────────────────────────────────────────────────────────

void SettingsLayer::buildVisual(CCNode* c) {
    auto& cfg = g_config;

    c->addChild(sectionTitle("Font (tap to apply)"));
    const char* fLabels[] = {"Chat", "Gold", "Big", "GJ01"};
    auto fontMenu = rowMenu(350.f, 40.f, 6.f);
    for (int i = 0; i < 4; i++) {
        auto bg = CCScale9Sprite::create("square02_001.png");
        bg->setContentSize(CCSize(80.f, 36.f));
        bg->setColor(cfg.fontType == FONTS[i] ? ccColor3B{40, 80, 40} : ccColor3B{25, 25, 60});
        bg->setOpacity(220);
        auto prev = CCLabelBMFont::create(fLabels[i], FONTS[i]);
        prev->setScale(0.42f);
        prev->setPosition(CCPoint(40, 18));
        bg->addChild(prev);
        auto btn = CCMenuItemSpriteExtra::create(bg, this, menu_selector(SettingsLayer::onFont));
        btn->setTag(i);
        fontMenu->addChild(btn);
    }
    fontMenu->updateLayout();
    c->addChild(fontMenu);

    c->addChild(sectionTitle("Scale"));
    auto scaleMenu = rowMenu(350.f, 30.f, 4.f);
    const float sv[]   = {0.3f, 0.4f, 0.45f, 0.5f, 0.6f, 0.7f, 0.8f};
    const char* sl[]   = {"0.3", "0.4", "0.45", "0.5", "0.6", "0.7", "0.8"};
    for (int i = 0; i < 7; i++)
        scaleMenu->addChild(textBtn(sl[i], this, menu_selector(SettingsLayer::onScale),
                                    i, std::abs(cfg.textScale - sv[i]) < 0.01f, 0.40f));
    scaleMenu->updateLayout();
    c->addChild(scaleMenu);

    c->addChild(sectionTitle("Colors"));
    auto colorMenu = rowMenu(350.f, 30.f, 8.f);

    auto textColorSpr = ButtonSprite::create("Text Color", "chatFont.fnt", "GJ_button_04.png", 0.5f);
    m_textSwatch = CCLayerColor::create(
        ccColor4B{cfg.textColor.r, cfg.textColor.g, cfg.textColor.b, 255}, 14.f, 14.f);
    m_textSwatch->setPosition(CCPoint(4, 4));
    textColorSpr->addChild(m_textSwatch);
    colorMenu->addChild(CCMenuItemSpriteExtra::create(
        textColorSpr, this, menu_selector(SettingsLayer::onPickTextColor)));

    auto bgColorSpr = ButtonSprite::create("BG Color", "chatFont.fnt", "GJ_button_04.png", 0.5f);
    m_bgSwatch = CCLayerColor::create(
        ccColor4B{cfg.bgColor.r, cfg.bgColor.g, cfg.bgColor.b, 255}, 14.f, 14.f);
    m_bgSwatch->setPosition(CCPoint(4, 4));
    bgColorSpr->addChild(m_bgSwatch);
    colorMenu->addChild(CCMenuItemSpriteExtra::create(
        bgColorSpr, this, menu_selector(SettingsLayer::onPickBgColor)));

    colorMenu->updateLayout();
    c->addChild(colorMenu);

    c->addChild(toggleRow("Show Background", cfg.showBg, this, menu_selector(SettingsLayer::onToggle), 10));
}

// ── TAB: BUILDER ─────────────────────────────────────────────────────────────

void SettingsLayer::buildBuilder(CCNode* c) {
    auto& cfg = g_config;

    if (!m_draftInit) {
        m_draft     = cfg.formatStr;
        m_sep       = cfg.separator.empty() ? std::string(" | ") : cfg.separator;
        m_draftInit = true;
    }

    c->addChild(sectionTitle("Label Builder"));

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
    m_preview->setColor({180, 220, 255});
    m_preview->setAnchorPoint(CCPoint(0, .5f));
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
    auto tagMenu = rowMenu(350.f, 140.f, 4.f, true);
    for (int i = 0; i < static_cast<int>(tags.size()); i++) {
        m_tagVals.push_back(tags[i].first);
        auto bg = CCScale9Sprite::create("square02_001.png");
        bg->setContentSize(CCSize(58.f, 24.f));
        bg->setColor({25, 25, 65});
        bg->setOpacity(220);
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
    c->addChild(tagMenu);

    // Separator choice (highlighted) + Clear / Apply
    c->addChild(sectionTitle("Separator"));
    auto ctrl = rowMenu(350.f, 56.f, 5.f, true);
    const char* seps[] = {" | ", " - ", " / "};
    for (int i = 0; i < 3; i++)
        ctrl->addChild(textBtn(seps[i], this, menu_selector(SettingsLayer::onBuilderSep),
                               i, m_sep == seps[i], 0.42f));
    ctrl->addChild(textBtn("Clear", this, menu_selector(SettingsLayer::onBuilderClear), 0, false, 0.42f));
    ctrl->addChild(textBtn("Apply", this, menu_selector(SettingsLayer::onBuilderApply), 0, true,  0.42f));
    ctrl->updateLayout();
    c->addChild(ctrl);

    c->addChild(sectionTitle("Presets"));
    auto presetMenu = rowMenu(350.f, 30.f, 6.f);
    const char* pNames[] = {"Minimalist", "Standard", "Developer"};
    for (int i = 0; i < 3; i++)
        presetMenu->addChild(textBtn(pNames[i], this, menu_selector(SettingsLayer::onPreset),
                                     i, false, 0.44f));
    presetMenu->updateLayout();
    c->addChild(presetMenu);

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
    commit();   // applied live, no more "only on close"
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
    int i = static_cast<int>(static_cast<CCNode*>(s)->getTag());
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
