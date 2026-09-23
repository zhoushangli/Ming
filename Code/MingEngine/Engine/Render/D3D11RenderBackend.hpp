#pragma once

#include "MingEngine/Engine/Application/EngineBuildPreferences.hpp"

#include "MingEngine/Core/Image.hpp"
#include "MingEngine/Core/Math/IntVec2.hpp"
#include "MingEngine/Core/Math/Matrix4x4.hpp"
#include "MingEngine/Engine/Event/EventSystem.hpp"
#include "MingEngine/Engine/Render/PostProcessChain.hpp"
#include "MingEngine/Engine/Render/RenderBackend.hpp"
#include "MingEngine/Engine/Render/RenderTypes.hpp"
#include "MingEngine/Engine/Render/Shader.hpp"
#include "MingEngine/Engine/Render/TextureBindingSlots.hpp"

#include <string>
#include <vector>

class Projection;
class GPUTexture;

class VertexBuffer;
class IndexBuffer;
class ConstantBuffer;

struct Vector2;
struct Color;
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

class D3D11RenderBackend : public RenderBackend
{
public:
	D3D11RenderBackend(RendererServerConfig config);
	~D3D11RenderBackend();

	// Lifetime and frame loop
	void Startup() override;
	void Shutdown() override;
	void BeginFrame() override;
	void EndFrame() override;
	void CreateRenderingContext();

	// We still need bind camera
	// because we need to bind both world camera and UI camera in one render
	void BindCamera(Projection const& projection);

	void ClearScreen(Color const& clearColor);
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
	void BindTexture(GPUTexture* textureOrNull);
	void BindTexture(GPUTexture* textureOrNull, unsigned int slot);
	void BindSampler(SamplerMode samplerMode, unsigned int slot = 0);
	void BindShader(Shader* shader);

	void            BindConstantBuffer(ConstantBuffer* constantBuffer, int slot);
	ConstantBuffer* GetBuiltinConstantBuffer(BuiltinConstantBufferType id);

	// GPU resource creation
	Shader* CreateShader(
		std::string const& shaderName, std::string const& shaderSource, std::string const& shaderSourcePath);

	GPUTexture* CreateGPUTexture(char const* name, IntVec2 dimensions, int bytesPerTexel, uint8_t const* texelData);
	GPUTexture* CreateRenderTargetTexture(char const* name, IntVec2 dimensions);
	GPUTexture* CreateDepthStencilTexture(char const* name, IntVec2 dimensions);
	void        DestroyTexture(GPUTexture* texture);

	VertexBuffer*   CreateVertexBuffer(const unsigned int size, unsigned int stride);
	VertexBuffer*   CreateVertexBuffer(void const* data, const unsigned int size, unsigned int stride);
	ConstantBuffer* CreateConstantBuffer(const unsigned int size);
	IndexBuffer*    CreateIndexBuffer(const unsigned int size);
	IndexBuffer*    CreateIndexBuffer(void const* data, const unsigned int size, unsigned int stride);

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
	void                 ClearRenderTarget(GPUTexture* renderTarget, Color const& clearColor);
	void                 ClearDepthStencil(GPUTexture* depthTexture);
	void                 BindRenderTargets(GPUTexture* colorTarget, GPUTexture* depthTarget, GPUTexture* normalTarget);
	void                 BindRenderTarget(GPUTexture* colorTarget, GPUTexture* depthTarget = nullptr);
	void                 BindPostProcessInputs(GPUTexture* colorInput, GPUTexture* depthInput, GPUTexture* normalInput);
	void                 DrawFullscreenTriangle(Shader* shader, wchar_t const* eventName);
	void                 UnbindAllShaderResourceViews();
	void                 BindBackBuffer();

	// Drive the ImGui renderer backend with this backend's D3D11 device.
	// e.g. RenderImGui(ImGui::GetDrawData())
	bool        InitImGui() override;
	void        ShutdownImGui() override;
	void        BeginImGuiFrame() override;
	void        RenderImGui(ImDrawData* drawData) override;
	ImTextureID GetImGuiTextureID(GPUTexture* texture) const override;

	template <typename T>
	void UpdateConstantBuffer(ConstantBuffer* constantBuffer, const T& data)
	{
		CopyCPUToGPU(&data, sizeof(T), constantBuffer);
	}

	template <typename T>
	void UpdateAndBindConstantBuffer(BuiltinConstantBufferType id, T const& data)
	{
		int const                        index = (int)id;
		BuiltinConstantBufferDesc const& desc  = BuiltinConstantBufferDescs[index];

		ConstantBuffer* buffer = m_builtinConstantBuffers[index];
		UpdateConstantBuffer(buffer, data);
		BindConstantBuffer(buffer, desc.slot);
	}

private:
	// Texture cache internals
	GPUTexture* CreateTextureFromFile(char const* fileDataPath);

	// Shader creation internals
	bool CompileShaderToByteCode(
		std::vector<unsigned char>& outByteCode,
		char const*                 shaderPhysicalPath,
		char const*                 source,
		char const*                 entryPoint,
		char const*                 target);

	// Low-level buffer binding to D3D context
	void BindVertexBuffer(VertexBuffer* vertexBuffer);
	void BindIndexBuffer(IndexBuffer* indexBuffer);

	GPUTexture* CreateTextureInternal(
		char const*                            name,
		IntVec2                                dimensions,
		D3D11_TEXTURE2D_DESC const*            textureDesc,
		D3D11_SUBRESOURCE_DATA const*          initialData,
		D3D11_RENDER_TARGET_VIEW_DESC const*   rtvDesc = nullptr,
		D3D11_SHADER_RESOURCE_VIEW_DESC const* srvDesc = nullptr,
		D3D11_DEPTH_STENCIL_VIEW_DESC const*   dsvDesc = nullptr);

private:
	RendererServerConfig m_config;

	GPUTexture* m_defaultBlackTexture = nullptr;

	Shader* m_currentShader = nullptr;

	VertexBuffer*   m_currentVertexBuffer                                           = nullptr;
	IndexBuffer*    m_currentIndexBuffer                                            = nullptr;
	ConstantBuffer* m_builtinConstantBuffers[(int)BuiltinConstantBufferType::Count] = {};

	ID3D11Device*              m_d3dDevice           = nullptr;
	ID3D11DeviceContext*       m_d3dDeviceContext    = nullptr;
	IDXGISwapChain*            m_d3dSwapChain        = nullptr;
	ID3D11RenderTargetView*    m_d3dRenderTargetView = nullptr;
	ID3DUserDefinedAnnotation* m_d3dAnnotation       = nullptr;
	GPUTexture*                m_backBufferTexture   = nullptr;

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

	std::vector<uint8_t> m_vertexShaderByteCode;
	std::vector<uint8_t> m_pixelShaderByteCode;

#if defined(ENGINE_DEBUG_RENDER)
	void* m_dxgiDebug       = nullptr;
	void* m_dxgiDebugModule = nullptr;
#endif
};
