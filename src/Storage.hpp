#pragma once
#include <Geode/Geode.hpp>
#include <vector>
#include <string>

struct SavedStartPos {
    float x = 0.f;
    float y = 105.f;
    int gamemode = 0;   // 0 cube,1 ship,2 ball,3 ufo,4 wave,5 robot,6 spider,7 swing
    int speed = 0;      // 0 = 1x, 1 = 0.5x, 2 = 2x, 3 = 3x, 4 = 4x
    bool mini = false;
    bool flipped = false;
};

struct LevelStartPosData {
    std::vector<SavedStartPos> positions;
    int active = -1; // index of the start pos used when playing, -1 = start from beginning
};

namespace storage {
    std::string keyFor(GJGameLevel* level);
    bool has(GJGameLevel* level);
    void erase(GJGameLevel* level);
    LevelStartPosData load(GJGameLevel* level);
    void save(GJGameLevel* level, LevelStartPosData const& data);
    std::string objectString(SavedStartPos const& sp);
    // Raw (decompressed) object string of a level, main levels included.
    std::string rawLevelString(GJGameLevel* level);
    // Removes every start position (object id 31) from a decompressed level string.
    std::string stripStartPositions(std::string const& str);
    bool isStartPosObject(std::string const& obj);
    // Level string as the player/editor should see it: original start positions are
    // replaced by our saved ones (all of them if `all`, else only the active one).
    std::string applySaved(std::string const& raw, LevelStartPosData const& data, bool all);
    char const* gamemodeName(int mode);
    char const* speedName(int speed);
}
