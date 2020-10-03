#pragma once
#include "Math.hpp"

struct Rect
{
    float x, y, w, h;

    constexpr Rect() : x(0), y(0), w(0), h(0) {}
    constexpr Rect(float x, float y , float w, float h) : x(x), y(y), w(w), h(h) {}
    constexpr Rect(tako::Vector2 pos, tako::Vector2 size) : x(pos.x), y(pos.y), w(size.x), h(size.y) {}

    tako::Vector2 Position()
    {
        return {x, y};
    }

    void Position(tako::Vector2 p)
    {
        x = p.x;
        y = p.y;
    }

    float Left()
    {
        return x - w / 2;
    }

    float Right()
    {
        return x + w / 2;
    }

    float Top()
    {
        return y + h / 2;
    }

    float Bottom()
    {
        return y - h / 2;
    }

    static bool Overlap(Rect a, Rect b)
    {
        return OverlapX(a, b) && OverlapY(a, b);
    }

    static bool OverlapX(Rect a, Rect b)
    {
        return std::abs(a.x - b.x) < a.w / 2 + b.w / 2;
    }

    static bool OverlapY(Rect a, Rect b)
    {
        return std::abs(a.y - b.y) < a.h / 2 + b.h / 2;
    }
};
