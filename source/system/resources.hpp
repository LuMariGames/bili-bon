#pragma once
#include <citro2d.h>
#include <string>
#include <vector>
#include "../audio/ogg_audio.hpp"

struct Resources {
    C3D_RenderTarget* topTarget = nullptr;
    C3D_RenderTarget* bottomTarget = nullptr;
    C2D_SpriteSheet titleSheet = nullptr;
    C2D_Image titleImage{};
    bool hasTitleSprite = false;
    std::vector<OggPcm> soundEffects;

    bool load();
    void unload();
};
