#pragma once
#include "Tako.hpp"
#include "Position.hpp"
#include "Rect.hpp"
#include "World.hpp"

struct RigidBody
{
    tako::Vector2 size;
    tako::Entity entity;
};

namespace Physics
{
    void Move(tako::World& world, Position& pos, RigidBody& rigid, tako::Vector2 movement)
    {
        while ((tako::mathf::abs(movement.x) > 0.0000001f || tako::mathf::abs(movement.y) > 0.0000001f))
        {

            auto mov = movement;
            if (movement.magnitude() > 1)
            {
                mov.normalize();
            }
            Rect newPos = {pos.AsVec() + mov, rigid.size};
            auto bump = false;
            for (auto [otherPos, otherRigid] : world.Iter<Position, RigidBody>())
            {
                if (&rigid == &otherRigid)
                {
                    continue;
                }

                Rect other = {otherPos.AsVec(), otherRigid.size};
                if (Rect::Overlap(newPos, other))
                {
                    if (Rect::Overlap({{pos.x + mov.x, pos.y}, rigid.size}, other))
                    {
                        movement.x -= mov.x / 2;
                    }
                    if (Rect::Overlap({{pos.x, pos.y + mov.y}, rigid.size}, other))
                    {
                        movement.y -= mov.y / 2;
                    }

                    movement -= mov / 2;
                    bump = true;
                    break;
                }
            }
            if (bump) continue;
            movement -= mov;
            pos += mov;
        }
    }
}