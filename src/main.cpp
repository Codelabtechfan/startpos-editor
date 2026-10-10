#include <Geode/Geode.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>
#include <Geode/modify/LevelPage.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include "EditorPopup.hpp"

using namespace geode::prelude;

// Online / your own uploaded levels: button right next to the Play button.
class $modify(SPLevelInfoLayer, LevelInfoLayer) {
    bool init(GJGameLevel* level, bool challenge) {
        if (!LevelInfoLayer::init(level, challenge)) return false;
        auto menu = this->getChildByID("play-menu");
        auto play = menu ? menu->getChildByID("play-button") : nullptr;
        auto btn = makeStartPosButton(this, level);
        if (menu && play) {
            btn->setPosition(play->getPosition() + CCPoint{play->getScaledContentSize().width / 2 + 35.f, 0.f});
            menu->addChild(btn);
        } else {
            auto m = CCMenu::create();
            auto win = CCDirector::get()->getWinSize();
            m->setPosition({win.width / 2 + 90.f, win.height / 2 + 50.f});
            m->addChild(btn);
            this->addChild(m, 10);
        }
        return true;
    }
};

// Main levels: button inside the level's info popup (the "i" button on the level page).
class $modify(SPLevelPage, LevelPage) {
    void onInfo(CCObject* sender) {
        LevelPage::onInfo(sender);
        if (!m_level || m_level->m_levelID.value() <= 0) return; // skip "coming soon"/tower pages
        auto scene = CCDirector::get()->getRunningScene();
        if (!scene) return;
        FLAlertLayer* popup = nullptr;
        for (auto child : CCArrayExt<CCNode*>(scene->getChildren())) {
            if (auto a = typeinfo_cast<FLAlertLayer*>(child)) popup = a; // newest one
        }
        if (!popup || !popup->m_mainLayer) return;
        auto win = CCDirector::get()->getWinSize();
        auto menu = CCMenu::create();
        menu->setPosition({win.width / 2 + 150.f, win.height / 2 - 60.f});
        auto btn = makeStartPosButton(menu, m_level);
        btn->setScale(0.8f);
        btn->m_baseScale = 0.8f;
        menu->addChild(btn);
        // FLAlertLayer has its own touch priority; register our menu at the same priority
        menu->setTouchPriority(popup->m_buttonMenu ? popup->m_buttonMenu->getTouchPriority() : -502);
        popup->m_mainLayer->addChild(menu, 20);
    }
};

// Replace the level's start positions with the saved ones (only the chosen one is used).
class $modify(SPPlayLayer, PlayLayer) {
    void prepareCreateObjectsFromSetup(gd::string& levelString) {
        if (m_level) { // always: no chosen start pos = start from 0%
            auto data = storage::load(m_level);
            std::string str = levelString;
            if (!str.empty() && str.find(';') == std::string::npos) {
                str = ZipUtils::decompressString(str, false, 0);
            }
            levelString = storage::applySaved(str, data, false);
            
        }
        PlayLayer::prepareCreateObjectsFromSetup(levelString);
    }
};
