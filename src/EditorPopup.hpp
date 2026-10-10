#pragma once
#include "Storage.hpp"

class StartPosEditorPopup : public geode::Popup {
protected:
    GJGameLevel* m_level = nullptr;
    LevelStartPosData m_data;
    float m_length = 30000.f;
    SavedStartPos m_draft;
    int m_page = 0;

    Slider* m_slider = nullptr;
    geode::TextInput* m_xInput = nullptr;
    cocos2d::CCLabelBMFont* m_percentLabel = nullptr;
    cocos2d::CCLabelBMFont* m_modeLabel = nullptr;
    cocos2d::CCLabelBMFont* m_speedLabel = nullptr;
    CCMenuItemToggler* m_miniToggle = nullptr;
    CCMenuItemToggler* m_flipToggle = nullptr;
    cocos2d::CCMenu* m_listMenu = nullptr;

    bool init(GJGameLevel* level);
    void onSlider(cocos2d::CCObject*);
    void onMode(cocos2d::CCObject*);
    void onSpeed(cocos2d::CCObject*);
    void onAdd(cocos2d::CCObject*);
    void onRemove(cocos2d::CCObject*);
    void onSelect(cocos2d::CCObject*);
    void onPage(cocos2d::CCObject*);
    void onClearActive(cocos2d::CCObject*);
    void refreshDraft();
    void refreshList();
    void persist();

public:
    ~StartPosEditorPopup() { if (m_level) m_level->release(); }
    static StartPosEditorPopup* create(GJGameLevel* level);
};

CCMenuItemSpriteExtra* makeStartPosButton(cocos2d::CCNode* target, GJGameLevel* level);
