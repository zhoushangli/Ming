#pragma once

#include "MingEngine/Engine/Render/Texture.hpp"

enum class SpriteAnimPlaybackType
{
    ONCE,
    LOOP,
    PINGPONG
};

class SpriteAnimDefinition
{
public:
    SpriteAnimDefinition(SpriteSheet const& sheet, int startSpriteIndex, int endSpriteIndex,
        float framesPerSecond, SpriteAnimPlaybackType playbackType = SpriteAnimPlaybackType::LOOP);

    SpriteDefinition const& GetSpriteDefAtTime(float seconds) const; // Most of the logic for this class is done here!

private:
    SpriteSheet const&  m_spriteSheet;
    int				    m_startSpriteIndex  = -1;
    int				    m_endSpriteIndex    = -1;
    float				m_framesPerSecond   = 1.f;
    SpriteAnimPlaybackType	m_playbackType  = SpriteAnimPlaybackType::LOOP;
};
