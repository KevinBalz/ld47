#pragma once
#include <array>

constexpr auto TOTAL_DAYS = 7;

struct Crop
{
    int stage;
    bool watered;
    int tileX;
    int tileY;
    std::array<int, TOTAL_DAYS + 1> stageHistory;
};

