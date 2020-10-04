#pragma once
#include "Tako.hpp"
#include "Position.hpp"
#include "Rect.hpp"
#include "World.hpp"
#include "Level.hpp"

struct RigidBody
{
    tako::Vector2 size;
    tako::Entity entity;
};

namespace Physics
{
    void Move(tako::World& world, Level& level, Position& pos, RigidBody& rigid, tako::Vector2 movement)
    {
        while ((tako::mathf::abs(movement.x) > 0.0000001f || tako::mathf::abs(movement.y) > 0.0000001f))
        {

            auto mov = movement;
            if (movement.magnitude() > 1)
            {
                mov.normalize();
            }
            Rect newPos = {pos.AsVec() + mov, rigid.size};

            if (level.Overlap(newPos))
            {
                if (level.Overlap({{pos.x + mov.x, pos.y}, rigid.size}))
                {
                    movement.x -= mov.x / 2;
                }
                if (level.Overlap({{pos.x, pos.y + mov.y}, rigid.size}))
                {
                    movement.y -= mov.y / 2;
                }

                movement -= mov / 2;
                continue;
            }
            auto bump = false;
            world.IterateComps<Position, RigidBody>([&](Position& otherPos, RigidBody& otherRigid)
            {
                if (bump)
                {
                    return;
                }
                if (&rigid == &otherRigid)
                {
                    return;
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
                    return;
                }
            });
            if (bump) continue;
            movement -= mov;
            pos += mov;
        }
    }
}