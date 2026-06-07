#include "MingEngine/Engine/Render/SpriteAnimDefinition.hpp"

SpriteAnimDefinition::SpriteAnimDefinition(
    SpriteSheet const& sheet,
    int startSpriteIndex,
    int endSpriteIndex,
    float framesPerSecond,
    SpriteAnimPlaybackType playbackType /*= SpriteAnimPlaybackType::LOOP*/) :
    m_spriteSheet(sheet),
    m_startSpriteIndex(startSpriteIndex),
    m_endSpriteIndex(endSpriteIndex),
    m_framesPerSecond(framesPerSecond),
    m_playbackType(playbackType)
{}

SpriteDefinition const& SpriteAnimDefinition::GetSpriteDefAtTime(float seconds) const
{
    int numFrames = m_endSpriteIndex - m_startSpriteIndex + 1;
    if (numFrames <= 0)
    {
        return m_spriteSheet.GetSpriteDef(m_startSpriteIndex);
    }

    seconds = seconds < 0.f ? 0.f : seconds;
    int frameIndex = 0;

    switch (m_playbackType)
    {
    case SpriteAnimPlaybackType::ONCE:
    {
        int frame = static_cast<int>(seconds * m_framesPerSecond);
        if (frame >= numFrames)
        {
            frameIndex = m_endSpriteIndex;
        }
        else
        {
            frameIndex = m_startSpriteIndex + frame;
        }
        break;
    }
    case SpriteAnimPlaybackType::LOOP:
    {
        int frame = static_cast<int>(seconds * m_framesPerSecond) % numFrames;
        frameIndex = m_startSpriteIndex + frame;
        break;
    }
    case SpriteAnimPlaybackType::PINGPONG:
    {
        int totalPingPongFrames = (numFrames * 2) - 2;
        int frame = static_cast<int>(seconds * m_framesPerSecond) % totalPingPongFrames;
        if (frame < numFrames)
        {
            frameIndex = m_startSpriteIndex + frame;
        }
        else
        {
            frameIndex = m_endSpriteIndex - (frame - numFrames + 1);
        }
        break;
    }
    default:
        frameIndex = m_startSpriteIndex;
        break;
    }

    return m_spriteSheet.GetSpriteDef(frameIndex);
}
