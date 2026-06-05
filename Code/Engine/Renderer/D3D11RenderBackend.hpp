#pragma once

#include "Game/EngineBuildPreferences.hpp"

#include "Engine/Core/EventSystem.hpp"
#include "Engine/Core/Image.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/Matrix4x4.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Renderer/PostProcessChain.hpp"
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
struct ID3D11ShaderResourceView;
struct D3D11_TEXTURE2D_DESC;
struct D3D11_SUBRESOURCE_DATA;
struct D3D11_RENDER_TARGET_VIEW_DESC;
struct D3D11_SHADER_RESOURCE_VIEW_DESC;
struct D3D11_DEPTH_STENCIL_VIEW_DESC;

namespace SurfaceTextureSlot
{
static const unsigned int Diffuse = 0;
} // namespace SurfaceTextureSlot

namespace PostProcessTextureSlot
{
static const unsigned int kColor           = 0;
static const unsigned int kDepth           = 1;
static const unsigned int kNormal          = 2;
static const unsigned int CustomInputStart = 8;
static const unsigned int MaxSamplerSlots  = 16;
} // namespace PostProcessTextureSlot

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
	POINT_WRAP,
	BILINEAR_WRAP,
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

struct GPUDirectionalLight
{
	Vec3  m_direction;
	float m_intensity;
};

struct GPUPointLight
{
	Vec3  m_position;
	float m_range;
	Vec3  m_color;
	float m_intensity;
};
static const int kMaxPointLights = 64;

struct LightConstants
{
	GPUDirectionalLight m_directionalLight;
	int                 m_pointLightCount;
	float               m_padding[3]; // Pad to 16 bytes for array alignment
	GPUPointLight       m_pointLights[kMaxPointLights];
};
static const int kLightConstantsSlot = 1;

struct CameraConstants
{
	Matrix4x4 WorldToCameraTransform;
	Matrix4x4 CameraToRenderTransform;
	Matrix4x4 RenderToClipTransform;
	Matrix4x4 CameraToWorldTransform;
	Matrix4x4 ClipToCameraTransform;
};
static const int kCameraConstantsSlot = 2;

struct ModelConstants
{
	Matrix4x4 ModelToWorld;
	float     ModelColor[4];
};
static const int kModelConstantsSlot = 3;

struct PostProcessConstants
{
	Vec2  m_screenDimensions;
	float m_cameraNear;
	float m_cameraFar;
};
static const int kPostProcessConstantsSlot = 4;

struct FrameConstants
{
	float m_time;
	float m_deltaSeconds;
	float Padding[2];
};
static const int kFrameConstantsSlot = 5;

enum class BuiltinConstantBufferType
{
	Light,
	Camera,
	Model,
	PostProcess,
	Frame,
	Count
};

struct BuiltinConstantBufferDesc
{
	char const* name = nullptr;
	size_t      size = 0;
	int         slot = 0;
};

static BuiltinConstantBufferDesc const kBuiltinConstantBufferDescs[] = {
	{ "Light", sizeof(LightConstants), kLightConstantsSlot },
	{ "Camera", sizeof(CameraConstants), kCameraConstantsSlot },
	{ "Model", sizeof(ModelConstants), kModelConstantsSlot },
	{ "PostProcess", sizeof(PostProcessConstants), kPostProcessConstantsSlot },
	{ "Frame", sizeof(FrameConstants), kFrameConstantsSlot },
};

class D3D11RenderBackend
{
public:
	D3D11RenderBackend(RendererConfig config);
	~D3D11RenderBackend();

	// Lifetime and frame loop
	void Startup();
	void Shutdown();
	void BeginFrame();
	void EndFrame();
	void CreateRenderingContext();

	// We still need bind camera
	// because we need to bind both world camera and UI camera in one render
	void BindCamera(Camera const& camera);

	void ClearScreen(Rgba8 const& clearColor);
	void SetBlendMode(BlendMode blendMode);
	void SetRasterizerMode(RasterizerMode rasterizerMode);
	void SetDepthMode(DepthMode depthMode);
	void SetStatesIfChanged();

	// Draw entry points
	void DrawVertexArray(int numVertexes, Vertex const* vertexes);
	void DrawVertexArray(std::vector<Vertex> const& verts);
	void DrawVertexArray(std::vector<Vertex> const& verts, std::vector<unsigned int> const& vertIndexs);
	void DrawVertexBuffer(VertexBuffer* vertexBuffer);
	void DrawIndexedVertexBuffer(VertexBuffer* vertexBuffer, IndexBuffer* indexBuffer);

	// High-level bind helpers used by gameplay/render features
	void BindTexture(Texture* textureOrNull);
	void BindTexture(Texture* textureOrNull, unsigned int slot);
	void BindSampler(SamplerMode samplerMode, unsigned int slot = 0);
	void BindShader(Shader* shader);

	void            BindConstantBuffer(ConstantBuffer* constantBuffer, int slot);
	ConstantBuffer* GetBuiltinConstantBuffer(BuiltinConstantBufferType id);

	// GPU resource creation and cache access
	Shader* CreateOrGetShader(char const* shaderName);

	Texture* CreateOrGetTexture(char const* fileDataPath);
	Texture* CreateTextureFromImage(const Image& image);
	Texture* CreateTextureFromData(char const* name, IntVec2 dimensions, int bytesPerTexel, uint8_t* texelData);
	Texture* CreateRenderTargetTexture(char const* name, IntVec2 dimensions);
	Texture* CreateDepthStencilTexture(char const* name, IntVec2 dimensions);
	void     DestroyTexture(Texture* texture);

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
	void BeginEvent(std::string const& eventName);
	void EndEvent();

	ID3D11Device*        GetD3DDevice() const;
	ID3D11DeviceContext* GetD3DDeviceContext() const;
	void                 SetViewport(IntVec2 dimensions, IntVec2 topLeft = IntVec2::Zero);
	void                 ResizeBackBuffer(IntVec2 newDimensions);
	Texture*             GetTextureFromFileName(char const* fileName);
	void                 ClearRenderTarget(Texture* renderTarget, Rgba8 const& clearColor);
	void                 ClearDepthStencil(Texture* depthTexture);
	void                 BindRenderTargets(Texture* colorTarget, Texture* depthTarget, Texture* normalTarget);
	void                 BindRenderTarget(Texture* colorTarget, Texture* depthTarget = nullptr);
	void                 BindPostProcessInputs(Texture* colorInput, Texture* depthInput, Texture* normalInput);
	void                 DrawFullscreenTriangle(Shader* shader, wchar_t const* eventName);
	void                 UnbindAllShaderResourceViews();
	void                 BindBackBuffer();

	template <typename T>
	void UpdateConstantBuffer(ConstantBuffer* constantBuffer, const T& data)
	{
		CopyCPUToGPU(&data, sizeof(T), constantBuffer);
	}

	template <typename T>
	void UpdateAndBindConstantBuffer(BuiltinConstantBufferType id, T const& data)
	{
		int const                        index = (int)id;
		BuiltinConstantBufferDesc const& desc  = kBuiltinConstantBufferDescs[index];

		ConstantBuffer* buffer = m_builtinConstantBuffers[index];
		UpdateConstantBuffer(buffer, data);
		BindConstantBuffer(buffer, desc.slot);
	}

private:
	// Texture cache internals
	Texture* CreateTextureFromFile(char const* fileDataPath);

	// Shader creation internals
	Shader* CreateShader(char const* shaderName, char const* shaderSource);
	bool    CompileShaderToByteCode(std::vector<unsigned char>& outByteCode,
		char const*                                          name,
		char const*                                          source,
		char const*                                          entryPoint,
		char const*                                          target);

	// Low-level buffer binding to D3D context
	void BindVertexBuffer(VertexBuffer* vertexBuffer);
	void BindIndexBuffer(IndexBuffer* indexBuffer);

	static bool Event_WindowResized(EventArgs& args);
	void        ResizeViewport(IntVec2 newDimensions);

	Texture* CreateTextureInternal(char const* name,
		IntVec2                                dimensions,
		D3D11_TEXTURE2D_DESC const*            textureDesc,
		D3D11_SUBRESOURCE_DATA const*          initialData,
		D3D11_RENDER_TARGET_VIEW_DESC const*   rtvDesc = nullptr,
		D3D11_SHADER_RESOURCE_VIEW_DESC const* srvDesc = nullptr,
		D3D11_DEPTH_STENCIL_VIEW_DESC const*   dsvDesc = nullptr);

private:
	RendererConfig m_config;

	Shader*  m_defaultShader       = nullptr;
	Texture* m_defaultWhiteTexture = nullptr;
	Texture* m_defaultBlackTexture = nullptr;

	Camera* m_currentCamera = nullptr;
	Shader* m_currentShader = nullptr;

	VertexBuffer*   m_currentVertexBuffer                                           = nullptr;
	IndexBuffer*    m_currentIndexBuffer                                            = nullptr;
	ConstantBuffer* m_builtinConstantBuffers[(int)BuiltinConstantBufferType::Count] = {};

	ID3D11Device*              m_d3dDevice           = nullptr;
	ID3D11DeviceContext*       m_d3dDeviceContext    = nullptr;
	IDXGISwapChain*            m_d3dSwapChain        = nullptr;
	ID3D11RenderTargetView*    m_d3dRenderTargetView = nullptr;
	ID3DUserDefinedAnnotation* m_d3dAnnotation       = nullptr;
	Texture*                   m_backBufferTexture   = nullptr;

	ID3D11BlendState* m_currentBlendState                  = nullptr;
	BlendMode         m_desiredBlendMode                   = BlendMode::ALPHA;
	ID3D11BlendState* m_blendStates[(int)BlendMode::COUNT] = {};

	ID3D11SamplerState* m_currentSamplerStates[PostProcessTextureSlot::MaxSamplerSlots] = {};
	ID3D11SamplerState* m_samplerStates[(int)(SamplerMode::COUNT)]                      = {};

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

#if defined(ENGINE_DEBUG_RENDER)
	void* m_dxgiDebug       = nullptr;
	void* m_dxgiDebugModule = nullptr;
#endif
};
