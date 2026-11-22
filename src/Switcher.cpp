#include "Switcher.hpp"
#include <Geode/modify/UILayer.hpp>
#include <Geode/loader/SettingV3.hpp>
#include <fmt/format.h>
#include <algorithm>
#include <cmath>

using namespace geode::prelude;

static constexpr int FADE_TAG = 0x53504E; // 'SPN'

bool switcher::active() {
    if (Loader::get()->getLoadedMod("capeling.startpos_switcher")) return false;
    return Mod::get()->getSettingValue<bool>("enable-switcher");
}

void switcher::sortByX(std::vector<StartPosObject*>& list) {
    std::sort(list.begin(), list.end(), [](StartPosObject* a, StartPosObject* b) {
        return a->getPositionX() < b->getPositionX();
    });
}

static GLubyte idleOpacityByte() {
    auto pct = std::clamp(static_cast<int>(Mod::get()->getSettingValue<int64_t>("switcher-opacity")), 0, 100);
    return static_cast<GLubyte>(std::lround(pct * 255.0 / 100.0));
}

static CCNode* switcherMenu() {
    auto play = PlayLayer::get();
    if (!play || !play->m_uiLayer) return nullptr;
    return play->m_uiLayer->getChildByID("switcher-menu"_spr);
}

void switcher::refreshUI() {
    auto menu = typeinfo_cast<CCMenu*>(switcherMenu());
    if (!menu) return;

    int index = 0, count = 0;
    view(index, count);

    auto prev = menu->getChildByID("switcher-prev"_spr);
    auto next = menu->getChildByID("switcher-next"_spr);
    auto label = typeinfo_cast<CCLabelBMFont*>(menu->getChildByID("switcher-label"_spr));

    bool show = active() && count > 0;
    menu->setVisible(show);
    if (!show) {
        menu->stopActionByTag(FADE_TAG);
        return;
    }

    bool hideButtons = Mod::get()->getSettingValue<bool>("hide-switcher-buttons");
    if (prev) prev->setVisible(!hideButtons);
    if (next) next->setVisible(!hideButtons);
    if (label) {
        label->setString(fmt::format("{}/{}", index, count).c_str());
        label->limitLabelWidth(40.f, 0.6f, 0.1f);
    }
    if (auto layout = typeinfo_cast<AxisLayout*>(menu->getLayout())) layout->ignoreInvisibleChildren(true);
    menu->updateLayout();

    menu->stopActionByTag(FADE_TAG);
    menu->setCascadeOpacityEnabled(true);
    menu->setOpacity(255);
    auto fade = CCSequence::create(
        CCDelayTime::create(0.5f),
        CCFadeTo::create(0.3f, idleOpacityByte()),
        nullptr
    );
    fade->setTag(FADE_TAG);
    menu->runAction(fade);
}

class $modify(SPUILayer, UILayer) {
    bool init(GJBaseGameLayer* layer) {
        if (!UILayer::init(layer)) return false;
        if (!typeinfo_cast<PlayLayer*>(layer)) return true;

        auto win = CCDirector::get()->getWinSize();
        auto menu = CCMenu::create();
        menu->setID("switcher-menu"_spr);
        menu->setPosition({win.width / 2.f, 22.f});
        menu->setVisible(false);
        menu->setCascadeOpacityEnabled(true);

        auto prevSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_02_001.png");
        prevSpr->setScale(0.6f);
        auto prev = CCMenuItemExt::createSpriteExtra(prevSpr, [](auto) { switcher::cycle(-1); });
        prev->setID("switcher-prev"_spr);

        auto label = CCLabelBMFont::create("0/0", "bigFont.fnt");
        label->setID("switcher-label"_spr);
        label->setScale(0.6f);

        auto nextSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_02_001.png");
        nextSpr->setScale(0.6f);
        nextSpr->setFlipX(true);
        auto next = CCMenuItemExt::createSpriteExtra(nextSpr, [](auto) { switcher::cycle(1); });
        next->setID("switcher-next"_spr);

        menu->addChild(prev);
        menu->addChild(label);
        menu->addChild(next);
        menu->setLayout(AxisLayout::create()->setAutoScale(false)->setGap(10.f)->setAxisAlignment(AxisAlignment::Center));
        this->addChild(menu, 100);
        return true;
    }
};

$on_mod(Loaded) {
    listenForKeybindSettingPresses("previous-startpos", [](Keybind const&, bool down, bool repeat, double) {
        if (down && !repeat) switcher::cycle(-1);
    });
    listenForKeybindSettingPresses("next-startpos", [](Keybind const&, bool down, bool repeat, double) {
        if (down && !repeat) switcher::cycle(1);
    });
    listenForSettingChanges<bool>("enable-switcher", [](bool) { switcher::refreshUI(); });
    listenForSettingChanges<bool>("hide-switcher-buttons", [](bool) { switcher::refreshUI(); });
    listenForSettingChanges<int64_t>("switcher-opacity", [](int64_t) { switcher::refreshUI(); });
}
