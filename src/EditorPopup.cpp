#include "EditorPopup.hpp"
#include <fmt/format.h>

using namespace geode::prelude;

static constexpr int PER_PAGE = 6;

StartPosEditorPopup* StartPosEditorPopup::create(GJGameLevel* level) {
    auto ret = new StartPosEditorPopup();
    if (ret->init(level)) { ret->autorelease(); return ret; }
    delete ret;
    return nullptr;
}

bool StartPosEditorPopup::init(GJGameLevel* level) {
    if (!Popup::init(400.f, 270.f)) return false;
    m_level = level;
    m_level->retain();
    m_data = storage::load(level);
    this->setTitle(fmt::format("Start Positions: {}", std::string(level->m_levelName)));

    auto size = m_mainLayer->getContentSize();
    auto top = CCMenu::create();
    top->setPosition({0, 0});
    top->setContentSize(size);
    m_mainLayer->addChild(top);

    auto editSpr = ButtonSprite::create("Open Editor", "goldFont.fnt", "GJ_button_01.png", 0.8f);
    editSpr->setScale(0.8f);
    auto editBtn = CCMenuItemSpriteExtra::create(editSpr, this, menu_selector(StartPosEditorPopup::onOpenEditor));
    editBtn->setPosition({size.width / 2 - 50, size.height - 55});
    top->addChild(editBtn);

    auto resetSpr = ButtonSprite::create("Reset", "goldFont.fnt", "GJ_button_06.png", 0.8f);
    resetSpr->setScale(0.6f);
    auto resetBtn = CCMenuItemSpriteExtra::create(resetSpr, this, menu_selector(StartPosEditorPopup::onReset));
    resetBtn->setPosition({size.width / 2 + 95, size.height - 55});
    top->addChild(resetBtn);

    m_listMenu = CCMenu::create();
    m_listMenu->setPosition({0, 0});
    m_listMenu->setContentSize(size);
    m_mainLayer->addChild(m_listMenu);
    refreshList();
    return true;
}

void StartPosEditorPopup::onOpenEditor(CCObject*) {
    auto level = m_level;
    level->retain();
    this->onClose(nullptr);
    openStartPosEditor(level);
    level->release();
}

void StartPosEditorPopup::onReset(CCObject*) {
    geode::createQuickPopup("Reset", "Remove all your saved start positions for this level and go back to the level's <cy>original</c> ones?",
        "Cancel", "Reset", [this](FLAlertLayer*, bool yes) {
            if (!yes) return;
            storage::erase(m_level);
            m_data = LevelStartPosData{};
            m_page = 0;
            refreshList();
        });
}

void StartPosEditorPopup::onRemove(CCObject* sender) {
    int idx = sender->getTag();
    if (idx < 0 || idx >= (int)m_data.positions.size()) return;
    m_data.positions.erase(m_data.positions.begin() + idx);
    if (m_data.active == idx) m_data.active = -1;
    else if (m_data.active > idx) m_data.active--;
    int maxPage = std::max(0, ((int)m_data.positions.size() - 1) / PER_PAGE);
    m_page = std::min(m_page, maxPage);
    persist();
    refreshList();
}

void StartPosEditorPopup::onSelect(CCObject* sender) { m_data.active = sender->getTag(); persist(); refreshList(); }
void StartPosEditorPopup::onClearActive(CCObject*) { m_data.active = -1; persist(); refreshList(); }

void StartPosEditorPopup::onPage(CCObject* sender) {
    int maxPage = std::max(0, ((int)m_data.positions.size() - 1) / PER_PAGE);
    m_page = std::clamp(m_page + sender->getTag(), 0, maxPage);
    refreshList();
}

void StartPosEditorPopup::persist() { storage::save(m_level, m_data); }

void StartPosEditorPopup::refreshList() {
    m_listMenu->removeAllChildren();
    auto size = m_mainLayer->getContentSize();
    float top = size.height - 90;

    if (m_data.positions.empty()) {
        auto lbl = CCLabelBMFont::create(storage::has(m_level)
            ? "No start positions. Open the editor to add some!"
            : "Using the level's original start positions.\nOpen the editor to add, move or remove them.",
            "chatFont.fnt");
        lbl->setScale(0.7f);
        lbl->setAlignment(kCCTextAlignmentCenter);
        lbl->setPosition({size.width / 2, top - 40});
        m_listMenu->addChild(lbl);
    }

    int start = m_page * PER_PAGE;
    for (int i = start; i < std::min(start + PER_PAGE, (int)m_data.positions.size()); i++) {
        auto& sp = m_data.positions[i];
        float y = top - (i - start) * 22.f;
        auto text = fmt::format("{}{}. x {:.0f}  {} {}{}{}",
            i == m_data.active ? "> " : "", i + 1, sp.x,
            storage::gamemodeName(sp.gamemode), storage::speedName(sp.speed),
            sp.mini ? " mini" : "", sp.flipped ? " flipped" : "");
        auto lbl = CCLabelBMFont::create(text.c_str(), "chatFont.fnt");
        lbl->setAnchorPoint({0, 0.5f});
        lbl->setScale(0.65f);
        lbl->setColor(i == m_data.active ? ccColor3B{0, 255, 0} : ccColor3B{255, 255, 255});
        lbl->setPosition({20, y});
        m_listMenu->addChild(lbl);

        auto useSpr = ButtonSprite::create("Use", "goldFont.fnt", "GJ_button_02.png", 0.6f);
        useSpr->setScale(0.45f);
        auto useBtn = CCMenuItemSpriteExtra::create(useSpr, this, menu_selector(StartPosEditorPopup::onSelect));
        useBtn->setTag(i);
        useBtn->setPosition({size.width - 75, y});
        m_listMenu->addChild(useBtn);

        auto delSpr = CCSprite::createWithSpriteFrameName("GJ_deleteIcon_001.png");
        delSpr->setScale(0.55f);
        auto delBtn = CCMenuItemSpriteExtra::create(delSpr, this, menu_selector(StartPosEditorPopup::onRemove));
        delBtn->setTag(i);
        delBtn->setPosition({size.width - 35, y});
        m_listMenu->addChild(delBtn);
    }

    if (!m_data.positions.empty()) {
        auto noneSpr = ButtonSprite::create("Play from start", "goldFont.fnt", "GJ_button_05.png", 0.6f);
        noneSpr->setScale(0.5f);
        auto noneBtn = CCMenuItemSpriteExtra::create(noneSpr, this, menu_selector(StartPosEditorPopup::onClearActive));
        noneBtn->setPosition({size.width / 2, 22});
        m_listMenu->addChild(noneBtn);
    }

    if (m_data.positions.size() > PER_PAGE) {
        auto prev = CCMenuItemSpriteExtra::create(CCSprite::createWithSpriteFrameName("GJ_arrow_03_001.png"),
            this, menu_selector(StartPosEditorPopup::onPage));
        prev->setTag(-1); prev->setScale(0.5f); prev->setPosition({30, 22});
        m_listMenu->addChild(prev);
        auto nextSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_03_001.png");
        nextSpr->setFlipX(true);
        auto next = CCMenuItemSpriteExtra::create(nextSpr, this, menu_selector(StartPosEditorPopup::onPage));
        next->setTag(1); next->setScale(0.5f); next->setPosition({size.width - 30, 22});
        m_listMenu->addChild(next);
    }
}

class StartPosButtonTarget : public CCNode {
public:
    GJGameLevel* m_level = nullptr;
    void onOpen(CCObject*) {
        if (auto p = StartPosEditorPopup::create(m_level)) p->show();
    }
};

CCMenuItemSpriteExtra* makeStartPosButton(CCNode* parent, GJGameLevel* level) {
    auto target = new StartPosButtonTarget();
    target->autorelease();
    target->m_level = level;
    auto spr = CircleButtonSprite::createWithSpriteFrameName("edit_eStartPosBtn_001.png", 1.0f,
        CircleBaseColor::Green, CircleBaseSize::Medium);
    if (!spr) {
        spr = CircleButtonSprite::create(CCLabelBMFont::create("SP", "bigFont.fnt"),
            CircleBaseColor::Green, CircleBaseSize::Medium);
    }
    auto btn = CCMenuItemSpriteExtra::create(spr, target, menu_selector(StartPosButtonTarget::onOpen));
    btn->setID("startpos-editor-button"_spr);
    btn->setUserObject(target);
    return btn;
}
