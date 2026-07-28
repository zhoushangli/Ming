#pragma once

namespace SurfaceTextureSlot
{
static const unsigned int Diffuse = 0;
static const unsigned int Normal  = 1;
static const unsigned int SGE     = 2;
} // namespace SurfaceTextureSlot

namespace PostProcessTextureSlot
{
static const unsigned int Color            = 0;
static const unsigned int Depth            = 1;
static const unsigned int Normal           = 2;
static const unsigned int CustomInputStart = 8;
static const unsigned int MaxSamplerSlots  = 16;
} // namespace PostProcessTextureSlot
