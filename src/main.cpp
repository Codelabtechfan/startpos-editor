#include <Geode/Geode.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>
#include <Geode/modify/LevelPage.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include "EditorPopup.hpp"

using namespace geode::prelude;

// Adds the start position button to the newest info popup (FLAlertLayer) in the scene.
static void addButtonToInfoPopup(GJGameLevel* level) {
    if (!level) return;
    auto scene = CCDirector::get()->getRunningScene();
    if (!scene) return;
    FLAlertLayer* popup = nullptr;
    for (auto child : CCArrayExt<CCNode*>(scene->getChildren())) {
        if (auto a = typeinfo_cast<FLAlertLayer*>(child)) popup = a; // newest one
    }
    if (!popup || !popup->m_mainLayer) return;
    if (popup->m_mainLayer->getChildByIDRecursive("startpos-editor-button"_spr)) return;
    auto win = CCDirector::get()->getWinSize();
    auto menu = CCMenu::create();
    menu->setPosition({win.width / 2 + 150.f, win.height / 2 - 60.f});
    auto btn = makeStartPosButton(menu, level);
    btn->setScale(0.8f);
    btn->m_baseScale = 0.8f;
    menu->addChild(btn);
    menu->setTouchPriority(popup->m_buttonMenu ? popup->m_buttonMenu->getTouchPriority() : -502);
    popup->m_mainLayer->addChild(menu, 20);
}

// Online / your own levels: button inside the level's info popup ("i" button).
class $modify(SPLevelInfoLayer, LevelInfoLayer) {
    void onInfo(CCObject* sender) {
        LevelInfoLayer::onInfo(sender);
        addButtonToInfoPopup(m_level);
    }
};

// Main levels: button inside the level's info popup (the "i" button on the level page).
class $modify(SPLevelPage, LevelPage) {
    void onInfo(CCObject* sender) {
        LevelPage::onInfo(sender);
        if (!m_level || m_level->m_levelID.value() <= 0) return; // skip "coming soon"/tower pages
        addButtonToInfoPopup(m_level);
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

    StartPosObject* findChosen() {
        if (!m_level) return nullptr;
        auto data = storage::load(m_level);
        if (data.active < 0 || data.active >= (int)data.positions.size()) return nullptr;
        auto& want = data.positions[data.active];
        StartPosObject* chosen = nullptr;
        float best = 5.f;
        for (auto sp : m_fields->startPositions) {
            float d = std::abs(sp->getPositionX() - want.x) + std::abs(sp->getPositionY() - want.y);
            if (d < best) { best = d; chosen = sp; }
        }
        return chosen;
    }

    void applyChoice() {
        auto chosen = findChosen();
        this->setStartPosObject(chosen); // nullptr = start from 0%
        m_isTestMode = chosen != nullptr;
        this->updateTestModeLabel();
    }

    // Switcher mods (e.g. capeling's StartPos Switcher) read m_startPosObject right after
    // this function to work out their n/N index, so set our choice BEFORE calling it.
    void createObjectsFromSetupFinished() {
        if (!m_fields->appliedInitial) {
            m_fields->appliedInitial = true;
            applyChoice();
        }
        PlayLayer::createObjectsFromSetupFinished();
    }

    void setupHasCompleted() {
        if (!m_fields->appliedInitial) { // fallback if the above didn't run
            m_fields->appliedInitial = true;
            applyChoice();
        }
        PlayLayer::setupHasCompleted();
    }
};
