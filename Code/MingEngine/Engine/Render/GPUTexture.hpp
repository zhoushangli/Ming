#pragma once

#include "MingEngine/Core/Math/AABB2.hpp"
#include "MingEngine/Core/Math/IntVec2.hpp"

#include <string>
#include <vector>

struct ID3D11Texture2D;
struct ID3D11ShaderResourceView;
struct ID3D11RenderTargetView;
struct ID3D11DepthStencilView;

class GPUTexture
{
	friend class D3D11RenderBackend; // Only the Renderer can create new GPUTexture objects!
	friend class TextureResource;    // TextureResource owns and destroys its GPUTexture

private:
	GPUTexture();                                // can't instantiate directly; must ask Renderer to do it for you
	GPUTexture(GPUTexture const& copy) = delete; // No copying allowed!  This represents GPU memory.
	~GPUTexture();

public:
	IntVec2            GetDimensions() const { return m_dimensions; }
	std::string const& GetImageFilePath() const { return m_name; }

	ID3D11ShaderResourceView* GetShaderResourceView() const { return m_shaderResourceView; }
	ID3D11RenderTargetView*   GetRenderTargetView() const { return m_renderTargetView; }
	ID3D11DepthStencilView*   GetDepthStencilView() const { return m_depthStencilView; }

protected:
	std::string m_name; // Can't be char const* -- store a copy, in case it was temporary
	IntVec2     m_dimensions;

	ID3D11ShaderResourceView* m_shaderResourceView = nullptr; // Read Handle
	ID3D11RenderTargetView*   m_renderTargetView   = nullptr; // Write Handle
	ID3D11DepthStencilView*   m_depthStencilView   = nullptr; // kDepth Handle

	ID3D11Texture2D* m_texture = nullptr;
};

class SpriteSheet;

class SpriteDefinition
{
public:
	explicit SpriteDefinition(
		SpriteSheet const& spriteSheet, int spriteIndex, Vec2 const& uvAtMins, Vec2 const& uvAtMaxs);

	void               GetUVs(Vec2& out_uvAtMins, Vec2& out_uvAtMaxs) const;
	AABB2              GetUVs() const;
	SpriteSheet const& GetSpriteSheet() const;
	GPUTexture*        GetTexture() const;
	float              GetAspect() const;

protected:
	SpriteSheet const& m_spriteSheet;
	int                m_spriteIndex = -1;
	Vec2               m_uvAtMins    = Vec2::Zero;
	Vec2               m_uvAtMaxs    = Vec2::One;
};

class SpriteSheet
{
public:
	explicit SpriteSheet(GPUTexture* colorTexture, IntVec2 const& dimension);

	GPUTexture*             GetTexture() const;
	int                     GetNumSprites() const;
	SpriteDefinition const& GetSpriteDef(int spriteIndex) const;
	void                    GetSpriteUVs(Vec2& out_uvAtMins, Vec2& out_uvAtMaxs, int spriteIndex) const;
	AABB2                   GetSpriteUVs(int spriteIndex) const;
	AABB2                   GetSpriteUVs(IntVec2& spriteCoords) const;

protected:
	IntVec2                       m_dimension;
	GPUTexture*                   m_colorTexture = nullptr;
	std::vector<SpriteDefinition> m_spriteDefs;
};
