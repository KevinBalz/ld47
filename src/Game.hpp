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
#include "Objects.hpp"

constexpr auto DAY_LENGTH = 60.0f;

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

tako::Vector2 FitMapBound(Rect bounds, tako::Vector2 cameraPos, tako::Vector2 camSize)
{
    cameraPos.x = std::max(bounds.Left() + camSize.x / 2, cameraPos.x);
    cameraPos.x = std::min(bounds.Right() - camSize.x / 2, cameraPos.x);
    cameraPos.y = std::min(bounds.Top() - camSize.y / 2, cameraPos.y);
    cameraPos.y = std::max(bounds.Bottom() + camSize.y / 2, cameraPos.y);

    return cameraPos;
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

        m_waterCan = drawer->CreateSprite(resources->Load<tako::Texture>("/Watercan.png"), 0, 0, 16, 16);
        m_seedBag = drawer->CreateSprite(resources->Load<tako::Texture>("/SeedBag.png"), 0, 0, 16, 16);
        m_parsnip = drawer->CreateSprite(resources->Load<tako::Texture>("/Parsnip.png"), 0, 0, 16, 16);

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
            {'W', [&](int x, int y)
            {
                SpawnBuilding(x, y, BUILDING_INFO.at('W'), Well());
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
            play.heldObject = std::nullopt;
        }
        m_currentDay = 1;
        m_currentDayText = CreateText(m_drawer, m_font, "Day " + std::to_string(m_currentDay));
        m_dayTimeLeft = DAY_LENGTH;
        m_dayTimeLeftText = CreateText(m_drawer, m_font, std::to_string(m_dayTimeLeft));

        SpawnObject(4, 8, m_seedBag, SeedBag());
    }

    template<class T>
    tako::Entity SpawnBuilding(int x, int y, const BuildingSize& size, T type)
    {
        auto entity = m_world.Create<Position, Interactable, T>();
        Position& pos = m_world.GetComponent<Position>(entity);
        pos = tako::Vector2(x * 16 + size.x * 8, (y+1) * 16 - size.y * 8);
        Interactable& inter = m_world.GetComponent<Interactable>(entity);
        inter.w = size.x * 16;
        inter.h = size.y * 16;
        T& t = m_world.GetComponent<T>(entity);
        t = type;
        return entity;
    }

    tako::Entity CreateCrop(int x, int y)
    {
        auto crop = m_world.Create<Position, Crop, Background>();
        Position& pos = m_world.GetComponent<Position>(crop);
        pos.x = x * 16 + 8;
        pos.y = y * 16 + 8;
        Crop& cr = m_world.GetComponent<Crop>(crop);
        cr.stage = 4;
        cr.watered = false;
        cr.stageHistory[m_currentDay] = cr.stage;
        for (int i = 0; i < m_currentDay; i++)
        {
            cr.stageHistory[i] = 0;
        }

        cr.tileX = x;
        cr.tileY = y;
        auto tile = m_level.GetTile(x, y).value();
        if (tile->index == 2)
        {
            cr.watered = true;
            tile->index = 4;
        }
        else
        {
            tile->index = 3;
        }

        return crop;
    }

    template<class T>
    tako::Entity SpawnObject(int x, int y, tako::Sprite* sprite, T type)
    {
        auto entity = m_world.Create<Position, SpriteRenderer, RigidBody, Pickup, Foreground, T>();
        Position& pos = m_world.GetComponent<Position>(entity);
        pos.x = x * 16 + 8;
        pos.y = y * 16 + 8;
        SpriteRenderer& ren = m_world.GetComponent<SpriteRenderer>(entity);
        ren.sprite = sprite;
        ren.size = {16, 16};
        RigidBody& rigid = m_world.GetComponent<RigidBody>(entity);
        rigid.size = {16, 16};
        rigid.entity = entity;
        T& t = m_world.GetComponent<T>(entity);
        t = type;
        Pickup& pickup = m_world.GetComponent<Pickup>(entity);
        pickup.x = x;
        pickup.y = y;
        pickup.entity = entity;

        return entity;
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
            Physics::Move(m_world, m_level, pos, rigid, moveVector * dt * 20);

            float interActX = pos.x + player.facing.x * 12;
            float interActY = pos.y + player.facing.y * 12;
            int tileX = ((int) interActX) / 16;
            int tileY = ((int) interActY) / 16;
            //Pickup drop
            if (input->GetKeyDown(tako::Key::L))
            {
                if (!player.heldObject)
                {
                    m_world.IterateComps<Pickup>([&](Pickup& pickup)
                    {
                        if (player.heldObject)
                        {
                            return;
                        }
                        if (pickup.x != tileX || pickup.y != tileY)
                        {
                            return;
                        }

                        player.heldObject = pickup.entity;
                    });
                    m_world.IterateHandle<Position, Interactable>([&](tako::EntityHandle handle)
                    {
                        if (player.heldObject)
                        {
                            return;
                        }
                        auto& interactable = m_world.GetComponent<Interactable>(handle.id);
                        auto& iPos = m_world.GetComponent<Position>(handle.id);
                        Rect interRect(iPos.x, iPos.y, interactable.w, interactable.h);
                        if (!interRect.PointInside(interActX, interActY))
                        {
                            return;
                        }

                        if (m_world.HasComponent<Well>(handle.id))
                        {
                            player.heldObject = SpawnObject(tileX, tileY, m_waterCan, WateringCan());
                        }
                    });
                    m_world.IterateComps<Crop>([&](Crop& crop)
                    {
                        if (player.heldObject)
                        {
                            return;
                        }
                        if (crop.tileX != tileX || crop.tileY != tileY)
                        {
                            return;
                        }
                        if (crop.stage == 4)
                        {
                            crop.stage = -69;
                            player.heldObject = SpawnObject(tileX, tileY, m_parsnip, Parsnip());
                            m_level.GetTile(tileX, tileY).value()->index = crop.watered ? 2 : 1;
                            crop.watered = true;
                        }
                    });
                    if (player.heldObject)
                    {
                        auto obj = player.heldObject.value();
                        m_world.RemoveComponent<Position>(obj);
                        m_world.RemoveComponent<Pickup>(obj);
                    }
                }
                else
                {
                    // Find out if tile is free
                    auto blocked = ((int) m_playerSpawn.x) / 16 == tileX && ((int) m_playerSpawn.y) / 16 == tileY;
                    m_world.IterateComps<Pickup>([&](Pickup& pickup)
                    {
                        if (blocked)
                        {
                            return;
                        }
                        if (pickup.x != tileX || pickup.y != tileY)
                        {
                            return;
                        }

                        blocked = true;
                    });
                    if (!blocked)
                    {
                        m_world.IterateComps<Crop>([&](Crop& crop)
                        {
                            if (blocked)
                            {
                                return;
                            }
                            if (crop.tileX != tileX || crop.tileY != tileY || crop.stage <= 0)
                            {
                                return;
                            }

                            blocked = true;
                        });
                    }

                    if (!blocked)
                    {
                        auto obj = player.heldObject.value();
                        m_world.AddComponent<Pickup>(obj);
                        m_world.AddComponent<Position>(obj);
                        Position& p = m_world.GetComponent<Position>(obj);
                        p.x = tileX * 16 + 8;
                        p.y = tileY * 16 + 8;
                        Pickup& pickup = m_world.GetComponent<Pickup>(obj);
                        pickup.x = tileX;
                        pickup.y = tileY;
                        pickup.entity = obj;
                        player.heldObject = std::nullopt;
                        Rect placed(p.x, p.y, 16, 16);
                        Rect self(pos.x, pos.y, rigid.size.x, rigid.size.y);
                        if (player.facing.x && Rect::OverlapX(self, placed))
                        {
                            pos.x += tako::mathf::sign(pos.x - p.x) * (16 - tako::mathf::abs(pos.x - p.x));
                        }
                        if (player.facing.y && Rect::OverlapY(self, placed))
                        {
                            pos.y += tako::mathf::sign(pos.y - p.y) * (16 - tako::mathf::abs(pos.y - p.y));
                        }
                    }
                }
            }
            // Use/interact
            if (input->GetKeyDown(tako::Key::K))
            {
                if (player.heldObject)
                {
                    auto obj = player.heldObject.value();
                    auto tileOpt = m_level.GetTile(tileX, tileY);
                    if (tileOpt)
                    {
                        auto tile = tileOpt.value();
                        if (m_world.HasComponent<WateringCan>(obj))
                        {
                            auto& waterCan = m_world.GetComponent<WateringCan>(obj);
                            if (tile->index == 1)
                            {
                                tile->index = 2;
                                waterCan.left--;
                            }
                            else
                            {
                                m_world.IterateComps<Crop>([&](Crop& crop)
                                {
                                    if (crop.tileX != tileX || crop.tileY != tileY)
                                    {
                                        return;
                                    }
                                    if (!crop.watered)
                                    {
                                        crop.watered = true;
                                        waterCan.left--;
                                        tile->index++;
                                    }
                                });
                            }
                            if (waterCan.left <= 0)
                            {
                                m_world.Delete(obj);
                                player.heldObject = std::nullopt;
                            }
                        }
                        else if(m_world.HasComponent<SeedBag>(obj))
                        {
                            if (tile->index == 1 || tile->index == 2)
                            {
                                auto blocked = false;
                                m_world.IterateComps<Pickup>([&](Pickup& pickup)
                                {
                                    if (tileX == pickup.x && tileY == pickup.y)
                                    {
                                        blocked = true;
                                    }
                                });
                                if (!blocked)
                                {
                                    CreateCrop(tileX, tileY);
                                }
                            }
                        }
                    }
                }
            }
        });


        m_dayTimeLeft -= dt;
        if (m_dayTimeLeft <= 0)
        {
            PassDay();
        }
        RerenderText(m_dayTimeLeftText, m_drawer, m_font, std::to_string((int) std::ceil(m_dayTimeLeft)));
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
                if (crop.stage < 4)
                {
                    crop.stage++;
                }
                crop.stageHistory[m_currentDay] = crop.stage;
            }
            else
            {
                crop.stage = crop.stageHistory[m_currentDay - 1];
            }
            if (crop.stage > 0)
            {
                m_level.GetTile(crop.tileX, crop.tileY).value()->index = 1 + 2 * crop.stage;
            }
            crop.watered = false;
        });
        std::vector<tako::Entity> clearCrop;
        if (allWatered)
        {
            m_currentDay++;
            RerenderText(m_currentDayText, m_drawer, m_font, "Day " + std::to_string(m_currentDay));
        }
        else
        {
            m_world.IterateHandle<Parsnip>([&](tako::EntityHandle handle)
            {
                clearCrop.push_back(handle.id);
                m_world.IterateComps<Player>([&](Player& player)
                {
                    if (player.heldObject && player.heldObject.value() == handle.id)
                    {
                        player.heldObject = std::nullopt;
                    }
                });
            });
        }
        m_world.IterateHandle<Crop>([&](tako::EntityHandle handle)
        {
            Crop& crop = m_world.GetComponent<Crop>(handle.id);
            if (crop.stage <= 0)
            {
                clearCrop.push_back(handle.id);
                if (!allWatered)
                {
                    m_level.GetTile(crop.tileX, crop.tileY).value()->index = 1;
                }
            }
        });
        for (auto ent : clearCrop)
        {
            m_world.Delete(ent);
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
        float colorGradient = dayTimeEasing(m_dayTimeLeft / DAY_LENGTH);
        tako::Color dayLightColor(255 * colorGradient, 255 * colorGradient, 255 * colorGradient, 255);

        m_world.IterateComps<Position, Camera>([&](Position& pos, Camera& player)
        {
           drawer->SetCameraPosition(FitMapBound(m_level.MapBounds(), pos.AsVec(), drawer->GetCameraViewSize()));
        });
        m_level.Draw(drawer, dayLightColor);

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
           drawer->DrawSprite(pos.x - sprite.size.x / 2, pos.y + sprite.size.y / 2, sprite.size.x, sprite.size.y, sprite.sprite, dayLightColor);
        });
        m_world.IterateComps<Position, SpriteRenderer, Foreground>([&](Position& pos, SpriteRenderer& sprite, Foreground& f)
        {
           drawer->DrawSprite(pos.x - sprite.size.x / 2, pos.y + sprite.size.y / 2, sprite.size.x, sprite.size.y, sprite.sprite, dayLightColor);
        });

        if (m_currentDay > 0)
        {
            drawer->SetCameraPosition(cameraSize / 2);
            constexpr auto uiBackground = tako::Color(238, 195, 154, 255);
            drawer->DrawRectangle(0, cameraSize.y, 60, 28, uiBackground);
            drawer->DrawImage(4, cameraSize.y - 4, m_currentDayText.size.x, m_currentDayText.size.y, m_currentDayText.texture, {0, 0, 0, 255});

            drawer->DrawRectangle(36, cameraSize.y - 4, 20, 20, {0, 0, 0, 255});
            drawer->DrawRectangle(37, cameraSize.y - 5, 18, 18, uiBackground);
            m_world.IterateComps<Player>([&](Player& p)
            {
                if (p.heldObject)
                {
                    drawer->DrawSprite(38, cameraSize.y - 6, 16, 16, m_world.GetComponent<SpriteRenderer>(p.heldObject.value()).sprite);
                }
            });


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
    tako::Sprite* m_waterCan;
    tako::Sprite* m_seedBag;
    tako::Sprite* m_parsnip;
    tako::PixelArtDrawer* m_drawer;

    float easeInSine(float x)
    {
        return 1 - std::cos((x * tako::mathf::PI) / 2);
    }

    float dayTimeEasing(float x)
    {
        if (x > 0.5f)
        {
            return 0.7f + 0.3f * easeInSine((1-x) / 0.5f);
        }
        else
        {
            return 0.4f + 0.6f * easeInSine(x / 0.5f);
        }
    }
};