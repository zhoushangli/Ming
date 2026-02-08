#pragma once

#include "Engine/Math/IntVec2.hpp"
#include "Engine/Renderer/Shader.hpp"
#include "Engine/Renderer/BitmapFont.hpp"

#include <map>
#include <vector>

class Camera;
class Texture;

class VertexBuffer;
class ConstantBuffer;

struct Vec2;
struct Rgba8;
struct Vertex;

struct ID3D11Device;
struct ID3D11DeviceContext;
struct IDXGISwapChain;
struct ID3D11RenderTargetView;
struct ID3D11VertexShader;
struct ID3D11PixelShader;
struct ID3D11InputLayout;
struct ID3D11Buffer;
struct ID3D11RasterizerState;
struct ID3D11BlendState;

enum class BlendMode
{
    ALPHA,
    ADDITIVE,
    OPAQUE,
    COUNT
};

struct RendererConfig
{
	bool m_isEnable = true;
};

struct CameraConstants
{
    float OrthoMinX;
    float OrthoMinY;
    float OrthoMinZ;
    float OrthoMaxX;
    float OrthoMaxY;
    float OrthoMaxZ;
    float pad0;
    float pad1;
};
static const int k_cameraConstantsSlot = 2;


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
    void SetStatesIfChanged();

    void BeginCamera(Camera const& camera);
    void EndCamera();

    void DrawVertexArray(int numVertexes, Vertex const* vertexes) const;
    void DrawVertexArray(std::vector<Vertex> const& verts) const;
    void DrawVertexBuffer(VertexBuffer* vertexBuffer, unsigned int vertexCount);

	void BindTexture(Texture* textureOrNull);
    void BindShader(Shader* shader);

    Shader* CreateShader(char const* shaderName);
	Texture* CreateOrGetTextureFromFile(char const* fileDataPath);
    Texture* CreateTextureFromData(char const* name, IntVec2 dimensions, int bytesPerTexel, uint8_t* texelData);
    BitmapFont* CreateOrGetBitmapFont(char const* fontFilePathNameWithNoExtension);
    VertexBuffer* CreateVertexBuffer(const unsigned int size, unsigned int stride);
    ConstantBuffer* CreateConstantBuffer(const unsigned int size);

    void CopyCPUToGPU(const void* data, unsigned int size, VertexBuffer* vertexBuffer);
    void CopyCPUToGPU(const void* data, unsigned int size, ConstantBuffer* constantBuffer);

private:
	Texture* CreateTextureFromFile(char const* fileDataPath);
	Texture* GetTextureFromFileName(char const* fileName);

    Shader* CreateShader(char const* shaderName, char const* shaderSource);
    bool CompileShaderToByteCode(std::vector<unsigned char>& outByteCode, char const* name,
        char const* source, char const* entryPoint, char const* target);
    void BindVertexBuffer(VertexBuffer* vertexBuffer);
    void BindConstantBuffer(ConstantBuffer* constantBuffer);


private:
	RendererConfig m_config;

    Shader* m_defaultShader = nullptr;

	Camera* m_currentCamera              = nullptr;
    Shader* m_currentShader              = nullptr;
    VertexBuffer* m_currentVertexBuffer  = nullptr;
    ConstantBuffer* m_cameraCBO          = nullptr;

    ID3D11Device* m_device                     = nullptr;
    ID3D11DeviceContext* m_deviceContext       = nullptr;
    IDXGISwapChain* m_swapChain                = nullptr;
    ID3D11RenderTargetView* m_renderTargetView = nullptr;
    ID3D11RasterizerState* m_rasterizerState   = nullptr;

    ID3D11BlendState* m_blendState                         = nullptr;
    BlendMode m_desiredBlendMode                           = BlendMode::ALPHA;
    ID3D11BlendState* m_blendStates[(int)BlendMode::COUNT] = {};

    std::vector<Shader*>    m_loadedShaders;
    std::vector<uint8_t>    m_vertexShaderByteCode;
    std::vector<uint8_t>    m_pixelShaderByteCode;

	std::map<std::string, Texture*> m_loadedTexturesDict;
    std::map<std::string, BitmapFont*> m_loadedFontsDict;

#if defined(ENGINE_DEBUG_RENDER)
    void* m_dxgiDebug = nullptr;
    void* m_dxgiDebugModule = nullptr;
#endif
};

