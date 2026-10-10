#pragma once
#include "Storage.hpp"

// Opens GD's real level editor in "start positions only" mode.
void openStartPosEditor(GJGameLevel* level);

class StartPosEditorPopup : public geode::Popup {
protected:
    GJGameLevel* m_level = nullptr;
    LevelStartPosData m_data;
    int m_page = 0;
    cocos2d::CCMenu* m_listMenu = nullptr;

    bool init(GJGameLevel* level);
    void onOpenEditor(cocos2d::CCObject*);
    void onRemove(cocos2d::CCObject*);
    void onSelect(cocos2d::CCObject*);
    void onPage(cocos2d::CCObject*);
    void onClearActive(cocos2d::CCObject*);
    void onReset(cocos2d::CCObject*);
    void refreshList();
    void persist();

public:
    ~StartPosEditorPopup() { if (m_level) m_level->release(); }
    static StartPosEditorPopup* create(GJGameLevel* level);
};

CCMenuItemSpriteExtra* makeStartPosButton(cocos2d::CCNode* target, GJGameLevel* level);
