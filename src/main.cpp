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

// Keep ALL start positions in the level (saved list if any, otherwise the originals) so
// start-pos switcher mods can see and cycle them. Only on the initial load, pick which one
// is active: the one chosen with "Use", or none (start from 0%).
class $modify(SPPlayLayer, PlayLayer) {
    struct Fields {
        std::vector<StartPosObject*> startPositions;
        bool appliedInitial = false;
    };

    void prepareCreateObjectsFromSetup(gd::string& levelString) {
        if (m_level && storage::has(m_level)) {
            auto data = storage::load(m_level);
            std::string str = levelString;
            if (!str.empty() && str.find(';') == std::string::npos) {
                str = ZipUtils::decompressString(str, false, 0);
            }
            levelString = storage::applySaved(str, data, true);
        }
        PlayLayer::prepareCreateObjectsFromSetup(levelString);
    }

    void addObject(GameObject* obj) {
        PlayLayer::addObject(obj);
        if (obj && obj->m_objectID == 31) {
            if (auto sp = typeinfo_cast<StartPosObject*>(obj)) m_fields->startPositions.push_back(sp);
        }
    }

    void setupHasCompleted() {
        if (!m_fields->appliedInitial && m_level) {
            m_fields->appliedInitial = true;
            StartPosObject* chosen = nullptr;
            auto data = storage::load(m_level);
            if (data.active >= 0 && data.active < (int)data.positions.size()) {
                auto& want = data.positions[data.active];
                float best = 5.f;
                for (auto sp : m_fields->startPositions) {
                    float d = std::abs(sp->getPositionX() - want.x) + std::abs(sp->getPositionY() - want.y);
                    if (d < best) { best = d; chosen = sp; }
                }
            }
            this->setStartPosObject(chosen); // nullptr = start from 0%
        }
        PlayLayer::setupHasCompleted();
    }
};
