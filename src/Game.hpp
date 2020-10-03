#pragma once
#include "Tako.hpp"
#include "World.hpp"
#include "Font.hpp"
#include "Position.hpp"
#include "Renderer.hpp"
#include "Player.hpp"
#include "Physics.hpp"
#include "Crop.hpp"

constexpr auto DAY_LENGTH = 10.0f;

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

void RerenderText(Text& tex, tako::PixelArtDrawer* drawer, tako::Font* font, std::string_view text)
{
    auto bitmap = font->RenderText(text, 1);
    drawer->UpdateTexture(tex.texture, bitmap);
    tex.size = tako::Vector2(bitmap.Width(), bitmap.Height());
}


class Game
{
public:
    void Setup(tako::PixelArtDrawer* drawer)
    {
        m_drawer = drawer;
        drawer->SetTargetSize(240, 135);
        drawer->AutoScale();

        m_font = new tako::Font("/charmap-cellphone.png", 5, 7, 1, 1, 2, 2,
                                " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]\a_`abcdefghijklmnopqrstuvwxyz{|}~");

        InitGame();
    }

    void InitGame()
    {
        m_currentDay = 1;
        m_currentDayText = CreateText(m_drawer, m_font, "Day " + std::to_string(m_currentDay));
        m_dayTimeLeft = DAY_LENGTH;
        m_dayTimeLeftText = CreateText(m_drawer, m_font, std::to_string(m_dayTimeLeft));
        {
            auto player = m_world.Create<Position, RectangleRenderer, Player, RigidBody, Foreground, Camera>();
            Position& pos = m_world.GetComponent<Position>(player);
            pos.x = 0;
            pos.y = 0;
            RigidBody& rigid = m_world.GetComponent<RigidBody>(player);
            rigid.size = { 16, 16 };
            rigid.entity = player;
            RectangleRenderer& renderer = m_world.GetComponent<RectangleRenderer>(player);
            renderer.size = { 16, 16};
            renderer.color = {0, 0, 0, 255};
        }
        CreateCrop(32, 32);
        CreateCrop(48, 32);
    }

    void CreateCrop(int x, int y)
    {
        auto crop = m_world.Create<Position, RectangleRenderer, Crop, RigidBody, Background>();
        Position& pos = m_world.GetComponent<Position>(crop);
        pos.x = x;
        pos.y = y;
        Crop& cr = m_world.GetComponent<Crop>(crop);
        cr.stage = 1;
        cr.watered = false;
        cr.stageHistory[m_currentDay - 1] = cr.stage;
        RigidBody& rigid = m_world.GetComponent<RigidBody>(crop);
        rigid.size = { 16, 16 };
        rigid.entity = crop;
        RectangleRenderer& renderer = m_world.GetComponent<RectangleRenderer>(crop);
        renderer.size = { 16, 16};
        renderer.color = {128, 0, 0, 255};
    }


    void Update(tako::Input* input, float dt)
    {
        m_world.IterateComps<Position, Player, RigidBody>([&](Position& pos, Player& player, RigidBody& rigid)
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
            //pos += moveVector * dt * 40;
            Physics::Move(m_world, pos, rigid, moveVector * dt * 20);
            if (input->GetKeyDown(tako::Key::L))
            {
                for (auto [cPos, crop] : m_world.Iter<Position, Crop>())
                {
                    LOG("{} {} {}", (cPos.AsVec() - pos.AsVec()).x, (cPos.AsVec() - pos.AsVec()).y, (cPos.AsVec() - pos.AsVec()).magnitude());
                    if (tako::mathf::abs((cPos.AsVec() - pos.AsVec()).magnitude()) < 20)
                    {
                        crop.watered = true;
                    }
                }
            }
        });



        for (auto [renderer, crop] : m_world.Iter<RectangleRenderer, Crop>())
        {
            renderer.color = crop.watered ? tako::Color(0, 0, 255, 255) : tako::Color(255, 0, 0, 255);
            renderer.size = tako::Vector2(crop.stage * 4, crop.stage * 4);
        }

        m_dayTimeLeft -= dt;
        if (m_dayTimeLeft <= 0)
        {
            PassDay();
        }
        RerenderText(m_dayTimeLeftText, m_drawer, m_font, std::to_string(m_dayTimeLeft));
    }

    void PassDay()
    {
        if (m_currentDay == TOTAL_DAYS)
        {
            return;
        }
        bool allWatered = true;
        for (auto [crop] : m_world.Iter<Crop>())
        {
            if (!crop.watered)
            {
                allWatered = false;
                break;
            }
        }
        m_world.IterateHandle<Crop>([&](tako::EntityHandle handle)
        {
            Crop& crop = m_world.GetComponent<Crop>(handle.id);
            if (allWatered)
            {
                crop.stage++;
                crop.stageHistory[m_currentDay] = crop.stage;
            }
            crop.watered = false;
        });
        if (allWatered)
        {
            m_currentDay++;
            RerenderText(m_currentDayText, m_drawer, m_font, "Day " + std::to_string(m_currentDay));

        }
        m_dayTimeLeft = DAY_LENGTH;
    }


    void Draw(tako::PixelArtDrawer* drawer)
    {
        auto cameraSize = drawer->GetCameraViewSize();
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

        if (m_currentDay > 0)
        {
            drawer->SetCameraPosition(cameraSize / 2);
            drawer->DrawImage(4, cameraSize.y - 4, m_currentDayText.size.x, m_currentDayText.size.y, m_currentDayText.texture, {0, 0, 0, 255});

            drawer->DrawImage(4, cameraSize.y - 16, m_dayTimeLeftText.size.x, m_dayTimeLeftText.size.y, m_dayTimeLeftText.texture, {0, 0, 0, 255});
        }
    }
private:
    int m_currentDay = 0;
    float m_dayTimeLeft;
    Text m_currentDayText;
    Text m_dayTimeLeftText;
    tako::Font* m_font;
    tako::World m_world;
    tako::PixelArtDrawer* m_drawer;
};