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
    LevelStartPosData load(GJGameLevel* level);
    void save(GJGameLevel* level, LevelStartPosData const& data);
    std::string objectString(SavedStartPos const& sp);
    float estimateLevelLength(GJGameLevel* level);
    char const* gamemodeName(int mode);
    char const* speedName(int speed);
}
