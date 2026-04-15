#pragma once

#include "Game/EngineBuildPreferences.hpp"

#include "Engine/Core/Image.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/Matrix4x4.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Renderer/PostProcessPass.hpp"
#include "Engine/Renderer/Shader.hpp"

#include <map>
#include <vector>

class Camera;
class Texture;

class VertexBuffer;
class IndexBuffer;
class ConstantBuffer;

struct Vec2;
struct Rgba8;
struct Vertex;

struct ID3D11Device;
struct ID3D11DeviceContext;
struct IDXGISwapChain;
struct ID3DUserDefinedAnnotation;
struct ID3D11RenderTargetView;
struct ID3D11VertexShader;
struct ID3D11PixelShader;
struct ID3D11InputLayout;
struct ID3D11Buffer;
struct ID3D11RasterizerState;
struct ID3D11BlendState;
struct ID3D11SamplerState;
struct ID3D11Texture2D;
struct ID3D11DepthStencilView;
struct ID3D11DepthStencilState;

enum class BlendMode
{
	ALPHA,
	ADDITIVE,
	OPAQUE,
	COUNT
};

enum class SamplerMode
{
	POINT_CLAMP,
	BILINEAR_CLAMP,
	COUNT
};

enum class RasterizerMode
{
	SOLID_CULL_NONE,
	SOLID_CULL_BACK,
	WIREFRAME_CULL_NONE,
	WIREFRAME_CULL_BACK,
	COUNT
};

enum class DepthMode
{
	DISABLED,
	READ_ONLY_ALWAYS,
	READ_ONLY_LESS_EQUAL,
	READ_WRITE_LESS_EQUAL,
	COUNT
};

struct RendererConfig
{
	bool m_isEnable = true;
};

struct LightConstants
{
	Vec3  SunDirection;
	float SunIntensity;
	float AmbientIntensity;

	float padding[3]; // Padding to ensure the struct size is a multiple of 16 bytes for GPU constant buffer alignment
};
static const int k_lightConstantsSlot = 1;

struct CameraConstants
{
	Matrix4x4 WorldToCameraTransform;
	Matrix4x4 CameraToRenderTransform;
	Matrix4x4 RenderToClipTransform;
};
static const int k_cameraConstantsSlot = 2;

struct ModelConstants
{
	Matrix4x4 ModelToWorldTransform;
	float     ModelColor[4];
};
static const int k_modelConstantsSlot = 3;

struct PostProcessConstants
{
	Vec2  ScreenDimensions;
	float cameraNear;
	float cameraFar;
};
static const int k_postProcessConstantsSlot = 4;

class Renderer
{
public:
	Renderer(RendererConfig config);
	~Renderer();

	// Lifetime and frame loop
	void Startup();
	void Shutdown();
	void BeginColorPass();
	void BeginNormalPass();
	void BeginUIPass();
	void EndFrame();
	void CreateRenderingContext();

	void RenderPostProcess(Camera const& camera);

	// Camera and pipeline state
	void BeginCamera(Camera const& camera);
	void EndCamera();

	void ClearScreen(Rgba8 const& clearColor);
	void SetBlendMode(BlendMode blendMode);
	void SetSamplerMode(SamplerMode samplerMode);
	void SetRasterizerMode(RasterizerMode rasterizerMode);
	void SetDepthMode(DepthMode depthMode);
	void SetStatesIfChanged();

	// Draw entry points
	void DrawVertexArray(int numVertexes, Vertex const* vertexes) const;
	void DrawVertexArray(std::vector<Vertex> const& verts) const;
	void DrawVertexArray(std::vector<Vertex> const& verts, std::vector<unsigned int> const& vertIndexs) const;
	void DrawVertexBuffer(VertexBuffer* vertexBuffer);
	void DrawIndexedVertexBuffer(VertexBuffer* vertexBuffer, IndexBuffer* indexBuffer);

	// High-level bind helpers used by gameplay/render features
	void BindTexture(Texture* textureOrNull);
	void BindTexture(Texture* textureOrNull, unsigned int slot);
	void BindShader(Shader* shader);
	void BindModelConstants(Matrix4x4 const& modelToWorldTransform, Rgba8 const& modelColor);
	void BindLightConstants(Vec3 const& sunDirection, float const sunIntensity, float const ambientIntensity);
	void BindPostProcessConstants(Vec2 const& screenDimensions, float cameraNear, float cameraFar);

	// GPU resource creation and cache access
	Shader* CreateOrGetShader(char const* shaderName);

	Texture*    CreateOrGetTextureFromFile(char const* fileDataPath);
	Texture*    CreateTextureFromImage(const Image& image);
	Texture*    CreateTextureFromData(char const* name, IntVec2 dimensions, int bytesPerTexel, uint8_t* texelData);
	Texture*    CreateRenderTargetTexture(char const* name, IntVec2 dimensions);
	Texture*    CreateDepthStencilTexture(char const* name, IntVec2 dimensions);
	BitmapFont* CreateOrGetBitmapFont(char const* fontFilePathNameWithNoExtension);

	VertexBuffer*   CreateVertexBuffer(const unsigned int size, unsigned int stride);
	VertexBuffer*   CreateVertexBuffer(std::vector<Vertex> const& verts);
	ConstantBuffer* CreateConstantBuffer(const unsigned int size);
	IndexBuffer*    CreateIndexBuffer(const unsigned int size);
	IndexBuffer*    CreateIndexBuffer(std::vector<unsigned int> const& indexes);

	// CPU -> GPU uploads
	void CopyCPUToGPU(const void* data, unsigned int size, VertexBuffer* vertexBuffer);
	void CopyCPUToGPU(const void* data, unsigned int size, ConstantBuffer* constantBuffer);
	void CopyCPUToGPU(const void* data, unsigned int size, IndexBuffer* indexBuffer);

	// Post-process pass management
	void AddPostProcessPass(PostProcessPass const& pass);
	void BeginEvent(std::string const& eventName);
	void EndEvent();

private:
	// Texture cache internals
	Texture* CreateTextureFromFile(char const* fileDataPath);
	Texture* GetTextureFromFileName(char const* fileName);

	// Shader creation internals
	Shader* CreateShader(char const* shaderName, char const* shaderSource);
	bool    CompileShaderToByteCode(
		std::vector<unsigned char>& outByteCode,
		char const*                 name,
		char const*                 source,
		char const*                 entryPoint,
		char const*                 target
	);

	// Low-level buffer binding to D3D context
	void BindVertexBuffer(VertexBuffer* vertexBuffer);
	void BindConstantBuffer(ConstantBuffer* constantBuffer, int slot);
	void BindIndexBuffer(IndexBuffer* indexBuffer);

private:
	RendererConfig m_config;

	Shader*  m_defaultShader  = nullptr;
	Texture* m_defaultTexture = nullptr;

	Camera* m_currentCamera = nullptr;
	Shader* m_currentShader = nullptr;

	VertexBuffer*   m_currentVertexBuffer       = nullptr;
	IndexBuffer*    m_currentIndexBuffer        = nullptr;
	ConstantBuffer* m_lightConstantBuffer       = nullptr;
	ConstantBuffer* m_cameraConstantBuffer      = nullptr;
	ConstantBuffer* m_modelConstantBuffer       = nullptr;
	ConstantBuffer* m_postProcessConstantBuffer = nullptr;

	ID3D11Device*              m_d3dDevice           = nullptr;
	ID3D11DeviceContext*       m_d3dDeviceContext    = nullptr;
	IDXGISwapChain*            m_d3dSwapChain        = nullptr;
	ID3D11RenderTargetView*    m_d3dRenderTargetView = nullptr;
	ID3DUserDefinedAnnotation* m_d3dAnnotation       = nullptr;

	ID3D11BlendState* m_currentBlendState                  = nullptr;
	BlendMode         m_desiredBlendMode                   = BlendMode::ALPHA;
	ID3D11BlendState* m_blendStates[(int)BlendMode::COUNT] = {};

	ID3D11SamplerState* m_currentSamplerState                      = nullptr;
	SamplerMode         m_desiredSamplerMode                       = SamplerMode::POINT_CLAMP;
	ID3D11SamplerState* m_samplerStates[(int)(SamplerMode::COUNT)] = {};

	ID3D11RasterizerState* m_currentRasterizerState                         = nullptr;
	RasterizerMode         m_desiredRasterizerMode                          = RasterizerMode::SOLID_CULL_BACK;
	ID3D11RasterizerState* m_rasterizerStates[(int)(RasterizerMode::COUNT)] = {};

	ID3D11DepthStencilState* m_currentDepthStencilState                    = nullptr;
	DepthMode                m_desiredDepthMode                            = DepthMode::READ_WRITE_LESS_EQUAL;
	ID3D11DepthStencilState* m_depthStencilStates[(int)(DepthMode::COUNT)] = {};

	std::vector<Shader*> m_cachedShaders;
	std::vector<uint8_t> m_vertexShaderByteCode;
	std::vector<uint8_t> m_pixelShaderByteCode;

	std::map<std::string, Texture*>    m_texturesByName;
	std::map<std::string, BitmapFont*> m_fontsByName;

	// Post-process pass resources
	Texture* m_sceneColorTexture     = nullptr;
	Texture* m_sceneDepthTexture     = nullptr;
	Texture* m_sceneNormalTexture    = nullptr;
	Texture* m_postProcessTextureA   = nullptr;
	Texture* m_postProcessTextureB   = nullptr;
	Shader*  m_postProcessCopyShader = nullptr;

	std::vector<PostProcessPass> m_postProcessPasses;

#if defined(ENGINE_DEBUG_RENDER)
	void* m_dxgiDebug       = nullptr;
	void* m_dxgiDebugModule = nullptr;
#endif
};
