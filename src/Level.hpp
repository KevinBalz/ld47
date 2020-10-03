#pragma once
#include "Tako.hpp"
#include "Rect.hpp"
#include <map>
#include <array>
#include <vector>

namespace
{
    constexpr auto tilesetTileCount = 4;
}

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
                    tile.index = 1;
                    break;
                case 'C':
                    tile.index = 3;
                    break;
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

    void Draw(tako::PixelArtDrawer* drawer)
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

                drawer->DrawSprite(x * 16, y * 16 + 16, 16, 16, m_tileSprites[tile - 1]);
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
            if (tile.index == 2)
            {
                tile.index = 1;
            }
            else if (tile.index == 4)
            {
                tile.index = 3;
            }
        }
    }
private:
    std::array<tako::Sprite*, tilesetTileCount> m_tileSprites;
    std::vector<Tile> m_tiles;
    int m_width;
    int m_height;
};