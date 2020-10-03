#pragma once
#include "Tako.hpp"
#include "World.hpp"
#include "Font.hpp"
#include "Position.hpp"
#include "Renderer.hpp"
#include "Player.hpp"

struct Text
{
    tako::Texture* texture;
    tako::Vector2 size;
};

struct Camera {};

Text CreateText(tako::PixelArtDrawer* drawer, tako::Font* font, std::string_view text)
{
    auto bitmap = font->RenderText(text, 1);
    auto texture = drawer->CreateTexture(bitmap);
    return
    {
        texture,
        tako::Vector2(bitmap.Width(), bitmap.Height())
    };
}


class Game
{
public:
    void Setup(tako::PixelArtDrawer* drawer)
    {
        m_drawer = drawer;
        drawer->SetTargetSize(240, 135);
        drawer->AutoScale();
        InitGame();
    }

    void InitGame()
    {
        {
            auto player = m_world.Create<Position, RectangleRenderer, Player, Foreground, Camera>();
            Position& pos = m_world.GetComponent<Position>(player);
            pos.x = 0;
            pos.y = 0;
            RectangleRenderer& renderer = m_world.GetComponent<RectangleRenderer>(player);
            renderer.size = { 16, 16};
            renderer.color = {0, 0, 0, 255};
        }
        {
            auto asdf = m_world.Create<Position, RectangleRenderer, Background>();
            Position& pos = m_world.GetComponent<Position>(asdf);
            pos.x = 0;
            pos.y = 0;
            RectangleRenderer& renderer = m_world.GetComponent<RectangleRenderer>(asdf);
            renderer.size = { 16, 16};
            renderer.color = {128, 0, 0, 255};
        }
    }


    void Update(tako::Input* input, float dt)
    {
        m_world.IterateComps<Position, Player>([&](Position& pos, Player& player)
        {
            tako::Vector2 moveVector;
            if (input->GetKey(tako::Key::Left) || input->GetKey(tako::Key::A) || input->GetKey(tako::Key::Gamepad_Dpad_Left))
            {
                moveVector.x -= 1;
            }
            if (input->GetKey(tako::Key::Right) || input->GetKey(tako::Key::D) || input->GetKey(tako::Key::Gamepad_Dpad_Right))
            {
                moveVector.x += 1;
            }
            if (input->GetKey(tako::Key::Up) || input->GetKey(tako::Key::W) || input->GetKey(tako::Key::Gamepad_Dpad_Up))
            {
                moveVector.y += 1;
            }
            if (input->GetKey(tako::Key::Down) || input->GetKey(tako::Key::S) || input->GetKey(tako::Key::Gamepad_Dpad_Down))
            {
                moveVector.y -= 1;
            }
            //moveVector.normalize();
            pos += moveVector * dt * 40;
        });
    }


    void Draw(tako::PixelArtDrawer* drawer)
    {
        drawer->SetClearColor({255, 255, 255, 255});
        drawer->Clear();
        m_world.IterateComps<Position, Camera>([&](Position& pos, Camera& player)
        {
            drawer->SetCameraPosition(pos.AsVec());
        });

        m_world.IterateComps<Position, RectangleRenderer, Background>([&](Position& pos, RectangleRenderer& rect, Background& b)
        {
          drawer->DrawRectangle(pos.x - rect.size.x / 2, pos.y + rect.size.y / 2, rect.size.x, rect.size.y,  rect.color);
        });
        m_world.IterateComps<Position, RectangleRenderer, Foreground>([&](Position& pos, RectangleRenderer& rect, Foreground& f)
        {
          drawer->DrawRectangle(pos.x - rect.size.x / 2, pos.y + rect.size.y / 2, rect.size.x, rect.size.y,  rect.color);
        });
        m_world.IterateComps<Position, SpriteRenderer, Background>([&](Position& pos, SpriteRenderer& sprite, Background& b)
        {
           drawer->DrawSprite(pos.x - sprite.size.x / 2, pos.y + sprite.size.y / 2, sprite.size.x, sprite.size.y, sprite.sprite);
        });
        m_world.IterateComps<Position, SpriteRenderer, Foreground>([&](Position& pos, SpriteRenderer& sprite, Foreground& f)
        {
           drawer->DrawSprite(pos.x - sprite.size.x / 2, pos.y + sprite.size.y / 2, sprite.size.x, sprite.size.y, sprite.sprite);
        });
    }
private:
    tako::World m_world;
    tako::PixelArtDrawer* m_drawer;
};