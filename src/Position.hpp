#pragma once
#include "Tako.hpp"

struct Position
{
    float x;
    float y;

    tako::Vector2 AsVec()
    {
        return {x, y};
    }

    tako::Vector2 operator+=(const tako::Vector2& rhs)
    {
        x += rhs.x;
        y += rhs.y;
        return AsVec();
    }

    tako::Vector2 operator-=(const tako::Vector2& rhs)
    {
        x -= rhs.x;
        y -= rhs.y;
        return AsVec();
    }

    Position& operator=(const tako::Vector2& rhs)
    {
        x = rhs.x;
        y = rhs.y;
        return *this;
    }
};