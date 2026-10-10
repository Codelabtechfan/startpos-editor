#include "Storage.hpp"
#include <Geode/binding/LocalLevelManager.hpp>
#include <fmt/format.h>

using namespace geode::prelude;

static bool isMainLevel(GJGameLevel* level) {
    return level->m_levelType == GJLevelType::Main;
}

std::string storage::keyFor(GJGameLevel* level) {
    if (isMainLevel(level)) return fmt::format("main-{}", level->m_levelID.value());
    if (level->m_levelType == GJLevelType::Editor)
        return fmt::format("editor-{}-{}", std::string(level->m_levelName), level->m_M_ID);
    return fmt::format("online-{}", level->m_levelID.value());
}

LevelStartPosData storage::load(GJGameLevel* level) {
    LevelStartPosData out;
    auto root = Mod::get()->getSavedValue<matjson::Value>("levels");
    if (!root.isObject() || !root.contains(keyFor(level))) return out;
    auto entry = root[keyFor(level)];
    out.active = (int)entry["active"].asInt().unwrapOr(-1);
    if (entry["positions"].isArray()) {
        for (auto& p : entry["positions"]) {
            SavedStartPos sp;
            sp.x = (float)p["x"].asDouble().unwrapOr(0);
            sp.y = (float)p["y"].asDouble().unwrapOr(105);
            sp.gamemode = (int)p["mode"].asInt().unwrapOr(0);
            sp.speed = (int)p["speed"].asInt().unwrapOr(0);
            sp.mini = p["mini"].asBool().unwrapOr(false);
            sp.flipped = p["flip"].asBool().unwrapOr(false);
            out.positions.push_back(sp);
        }
    }
    if (out.active >= (int)out.positions.size()) out.active = -1;
    return out;
}

void storage::save(GJGameLevel* level, LevelStartPosData const& data) {
    auto root = Mod::get()->getSavedValue<matjson::Value>("levels");
    if (!root.isObject()) root = matjson::Value::object();
    auto arr = matjson::Value::array();
    for (auto& sp : data.positions) {
        arr.push(matjson::makeObject({
            {"x", sp.x}, {"y", sp.y}, {"mode", sp.gamemode},
            {"speed", sp.speed}, {"mini", sp.mini}, {"flip", sp.flipped},
        }));
    }
    root[keyFor(level)] = matjson::makeObject({ {"active", data.active}, {"positions", arr} });
    Mod::get()->setSavedValue("levels", root);
    (void)Mod::get()->saveData(); // write to disk right away
}

std::string storage::objectString(SavedStartPos const& sp) {
    // object id 31 = start position. kA2 gamemode, kA3 mini, kA4 speed, kA11 flip gravity
    return fmt::format("1,31,2,{:.2f},3,{:.2f},kA2,{},kA3,{},kA4,{},kA11,{}",
        sp.x, sp.y, sp.gamemode, sp.mini ? 1 : 0, sp.speed, sp.flipped ? 1 : 0);
}

float storage::estimateLevelLength(GJGameLevel* level) {
    std::string raw;
    if (isMainLevel(level)) raw = LocalLevelManager::sharedState()->getMainLevelString(level->m_levelID.value());
    else raw = level->m_levelString;
    if (raw.empty()) return 30000.f;
    std::string str = raw;
    if (raw.find(';') == std::string::npos) {
        str = ZipUtils::decompressString(raw, false, 0);
    }
    float maxX = 0.f;
    size_t pos = str.find(';'); // skip level header
    while (pos != std::string::npos) {
        size_t next = str.find(';', pos + 1);
        auto obj = str.substr(pos + 1, next == std::string::npos ? std::string::npos : next - pos - 1);
        auto k = obj.find(",2,");
        if (k != std::string::npos) {
            maxX = std::max(maxX, std::strtof(obj.c_str() + k + 3, nullptr));
        }
        pos = next;
    }
    return maxX > 100.f ? maxX : 30000.f;
}

char const* storage::gamemodeName(int m) {
    static char const* n[] = {"Cube","Ship","Ball","UFO","Wave","Robot","Spider","Swing"};
    return (m >= 0 && m < 8) ? n[m] : "Cube";
}
char const* storage::speedName(int s) {
    static char const* n[] = {"1x","0.5x","2x","3x","4x"};
    return (s >= 0 && s < 5) ? n[s] : "1x";
}
