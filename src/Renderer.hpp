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
    tako::Vector2 offset;
};

struct AnimatedSprite
{
    float duration;
    float passed;
    int frames;
    tako::Sprite** sprites;

    void SetStatic(tako::Sprite** sprite)
    {
        passed = 9999;
        duration = 9999;
        frames = 1;
        sprites = sprite;
    }

    void SetAnim(float duration, tako::Sprite** sprites, int frames)
    {
        this->duration = duration;
        passed = duration;
        this->sprites = sprites;
        this->frames = frames;
    }
};

struct Background {};
struct Foreground {};