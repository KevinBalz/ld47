#pragma once
#include "Tako.hpp"
#include "Rect.hpp"
#include <map>
#include <array>
#include <vector>

namespace
{
    constexpr auto tilesetTileCount = 20;
}

struct BuildingSize
{
    int startIndex;
    int x;
    int y;

    BuildingSize(int s, int x, int y)
    {
        startIndex = s;
        this->x = x;
        this->y = y;
    }
};

const std::map<char, BuildingSize> BUILDING_INFO =
{{
    {'W', {13, 2, 2}},
    {'B', {17, 2, 2}}
}};

struct Tile
{
    int index = 0;
    bool solid = false;
};

class Level
{
public:
    void Init(tako::PixelArtDrawer* drawer, tako::Resources* resources)
    {
        auto bitmap = tako::Bitmap::FromFile("/Tileset.png");
        auto tileset = drawer->CreateTexture(bitmap);
        int tilesPerTilesetRow = bitmap.Width() / 16;
        for (int i = 0; i < tilesetTileCount; i++)
        {
            int y = i / tilesPerTilesetRow;
            int x = i - y * tilesPerTilesetRow;
            m_tileSprites[i] = drawer->CreateSprite(tileset, x * 16, y * 16, 16, 16);
        }
    }

    void LoadLevel(const char* file, std::map<char, std::function<void(int,int)>>& callbackMap)
    {
        constexpr size_t bufferSize = 1024 * 1024;
        std::array <tako::U8, bufferSize> buffer;
        size_t bytesRead = 0;
        if (!tako::FileSystem::ReadFile(file, buffer.data(), bufferSize, bytesRead))
        {
            LOG_ERR("Could not read level {}", file);
        }

        auto levelStr = reinterpret_cast<const char *>(buffer.data());
        std::vector<char> tileChars;
        tileChars.reserve(bytesRead);
        {
            int maxX = 0;
            int maxY = 0;
            int x = 0;
            int y = 0;

            for (int i = 0; i < bytesRead; i++) {
                if (levelStr[i] != '\n' && levelStr[i] != '\0') {
                    x++;
                    tileChars.push_back(levelStr[i]);
                } else {
                    maxY++;
                    maxX = std::max(maxX, x);
                    x = 0;
                }
            }

            m_width = maxX;
            m_height = maxY;
        }

        for (int i = 0; i < tileChars.size(); i++)
        {
            Tile tile;
            switch (tileChars[i])
            {
                case 'D':
                    tile.index = 1;
                    break;
                case 'S':
                    tile.index = 11;
                    break;
                case 'C':
                    tile.index = 3;
                    break;
                case 'G':
                case 'b':
                case 'w':
                    tile.index = 11;
                    break;
                case 'W':
                case 'B':
                    tile.index = BUILDING_INFO.at(tileChars[i]).startIndex;
                    tile.solid = true;
                    break;
                case '+':
                {
                    int bx = 0;
                    int by = 0;
                    while(i-bx > 0 && (tileChars[i-bx] == '+' || BUILDING_INFO.find(tileChars[i-bx]) != BUILDING_INFO.end()))
                    {
                        bx++;
                    }
                    while(i-by*m_width > 0 && (tileChars[i-by*m_width] == '+' || BUILDING_INFO.find(tileChars[i-by*m_width]) != BUILDING_INFO.end()))
                    {
                        by++;
                    }
                    bx--;
                    by--;
                    auto building = BUILDING_INFO.find(tileChars[i-bx-by*m_width]);
                    tile.index = building->second.startIndex + bx + by * building->second.x;
                    tile.solid = true;
                    break;
                }
            }

            m_tiles.push_back(tile);
            if (callbackMap.find(tileChars[i]) != callbackMap.end())
            {
                int y = m_height - i / m_width;
                int x = i % m_width;
                callbackMap[tileChars[i]](x, y);
            }
        }
    }

    void Draw(tako::PixelArtDrawer* drawer, tako::Color color = {255, 255, 255, 255})
    {
        for (int y = m_height; y >= 0; y--)
        {
            for (int x = 0; x < m_width; x++)
            {
                int i = (m_height - y) * m_width + x;
                int tile = m_tiles[i].index;
                if (tile == 0)
                {
                    continue;
                }

                drawer->DrawSprite(x * 16, y * 16 + 16, 16, 16, m_tileSprites[tile - 1], color);
            }
        }

    }

    Rect MapBounds()
    {
        float width = m_width * 16;
        float height = m_height * 16;
        return
        {
            width / 2,
            height / 2,
            width,
            height
        };
    }

    std::optional<Tile*> GetTile(int x, int y)
    {
        if (x < 0 || x > m_width || y < 0 || y > m_height)
        {
            return {};
        }
        return { &m_tiles[x + (m_height - y) * m_width] };
    }

    void ResetWatered()
    {
        for (auto& tile : m_tiles)
        {
            if (tile.index >= 1 && tile.index <= 10)
            {
                tile.index += tile.index % 2 - 1;
            }
        }
    }

    std::optional<Rect> Overlap(Rect rect)
    {
        /*
            TL TM TR
            ML MM MR
            BL BM BR
        */
        int tileX = ((int) rect.x) / 16;
        int tileY = ((int) rect.y) / 16;

        Rect r;
        for (int y = -1; y <= 1; y++)
        {
            for (int x = -1; x <= 1; x++)
            {
                int tX = tileX + x;
                int tY = tileY + y;

                if (tX < 0 || tX >= m_width || tY < 0 || tY >= m_height)
                {
                    r = { tX * 16.0f + 8, tY * 16.0f + 8, 16, 16};
                    if (Rect::Overlap(r, rect))
                    {
                        return r;
                    }
                }
                int i = (m_height - tY) * m_width + tX;
                if (!m_tiles[i].solid)
                {
                    continue;
                }

                r = { tX * 16.0f + 8, tY * 16.0f + 8, 16, 16};
                if (Rect::Overlap(r, rect))
                {
                    return r;
                }
            }
        }

        return std::nullopt;
    }
private:
    std::array<tako::Sprite*, tilesetTileCount> m_tileSprites;
    std::vector<Tile> m_tiles;
    int m_width;
    int m_height;
};