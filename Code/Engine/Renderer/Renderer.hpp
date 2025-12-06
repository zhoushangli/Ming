#pragma once

#include "Engine/Math/IntVec2.hpp"
#include "Engine/Renderer/BitmapFont.hpp"

#include <map>
#include <vector>

struct Vec2;
struct Rgba8;
struct Vertex;
class Camera;
class Texture;

struct RendererConfig
{
	bool m_isEnable = true;
};

enum class BlendMode
{
    ALPHA,
    ADDITIVE,
};


class Renderer
{
public:
    Renderer(RendererConfig config);
    ~Renderer();

    void Startup();
    void Shutdown();

    void BeginFrame();
    void EndFrame();

    void CreateRenderingContext();

    void ClearScreen(Rgba8 const& clearColor);
    void SetBlendMode(BlendMode blendMode);

    void BeginCamera(Camera const& camera);
    void EndCamera();

    void DrawVertexArray(int numVertexes, Vertex const* vertexes) const;
    void DrawVertexArray(std::vector<Vertex> const& verts) const;

	void BindTexture(Texture* textureOrNull);

	Texture* CreateOrGetTextureFromFile(char const* fileDataPath);
    Texture* CreateTextureFromData(char const* name, IntVec2 dimensions, int bytesPerTexel, uint8_t* texelData);
    BitmapFont* CreateOrGetBitmapFont(char const* fontFilePathNameWithNoExtension);

private:
	Texture* CreateTextureFromFile(char const* fileDataPath);
	Texture* GetTextureFromFileName(char const* fileName);

	std::map<std::string, Texture*> m_loadedTexturesDict;
    std::map<std::string, BitmapFont*> m_loadedFontsDict;

private:
	RendererConfig m_config;

	Camera* m_currentCamera = nullptr;
};

