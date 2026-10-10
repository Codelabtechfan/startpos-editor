#include "EditorPopup.hpp"
#include <Geode/ui/TextInput.hpp>
#include <fmt/format.h>

using namespace geode::prelude;

static constexpr int PER_PAGE = 5;

StartPosEditorPopup* StartPosEditorPopup::create(GJGameLevel* level) {
    auto ret = new StartPosEditorPopup();
    if (ret->init(level)) { ret->autorelease(); return ret; }
    delete ret;
    return nullptr;
}

bool StartPosEditorPopup::init(GJGameLevel* level) {
    if (!Popup::init(420.f, 290.f)) return false;
    m_level = level;
    m_level->retain();
    m_data = storage::load(level);
    m_length = storage::estimateLevelLength(level);
    this->setTitle(fmt::format("Start Positions: {}", std::string(level->m_levelName)));

    auto size = m_mainLayer->getContentSize();
    auto controls = CCMenu::create();
    controls->setPosition({0, 0});
    controls->setContentSize(size);
    m_mainLayer->addChild(controls);

    // --- position scrubber ---
    m_slider = Slider::create(this, menu_selector(StartPosEditorPopup::onSlider), 0.8f);
    m_slider->setPosition({size.width / 2, size.height - 60});
    m_slider->setValue(0.f);
    m_mainLayer->addChild(m_slider);

    m_percentLabel = CCLabelBMFont::create("0%", "bigFont.fnt");
    m_percentLabel->setScale(0.4f);
    m_percentLabel->setPosition({size.width / 2 - 60, size.height - 85});
    m_mainLayer->addChild(m_percentLabel);

    m_xInput = TextInput::create(90.f, "X pos");
    m_xInput->setCommonFilter(CommonFilter::Float);
    m_xInput->setScale(0.7f);
    m_xInput->setPosition({size.width / 2 + 50, size.height - 85});
    m_xInput->setCallback([this](std::string const& str) {
        m_draft.x = std::max(0.f, std::strtof(str.c_str(), nullptr));
        m_slider->setValue(std::clamp(m_draft.x / m_length, 0.f, 1.f));
        m_percentLabel->setString(fmt::format("{:.1f}%", m_draft.x / m_length * 100.f).c_str());
    });
    m_mainLayer->addChild(m_xInput);

    // --- gamemode / speed / mini / flip ---
    auto mkBtn = [&](char const* text, SEL_MenuHandler sel, CCPoint pos) {
        auto spr = ButtonSprite::create(text, "bigFont.fnt", "GJ_button_04.png", 0.6f);
        spr->setScale(0.6f);
        auto btn = CCMenuItemSpriteExtra::create(spr, this, sel);
        btn->setPosition(pos);
        controls->addChild(btn);
        return btn;
    };
    float row = size.height - 115;
    mkBtn("Mode", menu_selector(StartPosEditorPopup::onMode), {45, row});
    m_modeLabel = CCLabelBMFont::create("Cube", "goldFont.fnt");
    m_modeLabel->setScale(0.5f);
    m_modeLabel->setPosition({100, row});
    m_mainLayer->addChild(m_modeLabel);

    mkBtn("Speed", menu_selector(StartPosEditorPopup::onSpeed), {160, row});
    m_speedLabel = CCLabelBMFont::create("1x", "goldFont.fnt");
    m_speedLabel->setScale(0.5f);
    m_speedLabel->setPosition({210, row});
    m_mainLayer->addChild(m_speedLabel);

    m_miniToggle = CCMenuItemToggler::createWithStandardSprites(this, nullptr, 0.5f);
    m_miniToggle->setPosition({245, row});
    controls->addChild(m_miniToggle);
    auto miniLbl = CCLabelBMFont::create("Mini", "bigFont.fnt");
    miniLbl->setScale(0.3f); miniLbl->setPosition({272, row});
    m_mainLayer->addChild(miniLbl);

    m_flipToggle = CCMenuItemToggler::createWithStandardSprites(this, nullptr, 0.5f);
    m_flipToggle->setPosition({300, row});
    controls->addChild(m_flipToggle);
    auto flipLbl = CCLabelBMFont::create("Flip", "bigFont.fnt");
    flipLbl->setScale(0.3f); flipLbl->setPosition({325, row});
    m_mainLayer->addChild(flipLbl);

    auto addSpr = ButtonSprite::create("Add", "goldFont.fnt", "GJ_button_01.png", 0.8f);
    addSpr->setScale(0.7f);
    auto addBtn = CCMenuItemSpriteExtra::create(addSpr, this, menu_selector(StartPosEditorPopup::onAdd));
    addBtn->setPosition({375, row});
    controls->addChild(addBtn);

    // --- list of saved start positions ---
    m_listMenu = CCMenu::create();
    m_listMenu->setPosition({0, 0});
    m_listMenu->setContentSize(size);
    m_mainLayer->addChild(m_listMenu);

    refreshDraft();
    refreshList();
    return true;
}

void StartPosEditorPopup::onSlider(CCObject*) {
    m_draft.x = m_slider->getValue() * m_length;
    m_percentLabel->setString(fmt::format("{:.1f}%", m_slider->getValue() * 100.f).c_str());
    m_xInput->setString(fmt::format("{:.0f}", m_draft.x));
}

void StartPosEditorPopup::onMode(CCObject*) { m_draft.gamemode = (m_draft.gamemode + 1) % 8; refreshDraft(); }
void StartPosEditorPopup::onSpeed(CCObject*) { m_draft.speed = (m_draft.speed + 1) % 5; refreshDraft(); }

void StartPosEditorPopup::refreshDraft() {
    m_modeLabel->setString(storage::gamemodeName(m_draft.gamemode));
    m_speedLabel->setString(storage::speedName(m_draft.speed));
}

void StartPosEditorPopup::onAdd(CCObject*) {
    auto sp = m_draft;
    // toggler state is the state *before* the toggle animation finishes, read directly
    sp.mini = m_miniToggle->isToggled();
    sp.flipped = m_flipToggle->isToggled();
    m_data.positions.push_back(sp);
    std::sort(m_data.positions.begin(), m_data.positions.end(),
        [](auto& a, auto& b) { return a.x < b.x; });
    // make the newly added one the active start position
    for (size_t i = 0; i < m_data.positions.size(); i++) {
        if (m_data.positions[i].x == sp.x) { m_data.active = (int)i; m_page = (int)i / PER_PAGE; break; }
    }
    persist();
    refreshList();
    Notification::create("Start position added", NotificationIcon::Success)->show();
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

void StartPosEditorPopup::onSelect(CCObject* sender) {
    m_data.active = sender->getTag();
    persist();
    refreshList();
}

void StartPosEditorPopup::onClearActive(CCObject*) {
    m_data.active = -1;
    persist();
    refreshList();
}

void StartPosEditorPopup::onPage(CCObject* sender) {
    int maxPage = std::max(0, ((int)m_data.positions.size() - 1) / PER_PAGE);
    m_page = std::clamp(m_page + sender->getTag(), 0, maxPage);
    refreshList();
}

void StartPosEditorPopup::persist() { storage::save(m_level, m_data); }

void StartPosEditorPopup::refreshList() {
    m_listMenu->removeAllChildren();
    auto size = m_mainLayer->getContentSize();
    float top = size.height - 150;

    if (m_data.positions.empty()) {
        auto lbl = CCLabelBMFont::create("No start positions yet. Pick a spot and press Add!", "chatFont.fnt");
        lbl->setScale(0.7f);
        lbl->setPosition({size.width / 2, top - 40});
        m_listMenu->addChild(lbl);
    }

    int start = m_page * PER_PAGE;
    for (int i = start; i < std::min(start + PER_PAGE, (int)m_data.positions.size()); i++) {
        auto& sp = m_data.positions[i];
        float y = top - (i - start) * 22.f;
        auto text = fmt::format("{}{}. {:.1f}%  (x {:.0f})  {} {}{}{}",
            i == m_data.active ? "> " : "", i + 1, sp.x / m_length * 100.f, sp.x,
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

    auto noneSpr = ButtonSprite::create("Play from start", "goldFont.fnt", "GJ_button_05.png", 0.6f);
    noneSpr->setScale(0.5f);
    auto noneBtn = CCMenuItemSpriteExtra::create(noneSpr, this, menu_selector(StartPosEditorPopup::onClearActive));
    noneBtn->setPosition({size.width / 2, 22});
    m_listMenu->addChild(noneBtn);

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

// ---------- button used on LevelInfoLayer / main level info popup ----------
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
    btn->setUserObject(target); // keeps target alive as long as the button
    return btn;
}
