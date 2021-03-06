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
struct TransportBox {};

struct Parsnip
{
    int harvestDay;
};

struct Interactable
{
    float w;
    float h;
};
