// Restricted "start positions only" mode for GD's real level editor.
#include "EditorPopup.hpp"
#include <Geode/modify/EditorUI.hpp>
#include <Geode/modify/EditorPauseLayer.hpp>

using namespace geode::prelude;

namespace {
    struct Session {
        bool active = false;
        GJGameLevel* target = nullptr; // the real level (retained), never saved
        LevelStartPosData before;
    } g_session;

    void endSession() {
        if (g_session.target) g_session.target->release();
        g_session = Session{};
    }

    bool restricted() { return g_session.active; }
    bool allowed(GameObject* obj) { return obj && obj->m_objectID == 31; }
}

void openStartPosEditor(GJGameLevel* level) {
    if (g_session.active) return;
    auto raw = storage::rawLevelString(level);
    if (raw.empty()) {
        FLAlertLayer::create("Start Positions", "This level's data isn't downloaded yet. Try again after it loads.", "OK")->show();
        return;
    }
    LevelStartPosData data = storage::load(level);
    bool hasSaved = storage::has(level);
    std::string str = hasSaved ? storage::applySaved(raw, data, true) : raw;

    // Work on a throwaway copy so the real level is never touched.
    auto copy = GJGameLevel::create();
    copy->m_levelName = level->m_levelName;
    copy->m_levelString = ZipUtils::compressString(str, false, 0);
    copy->m_audioTrack = level->m_audioTrack;
    copy->m_songID = level->m_songID;
    copy->m_songIDs = level->m_songIDs;
    copy->m_sfxIDs = level->m_sfxIDs;
    copy->m_levelType = GJLevelType::Editor;

    g_session.active = true;
    g_session.target = level;
    level->retain();
    g_session.before = data;

    auto scene = LevelEditorLayer::scene(copy, false);
    CCDirector::get()->pushScene(CCTransitionFade::create(0.5f, scene));
}

static void collectAndSave(LevelEditorLayer* editor) {
    if (!g_session.target || !editor) return;
    LevelStartPosData out;
    for (auto obj : CCArrayExt<GameObject*>(editor->m_objects)) {
        if (obj->m_objectID != 31) continue;
        auto sp = typeinfo_cast<StartPosObject*>(obj);
        SavedStartPos s;
        s.x = obj->getPositionX();
        s.y = obj->getPositionY();
        if (sp && sp->m_startSettings) {
            s.gamemode = sp->m_startSettings->m_startMode;
            s.speed = (int)sp->m_startSettings->m_startSpeed;
            s.mini = sp->m_startSettings->m_startMini;
            s.flipped = sp->m_startSettings->m_isFlipped;
        }
        out.positions.push_back(s);
    }
    std::sort(out.positions.begin(), out.positions.end(), [](auto& a, auto& b) { return a.x < b.x; });

    // keep the previously chosen start position selected if it still exists
    out.active = -1;
    auto& b = g_session.before;
    if (b.active >= 0 && b.active < (int)b.positions.size()) {
        float ax = b.positions[b.active].x;
        for (size_t i = 0; i < out.positions.size(); i++) {
            if (std::abs(out.positions[i].x - ax) < 1.f) { out.active = (int)i; break; }
        }
    }
    if (out.active == -1 && !out.positions.empty()) out.active = (int)out.positions.size() - 1;

    storage::save(g_session.target, out);
    g_session.before = out;
}

static void leaveEditor() {
    endSession();
    GameManager::get()->fadeInMenuMusic();
    CCDirector::get()->popSceneWithTransition(0.5f, PopTransition::kPopTransitionFade);
}

class $modify(SPEditorUI, EditorUI) {
    bool init(LevelEditorLayer* layer) {
        if (!EditorUI::init(layer)) return false;
        if (!restricted()) return true;
        // Every build tab only offers the start position object.
        if (m_createButtonBars) {
            for (auto bar : CCArrayExt<EditButtonBar*>(m_createButtonBars)) {
                auto items = CCArray::create();
                items->addObject(this->getCreateBtn(31, 4));
                bar->loadFromItems(items, 2, 6, false);
            }
        }
        Notification::create("Start position mode: only start positions can be edited", NotificationIcon::Info)->show();
        return true;
    }

    void selectObject(GameObject* obj, bool ignoreFilter) {
        if (restricted() && !allowed(obj)) return;
        EditorUI::selectObject(obj, ignoreFilter);
    }

    void selectObjects(CCArray* objs, bool ignoreFilter) {
        if (restricted() && objs) {
            auto filtered = CCArray::create();
            for (auto o : CCArrayExt<GameObject*>(objs)) if (allowed(o)) filtered->addObject(o);
            EditorUI::selectObjects(filtered, ignoreFilter);
            return;
        }
        EditorUI::selectObjects(objs, ignoreFilter);
    }

    void onPaste(CCObject* s) {
        if (restricted()) return; // pasting arbitrary objects isn't allowed
        EditorUI::onPaste(s);
    }

    void onDeleteSelectedType(CCObject* s) {
        if (restricted()) return; // could delete non-startpos objects
        EditorUI::onDeleteSelectedType(s);
    }
};

class $modify(SPEditorPauseLayer, EditorPauseLayer) {
    void saveLevel() {
        if (!restricted()) return EditorPauseLayer::saveLevel();
        collectAndSave(m_editorLayer); // never save the level itself
    }
    void onSave(CCObject* s) {
        if (!restricted()) return EditorPauseLayer::onSave(s);
        collectAndSave(m_editorLayer);
        Notification::create("Start positions saved", NotificationIcon::Success)->show();
    }
    void onSaveAndPlay(CCObject* s) {
        if (!restricted()) return EditorPauseLayer::onSaveAndPlay(s);
        collectAndSave(m_editorLayer);
        Notification::create("Saved! Exit and press Play to try it", NotificationIcon::Success)->show();
    }
    void onSaveAndExit(CCObject* s) {
        if (!restricted()) return EditorPauseLayer::onSaveAndExit(s);
        collectAndSave(m_editorLayer);
        leaveEditor();
    }
    void onExitEditor(CCObject* s) {
        if (!restricted()) return EditorPauseLayer::onExitEditor(s);
        collectAndSave(m_editorLayer);
        leaveEditor();
    }
    void onExitNoSave(CCObject* s) {
        if (!restricted()) return EditorPauseLayer::onExitNoSave(s);
        geode::createQuickPopup("Exit", "Exit without saving your start position changes?", "Cancel", "Exit",
            [](FLAlertLayer*, bool yes) { if (yes) leaveEditor(); });
    }
};
