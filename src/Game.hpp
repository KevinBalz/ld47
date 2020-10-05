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
#include <sstream>

constexpr auto DAY_LENGTH = 60.0f;

struct Text
{
    tako::Texture* texture;
    tako::Vector2 size;
};

enum class SCREEN
{
    PressAny,
    Title,
    Game,
    EndScreen
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
        m_parsnipUI = resources->Load<tako::Texture>("/ParsnipUI.png");
        auto playerBit = resources->Load<tako::Texture>("/Player.png");
        for (int i = 0; i < m_playerSprites.size(); i++)
        {
            m_playerSprites[i] = drawer->CreateSprite(playerBit, i * 16, 0, 16, 24);
        }
        LoadClips();

        m_level.Init(drawer, resources);
        std::map<char, std::function<void(int,int)>> dummyMap;
        m_level.LoadLevel("/Level.txt", dummyMap);

        m_textPressAny = CreateText(drawer, m_font, "Press a button to start");
        m_textTitle = CreateText(drawer, m_font, "HARVEST\nMINUTE");
        m_textControls = CreateText(drawer, m_font, " WASD - Move\n  L/C - Pickup/Drop\n  K/X - Use held item\nEnter - Skip to end of day");
        m_textCredits = CreateText(drawer, m_font, "Made in 72 hours by Malai\nLudum Dare 47 - Stuck in a loop\nPowered by tako");
        m_textEndScreen = CreateText(drawer, m_font, "This is a bug");
    }

    void StartGame()
    {
        InitGame();
    }

    void InitGame()
    {
        m_world.Reset();
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
            {'B', [&](int x, int y)
            {
                SpawnBuilding(x, y, BUILDING_INFO.at('B'), TransportBox());
            }},
            {'b', [&](int x, int y)
            {
                SpawnObject(x, y, m_seedBag, SeedBag());
            }},
            {'w', [&](int x, int y)
            {
                SpawnObject(x, y, m_waterCan, WateringCan());
            }}
        }};
        m_level.LoadLevel("/Level.txt", levelCallbacks);

        {
            auto player = m_world.Create<Position, SpriteRenderer, AnimatedSprite, Player, RigidBody, Foreground, Camera>();
            Position& pos = m_world.GetComponent<Position>(player);
            pos = m_playerSpawn;
            RigidBody& rigid = m_world.GetComponent<RigidBody>(player);
            rigid.size = { 15, 15 };
            rigid.entity = player;
            SpriteRenderer& renderer = m_world.GetComponent<SpriteRenderer>(player);
            renderer.size = { 16, 24};
            renderer.sprite = m_playerSprites[0];
            renderer.offset = {0, 8};
            AnimatedSprite& anim = m_world.GetComponent<AnimatedSprite>(player);
            anim.SetStatic(&m_playerSprites[0]);
            Player& play = m_world.GetComponent<Player>(player);
            play.facing = { 0, -1 };
            play.heldObject = std::nullopt;
        }
        m_currentDay = 1;
        m_currentDayText = CreateText(m_drawer, m_font, "Day " + std::to_string(m_currentDay));
        m_dayTimeLeft = DAY_LENGTH;
        m_dayTimeLeftText = CreateText(m_drawer, m_font, std::to_string(m_dayTimeLeft));
        m_parsnipCount = m_parsnipCountPrev = m_parsnipCountSafe = 0;
        m_parsnipText = CreateText(m_drawer, m_font, std::to_string(m_parsnipCount));
        m_screen = SCREEN::Game;
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
        cr.stage = 1;
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
        auto entity = m_world.Create<Position, SpriteRenderer, RigidBody, Pickup, Background, T>();
        Position& pos = m_world.GetComponent<Position>(entity);
        pos.x = x * 16 + 8;
        pos.y = y * 16 + 8;
        SpriteRenderer& ren = m_world.GetComponent<SpriteRenderer>(entity);
        ren.sprite = sprite;
        ren.size = {16, 16};
        RigidBody& rigid = m_world.GetComponent<RigidBody>(entity);
        rigid.size = {14, 14};
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
        switch (m_screen)
        {
            case SCREEN::PressAny:
                if (GetAnyDown(input))
                {
                    tako::Audio::Play(*m_clipMusic, true);
                    m_screen = SCREEN::Title;
                }
                break;
            case SCREEN::Title:
                if (GetAnyDown(input))
                {
                    StartGame();
                }
                break;
            case SCREEN::Game:
                GameUpdate(input, dt);
                break;
            case SCREEN::EndScreen:
                if (input->GetKeyDown(tako::Key::Enter) || input->GetKeyDown(tako::Key::Gamepad_Start))
                {
                    InitGame();
                }
                break;
        }
    }

    bool GetAnyDown(tako::Input* input)
    {
        for (int i = 0; i < (int) tako::Key::Unknown; i++)
        {
            if (input->GetKeyDown((tako::Key) i))
            {
                return true;
            }
        }

        return false;
    }

    void GameUpdate(tako::Input* input, float dt)
    {
        m_world.IterateComps<Position, Player, RigidBody, SpriteRenderer, AnimatedSprite>([&](Position& pos, Player& player, RigidBody& rigid, SpriteRenderer& spriteRenderer, AnimatedSprite& anim)
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
            auto moveMagnitude = moveVector.magnitude();
            bool changedFacing = false;
            if (moveMagnitude > 1)
            {
                moveVector.normalize();
            }
            else if (moveMagnitude > 0)
            {
                auto newFace = tako::Vector2::Normalized(moveVector);
                changedFacing = player.facing != newFace;
                player.facing = newFace;
                if (changedFacing)
                {
                    spriteRenderer.size.x = tako::mathf::sign(player.facing.x) * tako::mathf::abs(spriteRenderer.size.x);
                }
            }
            Physics::Move(m_world, m_level, pos, rigid, moveVector * dt * 30);
            if ((!player.wasMoving || changedFacing) && moveMagnitude > 0)
            {
                anim.SetAnim(0.15f, &m_playerSprites[GetIdleIndex(player.facing)], 4);
                spriteRenderer.sprite = m_playerSprites[GetIdleIndex(player.facing)+1];
                anim.passed = 0;
            }
            else if ((player.wasMoving || changedFacing) && moveMagnitude == 0)
            {
                anim.SetStatic(&m_playerSprites[GetIdleIndex(player.facing)]);
            }
            player.wasMoving = moveMagnitude > 0;

            float interActX = pos.x + player.facing.x * 12;
            float interActY = pos.y + player.facing.y * 12;
            int tileX = ((int) interActX) / 16;
            int tileY = ((int) interActY) / 16;
            //Pickup drop
            if (input->GetKeyDown(tako::Key::L) || input->GetKeyDown(tako::Key::C) || input->GetKeyDown(tako::Key::Gamepad_A))
            {
                bool didInteract = false;
                m_world.IterateHandle<Position, Interactable>([&](tako::EntityHandle handle)
                {
                    auto& interactable = m_world.GetComponent<Interactable>(handle.id);
                    auto& iPos = m_world.GetComponent<Position>(handle.id);
                    Rect interRect(iPos.x, iPos.y, interactable.w, interactable.h);
                    if (!interRect.PointInside(interActX, interActY))
                    {
                        return;
                    }

                    if (m_world.HasComponent<Well>(handle.id))
                    {
                        if (player.heldObject)
                        {
                            auto held = player.heldObject.value();
                            if (m_world.HasComponent<WateringCan>(held))
                            {
                                auto& watering = m_world.GetComponent<WateringCan>(held);
                                watering = WateringCan();
                                tako::Audio::Play(*m_clipSplash);
                                didInteract = true;
                            }
                            return;
                        }
                        auto obj = SpawnObject(tileX, tileY, m_waterCan, WateringCan());
                        m_world.RemoveComponent<Position>(obj);
                        m_world.RemoveComponent<Pickup>(obj);
                        player.heldObject = obj;
                        tako::Audio::Play(*m_clipSplash);
                        didInteract = true;
                    }
                    else if (m_world.HasComponent<TransportBox>(handle.id))
                    {
                        if (!player.heldObject)
                        {
                            return;
                        }
                        auto held = player.heldObject.value();
                        if (m_world.HasComponent<Parsnip>(held))
                        {
                            if (m_world.GetComponent<Parsnip>(held).harvestDay < m_currentDay)
                            {
                                m_parsnipCountSafe++;
                            }
                            m_world.Delete(held);
                            player.heldObject = std::nullopt;
                            m_parsnipCount++;
                            RerenderText(m_parsnipText, m_drawer, m_font, std::to_string(m_parsnipCount));
                            tako::Audio::Play(*m_clipSend);
                            didInteract = true;
                        }
                    }
                });
                if (!didInteract)
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
                            tako::Audio::Play(*m_clipPickup);
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
                                Parsnip snip;
                                snip.harvestDay = m_currentDay;
                                player.heldObject = SpawnObject(tileX, tileY, m_parsnip, snip);
                                m_level.GetTile(tileX, tileY).value()->index = crop.watered ? 2 : 1;
                                crop.watered = true;
                                tako::Audio::Play(*m_clipHarvest);
                            }
                        });
                        if (player.heldObject)
                        {
                            auto obj = player.heldObject.value();
                            m_world.RemoveComponent<Position>(obj);
                            m_world.RemoveComponent<Pickup>(obj);
                        }
                        else
                        {
                            tako::Audio::Play(*m_clipError);
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
                            m_world.IterateHandle<Position, Interactable>([&](tako::EntityHandle handle)
                            {
                                if (blocked)
                                {
                                    return;
                                }
                                auto &interactable = m_world.GetComponent<Interactable>(handle.id);
                                auto &iPos = m_world.GetComponent<Position>(handle.id);
                                Rect interRect(iPos.x, iPos.y, interactable.w, interactable.h);
                                if (interRect.PointInside(interActX, interActY))
                                {
                                    blocked = true;
                                    return;
                                }
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
                            tako::Audio::Play(*m_clipDrop);
                        }
                        else
                        {
                            tako::Audio::Play(*m_clipError);
                        }
                    }
                }
            }
            // Use/interact
            if (input->GetKeyDown(tako::Key::K) || input->GetKeyDown(tako::Key::X) || input->GetKeyDown(tako::Key::Gamepad_B))
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
                            auto didWater = false;
                            if (tile->index == 1)
                            {
                                tile->index = 2;
                                waterCan.left--;
                                didWater = true;
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
                                        didWater = true;
                                    }
                                });
                            }
                            if (waterCan.left <= 0)
                            {
                                m_world.Delete(obj);
                                player.heldObject = std::nullopt;
                            }
                            tako::Audio::Play(didWater ? *m_clipWater : *m_clipError);
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
                                    tako::Audio::Play(*m_clipSow);
                                }
                                else
                                {
                                    tako::Audio::Play(*m_clipError);
                                }
                            }
                            else
                            {
                                tako::Audio::Play(*m_clipError);
                            }
                        }
                        else
                        {
                            tako::Audio::Play(*m_clipError);
                        }
                    }
                }
                else
                {
                    tako::Audio::Play(*m_clipError);
                }
            }
        });

        if (m_dayTimeLeft > 3 && (input->GetKeyDown(tako::Key::Enter) || input->GetKeyDown(tako::Key::Gamepad_Start)))
        {
            m_dayTimeLeft = 3;
        }

        m_dayTimeLeft -= dt;
        if (m_dayTimeLeft <= 0)
        {
            PassDay();
        }
        int dayLeft = std::ceil(m_dayTimeLeft);
        if (dayLeft != m_dayTimeLeftPrev && dayLeft <= 10)
        {
            tako::Audio::Play(*m_clipTick);
        }
        RerenderText(m_dayTimeLeftText, m_drawer, m_font, (dayLeft < 10 ? " " : "") + std::to_string(dayLeft));
        m_dayTimeLeftPrev = dayLeft;


        m_world.IterateComps<SpriteRenderer, AnimatedSprite>([&](SpriteRenderer& sprite, AnimatedSprite& anim)
        {
            anim.passed += dt;
            if (anim.passed >= anim.duration)
            {
                int index = 0;
                while (index < anim.frames && anim.sprites[index] != sprite.sprite)
                {
                    index++;
                }
                index++;
                if (index >= anim.frames)
                {
                    sprite.sprite = anim.sprites[0];
                }
                else
                {
                    sprite.sprite = anim.sprites[index];
                }
                anim.passed = 0;
            }
        });
    }

    void PassDay()
    {
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
            tako::Audio::Play(*m_clipDay);
            if (m_currentDay == TOTAL_DAYS)
            {
                m_dayTimeLeft = 0;
                m_screen = SCREEN::EndScreen;
                RenderEndText();
                return;
            }
            m_currentDay++;
            m_parsnipCountPrev = m_parsnipCount;
            m_parsnipCountSafe = 0;
            RerenderText(m_currentDayText, m_drawer, m_font, "Day " + std::to_string(m_currentDay));
        }
        else
        {
            tako::Audio::Play(*m_clipLoop);
            m_parsnipCount = m_parsnipCountPrev + m_parsnipCountSafe;
            RerenderText(m_parsnipText, m_drawer, m_font, std::to_string(m_parsnipCount));
            m_world.IterateHandle<Parsnip>([&](tako::EntityHandle handle)
            {
                if (m_world.GetComponent<Parsnip>(handle.id).harvestDay == m_currentDay)
                {
                    clearCrop.push_back(handle.id);
                    m_world.IterateComps<Player>([&](Player &player)
                    {
                        if (player.heldObject && player.heldObject.value() == handle.id)
                        {
                            player.heldObject = std::nullopt;
                        }
                    });
                }
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
        for (auto [pos, player, anim]: m_world.Iter<Position, Player, AnimatedSprite>())
        {
            pos = m_playerSpawn;
            player.wasMoving = false;
            player.facing = { 0, -1 };
            anim.SetStatic(&m_playerSprites[0]);
        }
        m_dayTimeLeft = DAY_LENGTH;
    }


    void Draw(tako::PixelArtDrawer* drawer)
    {
        if (m_screen == SCREEN::PressAny)
        {
            return DrawPressAny(drawer);
        }
        if (m_screen == SCREEN::Title)
        {
            return DrawTitle(drawer);
        }
        auto cameraSize = drawer->GetCameraViewSize();
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
           drawer->DrawSprite(pos.x - sprite.size.x / 2 + sprite.offset.x, pos.y + sprite.size.y / 2 + sprite.offset.y, sprite.size.x, sprite.size.y, sprite.sprite, dayLightColor);
        });
        m_world.IterateComps<Position, SpriteRenderer, Foreground>([&](Position& pos, SpriteRenderer& sprite, Foreground& f)
        {
           drawer->DrawSprite(pos.x - sprite.size.x / 2 + sprite.offset.x, pos.y + sprite.size.y / 2+ sprite.offset.y, sprite.size.x, sprite.size.y, sprite.sprite, dayLightColor);
        });

        constexpr auto uiBackground = tako::Color(238, 195, 154, 255);
        if (m_screen == SCREEN::Game)
        {
            drawer->SetCameraPosition(cameraSize / 2);

            drawer->DrawRectangle(0, cameraSize.y, 60, 28, uiBackground);
            drawer->DrawImage(4, cameraSize.y - 4, m_currentDayText.size.x, m_currentDayText.size.y, m_currentDayText.texture, {0, 0, 0, 255});

            drawer->DrawImage(3, cameraSize.y - 16, 5, 7, m_parsnipUI);
            drawer->DrawImage(11, cameraSize.y - 16, m_parsnipText.size.x, m_parsnipText.size.y, m_parsnipText.texture, {0, 0, 0, 255});

            drawer->DrawRectangle(36, cameraSize.y - 4, 20, 20, {0, 0, 0, 255});
            drawer->DrawRectangle(37, cameraSize.y - 5, 18, 18, uiBackground);
            m_world.IterateComps<Player>([&](Player& p)
            {
                if (p.heldObject)
                {
                    drawer->DrawSprite(38, cameraSize.y - 6, 16, 16, m_world.GetComponent<SpriteRenderer>(p.heldObject.value()).sprite);
                }
            });

            drawer->DrawRectangle(4, cameraSize.y - 30, 15, 11, uiBackground);
            auto timerColor = m_dayTimeLeft > 10 ? tako::Color(0, 0, 0, 255) : tako::Color(255, 0, 0, 255);
            drawer->DrawImage(6, cameraSize.y - 32, m_dayTimeLeftText.size.x, m_dayTimeLeftText.size.y, m_dayTimeLeftText.texture, timerColor);
        }
        if (m_screen == SCREEN::EndScreen)
        {
            drawer->SetCameraPosition({0, 0});
            auto renPos = tako::Vector2(m_textEndScreen.size.x * -0.5f, m_textEndScreen.size.y * 0.5f);
            drawer->DrawRectangle(renPos.x - 4, renPos.y + 4, m_textEndScreen.size.x + 8, m_textEndScreen.size.y + 8, uiBackground);
            drawer->DrawImage(renPos.x, renPos.y, m_textEndScreen.size.x, m_textEndScreen.size.y, m_textEndScreen.texture, {0, 0, 0, 255});
        }
    }

    void DrawPressAny(tako::PixelArtDrawer* drawer)
    {
        drawer->Clear();
        drawer->SetCameraPosition({0, 0});
        drawer->DrawImage(-m_textPressAny.size.x/2, m_textPressAny.size.y/2, m_textPressAny.size.x, m_textPressAny.size.y, m_textPressAny.texture);
    }

    void DrawTitle(tako::PixelArtDrawer* drawer)
    {
        constexpr auto uiBackground = tako::Color(238, 195, 154, 255);
        auto cameraSize = drawer->GetCameraViewSize();
        drawer->Clear();
        drawer->SetCameraPosition(FitMapBound(m_level.MapBounds(), {170, 180}, cameraSize));
        m_level.Draw(drawer);
        drawer->SetCameraPosition({0, 0});
        constexpr auto titleScale = 3;
        auto renPos = tako::Vector2(m_textTitle.size.x * titleScale * -0.5f, m_textTitle.size.y * titleScale * 0.5f + 40);
        drawer->DrawRectangle(renPos.x - 8, renPos.y + 8, m_textTitle.size.x * titleScale + 16, m_textTitle.size.y * titleScale + 16, uiBackground);
        drawer->DrawImage(renPos.x, renPos.y, m_textTitle.size.x * titleScale, m_textTitle.size.y * titleScale, m_textTitle.texture, {0, 0, 0, 255});

        drawer->DrawImage(m_textControls.size.x / -2 , m_textControls.size.y / 2 - 25, m_textControls.size.x, m_textControls.size.y, m_textControls.texture, {0, 0, 0, 255});
        drawer->SetCameraPosition(cameraSize/2);
        drawer->DrawImage(4, m_textCredits.size.y + 4, m_textCredits.size.x, m_textCredits.size.y, m_textCredits.texture, {0, 0, 0, 255});
    }
private:
    SCREEN m_screen = SCREEN::PressAny;
    int m_currentDay = 0;
    float m_dayTimeLeft;
    int m_dayTimeLeftPrev;
    int m_parsnipCount;
    int m_parsnipCountSafe;
    int m_parsnipCountPrev;
    Text m_currentDayText;
    Text m_dayTimeLeftText;
    Text m_parsnipText;
    tako::Vector2 m_playerSpawn = {0, 0};
    tako::Font* m_font;
    tako::World m_world;
    Level m_level;
    tako::Texture* m_parsnipUI;
    tako::Sprite* m_waterCan;
    tako::Sprite* m_seedBag;
    tako::Sprite* m_parsnip;
    std::array<tako::Sprite*, 12> m_playerSprites;
    tako::PixelArtDrawer* m_drawer;
    tako::AudioClip* m_clipDay;
    tako::AudioClip* m_clipDrop;
    tako::AudioClip* m_clipError;
    tako::AudioClip* m_clipHarvest;
    tako::AudioClip* m_clipLoop;
    tako::AudioClip* m_clipMusic;
    tako::AudioClip* m_clipPickup;
    tako::AudioClip* m_clipSend;
    tako::AudioClip* m_clipSow;
    tako::AudioClip* m_clipSplash;
    tako::AudioClip* m_clipTick;
    tako::AudioClip* m_clipWater;

    Text m_textPressAny;
    Text m_textTitle;
    Text m_textControls;
    Text m_textCredits;
    Text m_textEndScreen;

    void LoadClips()
    {
        m_clipDay = new tako::AudioClip("/Day.wav");
        m_clipDrop = new tako::AudioClip("/Drop.wav");
        m_clipError = new tako::AudioClip("/Error.wav");
        m_clipHarvest = new tako::AudioClip("/Harvest.wav");
        m_clipLoop = new tako::AudioClip("/Loop.wav");
        m_clipMusic = new tako::AudioClip("/music.mp3");
        m_clipPickup = new tako::AudioClip("/Pickup.wav");
        m_clipSend = new tako::AudioClip("/Send.wav");
        m_clipSow = new tako::AudioClip("/Sow.wav");
        m_clipSplash = new tako::AudioClip("/Splash.wav");
        m_clipTick = new tako::AudioClip("/Tick.wav");
        m_clipWater = new tako::AudioClip("/Water.wav");
    }

    void RenderEndText()
    {
        std::stringstream str;
        str << "You harvested and sold\n"
            << m_parsnipCount << " parsnips!\n"
            << "Thanks for playing my LD 47 game!\n"
            << "Enter/Start to play again";
        RerenderText(m_textEndScreen, m_drawer, m_font, str.str());
    }

    float easeInSine(float x)
    {
        return 1 - std::cos((x * tako::mathf::PI) / 2);
    }

    float dayTimeEasing(float x)
    {
        if (x > 0.5f)
        {
            return 0.7f + 0.3f * (1-x) / 0.5f;
        }
        else
        {
            return 0.25f + 0.75f * easeInSine(x / 0.5f);
        }
    }

    int GetIdleIndex(tako::Vector2 facing)
    {
        if (facing.x != 0)
        {
            return 4;
        }
        if (facing.y > 0)
        {
            return 8;
        }

        return 0;
    }
};