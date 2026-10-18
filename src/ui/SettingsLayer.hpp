#pragma once
#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/ScrollLayer.hpp>
#include <Geode/ui/TextInput.hpp>
#include <vector>
#include <string>

using namespace geode::prelude;

namespace uaci {

// Settings window. Built on geode::Popup (Geode 5, non-templated) so it gets proper
// touch priority/swallowing, back-button handling, close button and background for free.
// Window size is 400x290: GD's design height is 320, so it always fits on screen.
class SettingsLayer : public geode::Popup {
protected:
    int            m_tab       = 0;
    CCMenu*        m_tabMenu   = nullptr;
    ScrollLayer*   m_scroll    = nullptr;
    TextInput*     m_fmtInput  = nullptr;
    CCLabelBMFont* m_preview   = nullptr;
    CCLayerColor*  m_textSwatch = nullptr;
    CCLayerColor*  m_bgSwatch   = nullptr;

    // Builder draft (survives tab switches until Apply)
    std::string              m_draft;
    std::string              m_sep       = " | ";
    bool                     m_draftInit = false;
    std::vector<std::string> m_tagVals;

    bool init();

    void buildTabs();
    void loadTab(int t);
    void reloadTabSoon(int t);   // deferred: never destroys the button that is being clicked
    void buildGeneral(CCNode* c);
    void buildLabels(CCNode* c);
    void buildVisual(CCNode* c);
    void buildBuilder(CCNode* c);

    void refreshPreview();
    void updateSwatches();

    void onTab(CCObject*);
    void onToggle(CCObject*);
    void onPosition(CCObject*);
    void onStartDrag(CCObject*);
    void onAndroidOverride(CCObject*);
    void onLabelToggle(CCObject*);
    void onDateFormat(CCObject*);
    void onFont(CCObject*);
    void onScale(CCObject*);
    void onPickTextColor(CCObject*);
    void onPickBgColor(CCObject*);
    void onBuilderTag(CCObject*);
    void onBuilderSep(CCObject*);
    void onBuilderClear(CCObject*);
    void onBuilderApply(CCObject*);
    void onPreset(CCObject*);

public:
    // Returns nullptr if a settings window is already open.
    static SettingsLayer* create();
    ~SettingsLayer();
};

} // namespace uaci
