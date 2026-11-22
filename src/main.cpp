#include <Geode/Geode.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>
#include <Geode/modify/LevelPage.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/FLAlertLayer.hpp>
#include "EditorPopup.hpp"
#include "Switcher.hpp"

using namespace geode::prelude;

// Shared placement: same position/scale for main-level and online-level popups.
static void attachButton(FLAlertLayer* popup, GJGameLevel* level) {
    if (!popup || !level || !popup->m_mainLayer) return;
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

static FLAlertLayer* newestAlert() {
    auto scene = CCDirector::get()->getRunningScene();
    if (!scene) return nullptr;
    FLAlertLayer* popup = nullptr;
    for (auto child : CCArrayExt<CCNode*>(scene->getChildren())) {
        if (auto a = typeinfo_cast<FLAlertLayer*>(child)) popup = a;
    }
    return popup;
}

static void addButtonToInfoPopup(GJGameLevel* level) { attachButton(newestAlert(), level); }

// Level whose stats popup is about to be shown (set during LevelInfoLayer::onLevelInfo).
static GJGameLevel* g_pendingStatsLevel = nullptr;

class $modify(SPAlert, FLAlertLayer) {
    void show() {
        FLAlertLayer::show();
        if (g_pendingStatsLevel) {
            auto lvl = g_pendingStatsLevel;
            g_pendingStatsLevel = nullptr;
            attachButton(this, lvl);
        }
    }
};

// Online / your own levels: button inside the level stats popup (small blue "i" icon).
class $modify(SPLevelInfoLayer, LevelInfoLayer) {
    void onLevelInfo(CCObject* sender) {
        g_pendingStatsLevel = m_level;
        LevelInfoLayer::onLevelInfo(sender);
        g_pendingStatsLevel = nullptr;
        // Fallback in case show() is deferred/overridden: try again next frame.
        Ref<GJGameLevel> lvl = m_level;
        Loader::get()->queueInMainThread([lvl] { addButtonToInfoPopup(lvl); });
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
// the built-in switcher can cycle them. On the initial load, pick which one is active:
// the one chosen with "Use", or none (start from 0%).
class $modify(SPPlayLayer, PlayLayer) {
    struct Fields {
        std::vector<StartPosObject*> startPositions;
        bool appliedInitial = false;
        int switcherIndex = 0; // 0 = from start, 1..N = start positions in X order
    };

    void prepareCreateObjectsFromSetup(std::string& levelString) {
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
        this->setActiveStartPosObject(chosen); // nullptr = start from 0%
        m_isTestMode = chosen != nullptr;
        this->updateTestModeLabel();
    }

    // Apply the "Use" choice before the original function returns, then sync the
    // built-in switcher's index (0 = from start) to that choice.
    void createObjectsFromSetupFinished() {
        if (!m_fields->appliedInitial) {
            m_fields->appliedInitial = true;
            applyChoice();
        }
        PlayLayer::createObjectsFromSetupFinished();
        switcher::syncIndexFromActive();
    }

    void setupHasCompleted() {
        if (!m_fields->appliedInitial) { // fallback if the above didn't run
            m_fields->appliedInitial = true;
            applyChoice();
        }
        PlayLayer::setupHasCompleted();
        switcher::syncIndexFromActive();
    }
};

bool switcher::view(int& index, int& count) {
    auto play = static_cast<SPPlayLayer*>(PlayLayer::get());
    if (!play) { index = 0; count = 0; return false; }
    index = play->m_fields->switcherIndex;
    count = (int)play->m_fields->startPositions.size();
    return true;
}

void switcher::syncIndexFromActive() {
    auto play = static_cast<SPPlayLayer*>(PlayLayer::get());
    if (!play) return;
    auto& list = play->m_fields->startPositions;
    switcher::sortByX(list);
    play->m_fields->switcherIndex = 0;
    if (auto cur = play->m_startPosObject) {
        for (int i = 0; i < (int)list.size(); i++) {
            if (list[i] == cur) { play->m_fields->switcherIndex = i + 1; break; }
        }
    }
    switcher::refreshUI();
}

void switcher::cycle(int delta) {
    auto play = static_cast<SPPlayLayer*>(PlayLayer::get());
    if (!play || !switcher::active() || play->m_isPaused) return;
    auto& list = play->m_fields->startPositions;
    if (list.empty()) return;
    int n = (int)list.size();
    int idx = play->m_fields->switcherIndex + delta;
    if (idx < 0) idx = n;
    if (idx > n) idx = 0;
    play->m_fields->switcherIndex = idx;
    auto obj = idx == 0 ? nullptr : list[idx - 1];
    play->setActiveStartPosObject(obj);
    play->m_isTestMode = obj != nullptr;
    play->updateTestModeLabel();
    if (play->m_isPracticeMode) play->resetLevelFromStart();
    play->resetLevel();
    play->startMusic();
    switcher::refreshUI();
}
