#pragma once
#include "Tako.hpp"
#include "World.hpp"
#include "Font.hpp"
#include "Position.hpp"
#include "Renderer.hpp"
#include "Player.hpp"
#include "Physics.hpp"
#include "Crop.hpp"
#include "Level.hpp"

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
    void Setup(tako::PixelArtDrawer* drawer, tako::Resources* resources)
    {
        m_drawer = drawer;
        drawer->SetTargetSize(240, 135);
        drawer->AutoScale();

        m_font = new tako::Font("/charmap-cellphone.png", 5, 7, 1, 1, 2, 2,
                                " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]\a_`abcdefghijklmnopqrstuvwxyz{|}~");

        m_groundTexture = resources->Load<tako::Texture>("/Ground.png");
        m_tiledTexture = resources->Load<tako::Texture>("/Tiled.png");
        m_plantedTexture = resources->Load<tako::Texture>("/Planted.png");

        m_level.Init(drawer, resources);
        InitGame();
    }

    void InitGame()
    {
        m_currentDay = 0;

        std::map<char, std::function<void(int,int)>> levelCallbacks
        {{
            {'S', [&](int x, int y)
            {
                m_playerSpawn = tako::Vector2(x * 16 + 8, y * 16 + 8);
            }},
            {'C', [&](int x, int y)
            {
                CreateCrop(x, y);
            }},
        }};
        m_level.LoadLevel("/Level.txt", levelCallbacks);

        {
            auto player = m_world.Create<Position, RectangleRenderer, Player, RigidBody, Foreground, Camera>();
            Position& pos = m_world.GetComponent<Position>(player);
            pos = m_playerSpawn;
            RigidBody& rigid = m_world.GetComponent<RigidBody>(player);
            rigid.size = { 16, 16 };
            rigid.entity = player;
            RectangleRenderer& renderer = m_world.GetComponent<RectangleRenderer>(player);
            renderer.size = { 16, 16};
            renderer.color = {0, 0, 0, 255};
            Player& play = m_world.GetComponent<Player>(player);
            play.facing = { 0, -1 };
        }
        m_currentDay = 1;
        m_currentDayText = CreateText(m_drawer, m_font, "Day " + std::to_string(m_currentDay));
        m_dayTimeLeft = DAY_LENGTH;
        m_dayTimeLeftText = CreateText(m_drawer, m_font, std::to_string(m_dayTimeLeft));
    }

    tako::Entity CreateCrop(int x, int y)
    {
        auto crop = m_world.Create<Position, Crop, Background>();
        Position& pos = m_world.GetComponent<Position>(crop);
        pos.x = x * 16 + 8;
        pos.y = y * 16 + 8;
        Crop& cr = m_world.GetComponent<Crop>(crop);
        cr.stage = 1;
        cr.watered = false;
        cr.stageHistory[m_currentDay] = cr.stage;
        for (int i = 0; i < m_currentDay; i++)
        {
            cr.stageHistory[i] = 0;
        }

        cr.tileX = x;
        cr.tileY = y;
        m_level.GetTile(x, y).value()->index = 3;

        return crop;
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
            if (moveVector.magnitude() > 1)
            {
                moveVector.normalize();
            }
            else if (moveVector.magnitude() > 0)
            {
                player.facing = tako::Vector2::Normalized(moveVector);
            }
            Physics::Move(m_world, pos, rigid, moveVector * dt * 20);
            if (input->GetKeyDown(tako::Key::L))
            {
                for (auto [cPos, crop] : m_world.Iter<Position, Crop>())
                {
                    if (crop.watered)
                    {
                        continue;
                    }

                    if (tako::mathf::abs((cPos.AsVec() - pos.AsVec()).magnitude()) < 20)
                    {
                        crop.watered = true;
                        break;
                    }
                }
                int tileX = ((int) pos.x + player.facing.x * 12) / 16;
                int tileY = ((int) pos.y + player.facing.y * 12) / 16;
                auto tileOpt = m_level.GetTile(tileX, tileY);
                if (tileOpt)
                {
                    auto tile = tileOpt.value();
                    if (tile->index == 1)
                    {
                        //tile->index = 2;
                        CreateCrop(tileX, tileY);
                    }
                    else
                    {
                        m_world.IterateComps<Crop>([&](Crop& crop)
                        {
                            if (crop.tileX != tileX || crop.tileY != tileY)
                            {
                                return;
                            }
                            crop.watered = true;
                            tile->index = 4;
                        });
                    }
                }
            }
        });


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
        m_world.IterateComps<Crop>([&](Crop& crop)
        {
            if (!crop.watered)
            {
                allWatered = false;
            }
        });
        m_world.IterateHandle<Crop>([&](tako::EntityHandle handle)
        {
            Crop& crop = m_world.GetComponent<Crop>(handle.id);
            if (allWatered)
            {
                crop.stage++;
                crop.stageHistory[m_currentDay] = crop.stage;
            }
            else
            {
                crop.stage = crop.stageHistory[m_currentDay - 1];
            }
            crop.watered = false;
        });
        if (allWatered)
        {
            m_currentDay++;
            RerenderText(m_currentDayText, m_drawer, m_font, "Day " + std::to_string(m_currentDay));
        }
        else
        {
            std::vector<tako::Entity> clearCrop;
            m_world.IterateHandle<Crop>([&](tako::EntityHandle handle)
            {
                Crop& crop = m_world.GetComponent<Crop>(handle.id);
                if (crop.stage == 0)
                {
                    clearCrop.push_back(handle.id);
                    m_level.GetTile(crop.tileX, crop.tileY).value()->index = 1;
                }
            });
            for (auto ent : clearCrop)
            {
                m_world.Delete(ent);
            }
        }
        m_level.ResetWatered();
        for (auto [pos, player]: m_world.Iter<Position, Player>())
        {
            pos = m_playerSpawn;
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
        //Render map
        /*
        for (int x = -16; x < 16; x++)
        {
            for (int y = -16; y < 16; y++)
            {
                drawer->DrawImage(x * 16, y * 16, 16, 16, y % 3 == 0 ? m_plantedTexture: m_tiledTexture);
            }
        }
         */
        m_level.Draw(drawer);

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
    tako::Vector2 m_playerSpawn = {0, 0};
    tako::Font* m_font;
    tako::World m_world;
    Level m_level;
    tako::Texture* m_groundTexture;
    tako::Texture* m_tiledTexture;
    tako::Texture* m_plantedTexture;
    tako::PixelArtDrawer* m_drawer;
};