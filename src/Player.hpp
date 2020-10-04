#pragma once
#include "Tako.hpp"
#include <optional>

struct Player
{
    tako::Vector2 facing;
    std::optional<tako::Entity> heldObject;
};
