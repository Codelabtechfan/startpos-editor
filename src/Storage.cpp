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

bool storage::has(GJGameLevel* level) {
    auto root = Mod::get()->getSavedValue<matjson::Value>("levels");
    return root.isObject() && root.contains(keyFor(level));
}

void storage::erase(GJGameLevel* level) {
    auto root = Mod::get()->getSavedValue<matjson::Value>("levels");
    if (!root.isObject()) return;
    (void)root.erase(keyFor(level));
    Mod::get()->setSavedValue("levels", root);
    (void)Mod::get()->saveData();
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

std::string storage::rawLevelString(GJGameLevel* level) {
    std::string raw;
    if (isMainLevel(level)) raw = LocalLevelManager::sharedState()->getMainLevelString(level->m_levelID.value());
    else raw = level->m_levelString;
    if (!raw.empty() && raw.find(';') == std::string::npos) {
        raw = ZipUtils::decompressString(raw, false, 0);
    }
    return raw;
}

bool storage::isStartPosObject(std::string const& obj) {
    // objects are "key,value,key,value,..."; key 1 is the object id
    size_t i = 0;
    while (i < obj.size()) {
        size_t c1 = obj.find(',', i);
        if (c1 == std::string::npos) return false;
        size_t c2 = obj.find(',', c1 + 1);
        auto key = obj.substr(i, c1 - i);
        auto val = obj.substr(c1 + 1, c2 == std::string::npos ? std::string::npos : c2 - c1 - 1);
        if (key == "1") return val == "31";
        if (c2 == std::string::npos) return false;
        i = c2 + 1;
    }
    return false;
}

std::string storage::stripStartPositions(std::string const& str) {
    std::string out;
    out.reserve(str.size());
    size_t start = 0;
    bool first = true; // first chunk is the level header
    while (start < str.size()) {
        size_t end = str.find(';', start);
        auto chunk = str.substr(start, end == std::string::npos ? std::string::npos : end - start);
        if (first || !isStartPosObject(chunk)) {
            if (!chunk.empty() || first) { out += chunk; out += ';'; }
        }
        first = false;
        if (end == std::string::npos) break;
        start = end + 1;
    }
    return out;
}

std::string storage::applySaved(std::string const& raw, LevelStartPosData const& data, bool all) {
    auto str = stripStartPositions(raw);
    if (!str.empty() && str.back() != ';') str += ';';
    if (all) {
        for (auto& sp : data.positions) str += objectString(sp) + ";";
    } else if (data.active >= 0 && data.active < (int)data.positions.size()) {
        str += objectString(data.positions[data.active]) + ";";
    }
    return str;
}

char const* storage::gamemodeName(int m) {
    static char const* n[] = {"Cube","Ship","Ball","UFO","Wave","Robot","Spider","Swing"};
    return (m >= 0 && m < 8) ? n[m] : "Cube";
}
char const* storage::speedName(int s) {
    static char const* n[] = {"1x","0.5x","2x","3x","4x"};
    return (s >= 0 && s < 5) ? n[s] : "1x";
}
