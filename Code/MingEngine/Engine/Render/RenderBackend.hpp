#pragma once

#include "MingEngine/Core/Math/IntVec2.hpp"
#include "MingEngine/Engine/Render/RenderTypes.hpp"

#include "ThirdParty/imgui/imgui.h"

#include <string>

class CameraContext;
class GPUTexture;
class Shader;
class VertexBuffer;
class IndexBuffer;
class ConstantBuffer;

struct Color;

class RenderBackend
{
public:
	virtual ~RenderBackend() = default;

	virtual void Startup()                = 0;
	virtual void Shutdown()               = 0;
	virtual void BeginFrame()             = 0;
	virtual void EndFrame()               = 0;
	virtual void CreateRenderingContext() = 0;

	virtual void BindCamera(CameraContext const& camera) = 0;

	virtual void SetBlendMode(BlendMode mode)           = 0;
	virtual void SetRasterizerMode(RasterizerMode mode) = 0;
	virtual void SetDepthMode(DepthMode mode)           = 0;

	virtual void DrawVertexBuffer(VertexBuffer* buffer)                                = 0;
	virtual void DrawIndexedVertexBuffer(VertexBuffer* vertices, IndexBuffer* indices) = 0;

	virtual void BindTexture(GPUTexture* texture, unsigned int slot)  = 0;
	virtual void BindSampler(SamplerMode mode, unsigned int slot = 0) = 0;
	virtual void BindShader(Shader* shader)                           = 0;

	virtual void            BindConstantBuffer(ConstantBuffer* buffer, int slot)     = 0;
	virtual ConstantBuffer* GetBuiltinConstantBuffer(BuiltinConstantBufferType type) = 0;

	virtual Shader* CreateShader(std::string const& name, std::string const& source, std::string const& sourcePath) = 0;

	virtual GPUTexture* CreateGPUTexture(
		char const* name, IntVec2 dimensions, int bytesPerTexel, uint8_t const* data) = 0;

	virtual GPUTexture* CreateRenderTargetTexture(char const* name, IntVec2 dimensions) = 0;
	virtual GPUTexture* CreateDepthStencilTexture(char const* name, IntVec2 dimensions) = 0;
	virtual void        DestroyTexture(GPUTexture* texture)                             = 0;

	virtual VertexBuffer* CreateVertexBuffer(unsigned int size, unsigned int stride)                   = 0;
	virtual VertexBuffer* CreateVertexBuffer(void const* data, unsigned int size, unsigned int stride) = 0;

	virtual IndexBuffer* CreateIndexBuffer(unsigned int size)                                        = 0;
	virtual IndexBuffer* CreateIndexBuffer(void const* data, unsigned int size, unsigned int stride) = 0;

	virtual ConstantBuffer* CreateConstantBuffer(unsigned int size) = 0;

	virtual void CopyCPUToGPU(void const* data, unsigned int size, VertexBuffer* buffer)   = 0;
	virtual void CopyCPUToGPU(void const* data, unsigned int size, IndexBuffer* buffer)    = 0;
	virtual void CopyCPUToGPU(void const* data, unsigned int size, ConstantBuffer* buffer) = 0;

	virtual void SetViewport(IntVec2 dimensions, IntVec2 topLeft = IntVec2::Zero) = 0;
	virtual void ResizeBackBuffer(IntVec2 dimensions)                             = 0;

	virtual void ClearRenderTarget(GPUTexture* target, Color const& color) = 0;
	virtual void ClearDepthStencil(GPUTexture* depth)                      = 0;

	virtual void BindRenderTargets(GPUTexture* color, GPUTexture* depth, GPUTexture* normal) = 0;
	virtual void BindRenderTarget(GPUTexture* color, GPUTexture* depth = nullptr)            = 0;

	virtual void BindPostProcessInputs(GPUTexture* color, GPUTexture* depth, GPUTexture* normal) = 0;
	virtual void DrawFullscreenTriangle(Shader* shader, wchar_t const* eventName)                = 0;
	virtual void UnbindAllShaderResourceViews()                                                  = 0;
	virtual void BindBackBuffer()                                                                = 0;

	virtual bool        InitImGui()                                  = 0;
	virtual void        ShutdownImGui()                              = 0;
	virtual void        BeginImGuiFrame()                            = 0;
	virtual void        RenderImGui(ImDrawData* drawData)            = 0;
	virtual ImTextureID GetImGuiTextureID(GPUTexture* texture) const = 0;

	template <typename T>
	void UpdateConstantBuffer(ConstantBuffer* buffer, T const& data)
	{
		CopyCPUToGPU(&data, static_cast<unsigned int>(sizeof(T)), buffer);
	}

	template <typename T>
	void UpdateAndBindConstantBuffer(BuiltinConstantBufferType type, T const& data)
	{
		int const       index  = static_cast<int>(type);
		ConstantBuffer* buffer = GetBuiltinConstantBuffer(type);

		UpdateConstantBuffer(buffer, data);
		BindConstantBuffer(buffer, BuiltinConstantBufferDescs[index].slot);
	}
};