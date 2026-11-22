#pragma once
#include <Geode/Geode.hpp>
#include <vector>

// Built-in start position switcher. Written for this mod; not a copy of any other switcher.
namespace switcher {
    // Setting is on, and Capeling's Startpos Switcher is not loaded.
    bool active();

    // delta -1 = previous, +1 = next. Wraps, including "from start" (index 0).
    void cycle(int delta);

    // After start positions have been collected and the "Use" choice applied, sort by X
    // and set the switcher index to match the active start position (0 = from start).
    void syncIndexFromActive();

    void sortByX(std::vector<StartPosObject*>& list);

    // Update the on-screen arrows/label and play the flash-then-fade animation.
    void refreshUI();

    // Used by the UI to read the current n/N without depending on PlayLayer's modify class.
    bool view(int& index, int& count);
}
