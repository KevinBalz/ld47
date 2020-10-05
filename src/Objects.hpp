#pragma once
#include "World.hpp"

struct Pickup
{
    int x;
    int y;
    tako::Entity entity;
};

struct WateringCan
{
    int left = 9;
};

struct SeedBag {};

struct Well {};

struct Parsnip {};

struct Interactable
{
    float w;
    float h;
};
