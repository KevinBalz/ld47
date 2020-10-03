#pragma once
#include "Tako.hpp"

struct RectangleRenderer
{
    tako::Vector2 size;
    tako::Color color;
};

struct SpriteRenderer
{
    tako::Vector2 size;
    tako::Sprite* sprite;
};

struct AnimatedSprite
{
    tako::Vector2 size;
    tako::Sprite* sprites;
};

struct Background {};
struct Foreground {};