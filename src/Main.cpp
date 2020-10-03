#include "Tako.hpp"
#include "Game.hpp"

static Game game;

void tako::Setup(tako::PixelArtDrawer* drawer, Resources* resources)
{
    game.Setup(drawer);
}

void tako::Update(tako::Input* input, float dt)
{
    game.Update(input, dt);
}

void tako::Draw(tako::PixelArtDrawer* drawer)
{
    game.Draw(drawer);
}