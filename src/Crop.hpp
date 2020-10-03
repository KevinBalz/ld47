#pragma once
#include <array>

constexpr auto TOTAL_DAYS = 7;

struct Crop
{
    int stage;
    bool watered;
    std::array<int, TOTAL_DAYS> stageHistory;
};

