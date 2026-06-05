#include "Engine/Renderer/D3D11RenderBackend.hpp"

#include "Engine/Core/Engine.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/Vertex.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Renderer/ConstantBuffer.hpp"
#include "Engine/Renderer/IndexBuffer.hpp"
#include "Engine/Renderer/Texture.hpp"
#include "Engine/Renderer/VertexBuffer.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "ThirdParty/stb/stb_image.h"

#include <Windows.h>
#include <d3d11.h>
#include <d3d11_1.h>
#include <d3dcompiler.h>

#if defined(OPAQUE)
#undef OPAQUE
#endif

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

#if defined(ENGINE_DEBUG_RENDER)
#include "D3D11RenderBackend.hpp"
#include <dxgidebug.h>
#pragma comment(lib, "dxguid.lib")
#endif

HGLRC g_openGLRenderingContext = nullptr;

namespace
{
Vertex const* GetFullscreenTriangleTopLeftUV()
{
	static Vertex const fullscreenTriangle[3] = {
		Vertex(Vec3(-1.f, -1.f, 0.f), Rgba8::White, Vec2(0.f, 1.f)),
		Vertex(Vec3(3.f, -1.f, 0.f), Rgba8::White, Vec2(2.f, 1.f)),
		Vertex(Vec3(-1.f, 3.f, 0.f), Rgba8::White, Vec2(0.f, -1.f)),
	};

	return fullscreenTriangle;
}

} // namespace

// clang-format off
const uint8_t kDefaultWhiteTexture[16] = 
{
	0xFF, 0xFF, 0xFF, 0xFF, // (0,0)
	0xFF, 0xFF, 0xFF, 0xFF, // (1,0)
	0xFF, 0xFF, 0xFF, 0xFF, // (0,1)
	0xFF, 0xFF, 0xFF, 0xFF  // (1,1)
};

const uint8_t kDefaultBlackTexture[16] = 
{
	0x00, 0x00, 0x00, 0xFF, // (0,0)
	0x00, 0x00, 0x00, 0xFF, // (1,0)
	0x00, 0x00, 0x00, 0xFF, // (0,1)
	0x00, 0x00, 0x00, 0xFF  // (1,1)
};
// clang-format on

//------------------------------------------------------------------------------------------------
// Lifetime and frame loop
D3D11RenderBackend::D3D11RenderBackend(RendererConfig config) : m_config(config) {}

D3D11RenderBackend::~D3D11RenderBackend() {}

#pragma region Public: Lifetime and frame loop
void           D3D11RenderBackend::Startup()
{
	unsigned int deviceFlags = 0;
#if defined(ENGINE_DEBUG_RENDER)
	deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

#pragma region Startup: Create device and swap chain

	DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
	swapChainDesc.BufferDesc.Width     = g_engine->m_window->GetClientDimensions().x;
	swapChainDesc.BufferDesc.Height    = g_engine->m_window->GetClientDimensions().y;
	swapChainDesc.BufferDesc.Format    = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.SampleDesc.Count     = 1;
	swapChainDesc.BufferUsage          = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount          = 2;
	swapChainDesc.OutputWindow         = (HWND)g_engine->m_window->GetHwnd();
	swapChainDesc.Windowed             = true;
	swapChainDesc.SwapEffect           = DXGI_SWAP_EFFECT_FLIP_DISCARD;

	HRESULT hr;
	hr = D3D11CreateDeviceAndSwapChain(nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		NULL,
		deviceFlags,
		nullptr,
		0,
		D3D11_SDK_VERSION,
		&swapChainDesc,
		&m_d3dSwapChain,
		&m_d3dDevice,
		nullptr,
		&m_d3dDeviceContext);

	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("Could not create D3D 11 device and swap chain.");
	}

#pragma endregion

#pragma region Startup: Get back buffer texture

	ID3D11Texture2D* backBuffer = nullptr;
	hr = m_d3dSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("Could not get swap chain buffer.");
	}

	hr = m_d3dDevice->CreateRenderTargetView(backBuffer, NULL, &m_d3dRenderTargetView);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("Could create render target view for swap chain buffer.");
	}
	backBuffer->Release();

#pragma endregion

#pragma region Startup: Create debug module

#if defined(ENGINE_DEBUG_RENDER)
	m_dxgiDebugModule = (void*)::LoadLibraryA("dxgidebug.dll");
	if (m_dxgiDebugModule == nullptr)
	{
		ERROR_AND_DIE("Could not load dxgidebug.dll.");
	}

	typedef HRESULT(WINAPI * GetDebugModuleCB)(REFIID, void**);
	((GetDebugModuleCB)::GetProcAddress((HMODULE)m_dxgiDebugModule, "DXGIGetDebugInterface"))(__uuidof(IDXGIDebug),
		&m_dxgiDebug);

	if (m_dxgiDebug == nullptr)
	{
		ERROR_AND_DIE("Could not load debug module.");
	}
#endif

#pragma endregion

#pragma region Startup: Rasterizer states

	// SOLID CULL NONE
	D3D11_RASTERIZER_DESC rasterizerDesc = {};
	rasterizerDesc.FillMode              = D3D11_FILL_SOLID;
	rasterizerDesc.CullMode              = D3D11_CULL_NONE;
	rasterizerDesc.FrontCounterClockwise = true;
	rasterizerDesc.DepthClipEnable       = true;
	rasterizerDesc.AntialiasedLineEnable = true;
	hr = m_d3dDevice->CreateRasterizerState(&rasterizerDesc, &m_rasterizerStates[(int)RasterizerMode::SOLID_CULL_NONE]);

	// SOLID CULL BACK
	rasterizerDesc.FillMode = D3D11_FILL_SOLID;
	rasterizerDesc.CullMode = D3D11_CULL_BACK;
	hr = m_d3dDevice->CreateRasterizerState(&rasterizerDesc, &m_rasterizerStates[(int)RasterizerMode::SOLID_CULL_BACK]);

	// WIREFRAME CULL NONE
	rasterizerDesc.FillMode = D3D11_FILL_WIREFRAME;
	rasterizerDesc.CullMode = D3D11_CULL_NONE;
	hr                      = m_d3dDevice->CreateRasterizerState(&rasterizerDesc,
		&m_rasterizerStates[(int)RasterizerMode::WIREFRAME_CULL_NONE]);

	// WIREFRAME CULL BACK
	rasterizerDesc.FillMode = D3D11_FILL_WIREFRAME;
	rasterizerDesc.CullMode = D3D11_CULL_BACK;
	hr                      = m_d3dDevice->CreateRasterizerState(&rasterizerDesc,
		&m_rasterizerStates[(int)RasterizerMode::WIREFRAME_CULL_BACK]);

	m_d3dDeviceContext->RSSetState(m_rasterizerStates[(int)RasterizerMode::SOLID_CULL_BACK]);

	m_d3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

#pragma endregion

#pragma region Startup: Create buffers

	m_currentVertexBuffer = CreateVertexBuffer(sizeof(Vertex) * 3, sizeof(Vertex));
	m_currentIndexBuffer  = CreateIndexBuffer(sizeof(unsigned int) * 3);

	for (int i = 0; i < (int)BuiltinConstantBufferType::Count; i++)
	{
		BuiltinConstantBufferDesc const& desc = kBuiltinConstantBufferDescs[i];
		m_builtinConstantBuffers[i]           = CreateConstantBuffer(desc.size);
	}

#pragma endregion

#pragma region Startup: Create blend states

	D3D11_BLEND_DESC blendDesc                      = {};
	blendDesc.RenderTarget[0].BlendEnable           = TRUE;
	blendDesc.RenderTarget[0].SrcBlend              = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlend             = D3D11_BLEND_ZERO;
	blendDesc.RenderTarget[0].BlendOp               = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha         = blendDesc.RenderTarget[0].SrcBlend;
	blendDesc.RenderTarget[0].DestBlendAlpha        = blendDesc.RenderTarget[0].DestBlend;
	blendDesc.RenderTarget[0].BlendOpAlpha          = blendDesc.RenderTarget[0].BlendOp;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	// OPAQUE
	hr = m_d3dDevice->CreateBlendState(&blendDesc, &m_blendStates[(int)(BlendMode::OPAQUE)]);

	// ALPHA
	blendDesc.RenderTarget[0].SrcBlend  = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;

	hr = m_d3dDevice->CreateBlendState(&blendDesc, &m_blendStates[(int)(BlendMode::ALPHA)]);

	// ADDITIVE
	blendDesc.RenderTarget[0].SrcBlend  = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;

	hr = m_d3dDevice->CreateBlendState(&blendDesc, &m_blendStates[(int)(BlendMode::ADDITIVE)]);

#pragma endregion

#pragma region Startup: Create sampler states

	D3D11_SAMPLER_DESC samplerDesc = {};

	// POINT_CLAMP
	samplerDesc.Filter         = D3D11_FILTER_MIN_MAG_MIP_POINT;
	samplerDesc.AddressU       = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV       = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW       = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	samplerDesc.MaxLOD         = D3D11_FLOAT32_MAX;

	hr = m_d3dDevice->CreateSamplerState(&samplerDesc, &m_samplerStates[(int)SamplerMode::POINT_CLAMP]);

	// BILINEAR_CLAMP
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;

	hr = m_d3dDevice->CreateSamplerState(&samplerDesc, &m_samplerStates[(int)SamplerMode::BILINEAR_CLAMP]);

	// POINT_WRAP
	samplerDesc.Filter   = D3D11_FILTER_MIN_MAG_MIP_POINT;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;

	hr = m_d3dDevice->CreateSamplerState(&samplerDesc, &m_samplerStates[(int)SamplerMode::POINT_WRAP]);

	// BILINEAR_WRAP
	samplerDesc.Filter   = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;

	hr = m_d3dDevice->CreateSamplerState(&samplerDesc, &m_samplerStates[(int)SamplerMode::BILINEAR_WRAP]);

#pragma endregion

#pragma region Startup: Create depth stencil state

	// 	// Create depth stencil texture and view
	// 	D3D11_TEXTURE2D_DESC depthTextureDesc = {};
	// 	depthTextureDesc.Width                = g_engine->m_window->GetClientDimensions().x;
	// 	depthTextureDesc.Height               = g_engine->m_window->GetClientDimensions().y;
	// 	depthTextureDesc.MipLevels            = 1;
	// 	depthTextureDesc.ArraySize            = 1;
	// 	depthTextureDesc.Usage                = D3D11_USAGE_DEFAULT;
	// 	depthTextureDesc.Format               = DXGI_FORMAT_D24_UNORM_S8_UINT;
	// 	depthTextureDesc.BindFlags            = D3D11_BIND_DEPTH_STENCIL;
	// 	depthTextureDesc.SampleDesc.Count     = 1;
	//
	// 	hr = m_d3dDevice->CreateTexture2D(&depthTextureDesc, nullptr, &m_depthStencilTexture);
	// 	hr = m_d3dDevice->CreateDepthStencilView(m_depthStencilTexture, nullptr, &m_depthStencilView);

	// DISABLED
	D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {};
	hr = m_d3dDevice->CreateDepthStencilState(&depthStencilDesc, &m_depthStencilStates[(int)DepthMode::DISABLED]);

	// READ_ONLY_ALWAYS
	depthStencilDesc.DepthEnable    = TRUE;
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	depthStencilDesc.DepthFunc      = D3D11_COMPARISON_ALWAYS;

	hr = m_d3dDevice->CreateDepthStencilState(&depthStencilDesc,
		&m_depthStencilStates[(int)DepthMode::READ_ONLY_ALWAYS]);

	// READ_ONLY_LESS_EQUAL
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	depthStencilDesc.DepthFunc      = D3D11_COMPARISON_LESS_EQUAL;

	hr = m_d3dDevice->CreateDepthStencilState(&depthStencilDesc,
		&m_depthStencilStates[(int)DepthMode::READ_ONLY_LESS_EQUAL]);

	// READ_WRITE_LESS_EQUAL
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.DepthFunc      = D3D11_COMPARISON_LESS_EQUAL;

	hr = m_d3dDevice->CreateDepthStencilState(&depthStencilDesc,
		&m_depthStencilStates[(int)DepthMode::READ_WRITE_LESS_EQUAL]);

#pragma endregion

#pragma region Startup: Create default shader

	m_defaultShader = CreateOrGetShader("Data/Shaders/DefaultUnlit");
	BindShader(m_defaultShader);

#pragma endregion

#pragma region Startup: Create default texture

	m_defaultWhiteTexture = CreateTextureFromData("DefaultWhite", IntVec2(2, 2), 4, (uint8_t*)kDefaultWhiteTexture);
	m_defaultBlackTexture = CreateTextureFromData("DefaultBlack", IntVec2(2, 2), 4, (uint8_t*)kDefaultBlackTexture);
	BindTexture(m_defaultWhiteTexture);

#pragma endregion

#pragma region Startup: Create debug annotations

	m_d3dDeviceContext->QueryInterface(__uuidof(ID3DUserDefinedAnnotation), (void**)&m_d3dAnnotation);

#pragma endregion

#pragma region Startup: Register events

	RegisterEvent("WindowResized", D3D11RenderBackend::Event_WindowResized);

#pragma endregion
}

void D3D11RenderBackend::Shutdown()
{
	m_currentCamera = nullptr;
	m_currentShader = nullptr;

	if (m_d3dDeviceContext)
	{
		m_d3dDeviceContext->ClearState();
		m_d3dDeviceContext->Flush();
	}

	for (ConstantBuffer*& buffer : m_builtinConstantBuffers)
	{
		delete buffer;
		buffer = nullptr;
	}

	delete m_currentIndexBuffer;
	m_currentIndexBuffer = nullptr;

	delete m_currentVertexBuffer;
	m_currentVertexBuffer = nullptr;

	for (auto& rasterizerState : m_rasterizerStates)
	{
		if (rasterizerState)
		{
			rasterizerState->Release();
			rasterizerState = nullptr;
		}
	}

	for (auto& depthStencilState : m_depthStencilStates)
	{
		if (depthStencilState)
		{
			depthStencilState->Release();
			depthStencilState = nullptr;
		}
	}

	for (auto& blendState : m_blendStates)
	{
		if (blendState)
		{
			blendState->Release();
			blendState = nullptr;
		}
	}

	for (auto& samplerState : m_samplerStates)
	{
		if (samplerState)
		{
			samplerState->Release();
			samplerState = nullptr;
		}
	}

	for (auto& shader : m_cachedShaders)
	{
		delete shader;
	}
	m_cachedShaders.clear();

	for (auto& pair : m_texturesByName)
	{
		delete pair.second;
	}
	m_texturesByName.clear();

	for (auto& pair : m_fontsByName)
	{
		delete pair.second;
	}
	m_fontsByName.clear();

	m_d3dAnnotation->Release();
	m_d3dRenderTargetView->Release();
	m_d3dSwapChain->Release();
	m_d3dDeviceContext->Release();
	m_d3dDevice->Release();

	// Report error leaks and release debug module
#if defined(ENGINE_DEBUG_RENDER)
	((IDXGIDebug*)m_dxgiDebug)
		->ReportLiveObjects(DXGI_DEBUG_ALL,
			(DXGI_DEBUG_RLO_FLAGS)(DXGI_DEBUG_RLO_DETAIL | DXGI_DEBUG_RLO_IGNORE_INTERNAL));

	((IDXGIDebug*)m_dxgiDebug)->Release();
	m_dxgiDebug = nullptr;

	::FreeLibrary((HMODULE)m_dxgiDebugModule);
	m_dxgiDebugModule = nullptr;
#endif
}

void D3D11RenderBackend::BeginFrame() {}

void D3D11RenderBackend::EndFrame()
{
	// Present
	HRESULT hr;
	hr = m_d3dSwapChain->Present(0, 0);
	if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
	{
		ERROR_AND_DIE("Device has been lost, application will now terminate.");
	}
}

void D3D11RenderBackend::CreateRenderingContext() {}

#pragma endregion

#pragma region Public: Camera and pipeline state

void D3D11RenderBackend::BindCamera(Camera const& camera)
{
	CameraConstants cameraData         = CameraConstants();
	cameraData.WorldToCameraTransform  = camera.GetWorldToCameraTransform();
	cameraData.CameraToRenderTransform = camera.GetCameraToRenderTransform();
	cameraData.RenderToClipTransform   = camera.GetRenderToClipTransform();
	cameraData.CameraToWorldTransform  = camera.GetCameraToWorldTransform();
	cameraData.ClipToCameraTransform   = camera.GetClipToCameraTransform();

	UpdateAndBindConstantBuffer(BuiltinConstantBufferType::Camera, cameraData);
}

void D3D11RenderBackend::ClearScreen(Rgba8 const& clearColor)
{
	float colorAsFloats[4];
	clearColor.GetAsFloats(colorAsFloats);

	m_d3dDeviceContext->ClearRenderTargetView(m_d3dRenderTargetView, colorAsFloats);
}

void D3D11RenderBackend::SetBlendMode(BlendMode blendMode) { m_desiredBlendMode = blendMode; }

void D3D11RenderBackend::SetRasterizerMode(RasterizerMode rasterizerMode) { m_desiredRasterizerMode = rasterizerMode; }

void D3D11RenderBackend::SetDepthMode(DepthMode depthMode) { m_desiredDepthMode = depthMode; }

void D3D11RenderBackend::SetStatesIfChanged()
{
	GUARANTEE_OR_DIE(m_d3dDeviceContext, "SetStatesIfChanged: m_d3dDeviceContext is null");

	// Blend state
	ID3D11BlendState* desiredBlendState = m_blendStates[(int)m_desiredBlendMode];
	if (m_currentBlendState != desiredBlendState)
	{
		m_currentBlendState = desiredBlendState;

		float blendFactor[4] = { 0.f, 0.f, 0.f, 0.f };
		UINT  sampleMask     = 0xffffffff;

		m_d3dDeviceContext->OMSetBlendState(m_currentBlendState, blendFactor, sampleMask);
	}

	ID3D11RasterizerState* desiredRasterizerState = m_rasterizerStates[(int)m_desiredRasterizerMode];
	if (m_currentRasterizerState != desiredRasterizerState)
	{
		m_currentRasterizerState = desiredRasterizerState;
		m_d3dDeviceContext->RSSetState(m_currentRasterizerState);
	}

	ID3D11DepthStencilState* desiredDepthStencilState = m_depthStencilStates[(int)m_desiredDepthMode];
	if (m_currentDepthStencilState != desiredDepthStencilState)
	{
		m_currentDepthStencilState = desiredDepthStencilState;
		m_d3dDeviceContext->OMSetDepthStencilState(m_currentDepthStencilState, 0);
	}
}

#pragma endregion

#pragma region Public: Draw entry points

void D3D11RenderBackend::DrawVertexArray(int numVertexes, Vertex const* vertexes)
{
	if (numVertexes % 3 != 0 || vertexes == nullptr)
	{
		return;
	}

	unsigned int size = numVertexes * sizeof(Vertex);

	m_currentVertexBuffer->Resize(size);
	CopyCPUToGPU(vertexes, size, m_currentVertexBuffer);
	DrawVertexBuffer(m_currentVertexBuffer);
}

void D3D11RenderBackend::DrawVertexArray(std::vector<Vertex> const& verts)
{
	DrawVertexArray(static_cast<int>(verts.size()), verts.data());
}

void D3D11RenderBackend::DrawVertexArray(std::vector<Vertex> const& verts, std::vector<unsigned int> const& vertIndexes)
{
	unsigned int vertsNum    = static_cast<unsigned int>(verts.size());
	unsigned int indexesNum  = static_cast<unsigned int>(vertIndexes.size());
	unsigned int vertsSize   = vertsNum * sizeof(Vertex);
	unsigned int indexesSize = indexesNum * sizeof(unsigned int);

	m_currentVertexBuffer->Resize(vertsSize);
	m_currentIndexBuffer->Resize(indexesSize);

	CopyCPUToGPU(verts.data(), vertsSize, m_currentVertexBuffer);
	CopyCPUToGPU(vertIndexes.data(), indexesSize, m_currentIndexBuffer);

	DrawIndexedVertexBuffer(m_currentVertexBuffer, m_currentIndexBuffer);
}

void D3D11RenderBackend::DrawVertexBuffer(VertexBuffer* vertexBuffer)
{
	SetStatesIfChanged();
	BindVertexBuffer(vertexBuffer);
	m_d3dDeviceContext->Draw(vertexBuffer->GetCount(), 0);
}

void D3D11RenderBackend::DrawIndexedVertexBuffer(VertexBuffer* vertexBuffer, IndexBuffer* indexBuffer)
{
	SetStatesIfChanged();
	BindVertexBuffer(vertexBuffer);
	BindIndexBuffer(indexBuffer);
	m_d3dDeviceContext->DrawIndexed(indexBuffer->GetCount(), 0, 0);
}

#pragma endregion

#pragma region Public: High-level bind helpers used by gameplay/render features

void D3D11RenderBackend::BindTexture(Texture* textureOrNull) { BindTexture(textureOrNull, 0); }

void D3D11RenderBackend::BindTexture(Texture* textureOrNull, unsigned int slot)
{
	GUARANTEE_OR_DIE(m_d3dDeviceContext, "BindTexture: m_d3dDeviceContext is null");

	if (textureOrNull == nullptr)
	{
		textureOrNull = m_defaultWhiteTexture;
	}

	ID3D11ShaderResourceView* srv = textureOrNull->m_shaderResourceView;
	m_d3dDeviceContext->PSSetShaderResources(slot, 1, &srv);
	m_d3dDeviceContext->VSSetShaderResources(slot, 1, &srv);
}

void D3D11RenderBackend::BindSampler(SamplerMode samplerMode, unsigned int slot)
{
	GUARANTEE_OR_DIE(m_d3dDeviceContext, "BindSampler: m_d3dDeviceContext is null");
	GUARANTEE_OR_DIE(slot < PostProcessTextureSlot::MaxSamplerSlots, "BindSampler: slot out of range");

	ID3D11SamplerState* samplerState = m_samplerStates[(int)samplerMode];
	if (m_currentSamplerStates[slot] != samplerState)
	{
		m_currentSamplerStates[slot] = samplerState;
		m_d3dDeviceContext->PSSetSamplers(slot, 1, &samplerState);
		m_d3dDeviceContext->VSSetSamplers(slot, 1, &samplerState);
	}
}

void D3D11RenderBackend::BindShader(Shader* shader)
{
	GUARANTEE_OR_DIE(m_d3dDeviceContext, "BindShader: m_d3dDeviceContext is null");

	if (shader == nullptr)
	{
		shader = m_defaultShader;
	}

	m_d3dDeviceContext->IASetInputLayout(shader->m_inputLayout);
	m_d3dDeviceContext->VSSetShader(shader->m_vertexShader, nullptr, 0);
	m_d3dDeviceContext->PSSetShader(shader->m_pixelShader, nullptr, 0);
}

#pragma endregion

#pragma region Public: GPU resource creation and cache access

Shader* D3D11RenderBackend::CreateOrGetShader(char const* shaderName)
{
	GUARANTEE_OR_DIE(shaderName && shaderName[0], "CreateShader(shaderName): shaderName is null/empty");

	std::string shaderKey = shaderName;
	for (Shader* shader : m_cachedShaders)
	{
		if (shader->GetName() == shaderKey)
		{
			return shader;
		}
	}

	std::string shaderFilename = std::string(shaderName) + ".hlsl";

	std::string shaderSource;
	int         bytesRead = FileReadToString(shaderSource, shaderFilename);

	GUARANTEE_OR_DIE(bytesRead > 0, Stringf("Failed to read shader file \"%s\"", shaderFilename.c_str()));

	return CreateShader(shaderName, shaderSource.c_str());
}

Texture* D3D11RenderBackend::CreateOrGetTexture(char const* imageFilePath)
{
	// See if we already have this texture previously loaded
	Texture* existingTexture = GetTextureFromFileName(imageFilePath); // You need to write this
	if (existingTexture)
	{
		return existingTexture;
	}

	// Never seen this texture before!  Let's load it.
	Texture* newTexture = CreateTextureFromFile(imageFilePath);
	return newTexture;
}

Texture* D3D11RenderBackend::CreateTextureFromImage(const Image& image)
{
	return CreateTextureFromData(image.GetImageFilePath().c_str(),
		image.GetDimensions(),
		4,
		(uint8_t*)image.GetRawData());
}

Texture* D3D11RenderBackend::CreateTextureFromData(
	char const* name, IntVec2 dimensions, int bytesPerTexel, uint8_t* texelData)
{
	// We only support RGBA8 format for now, so require 4 bytes per texel
	GUARANTEE_OR_DIE(bytesPerTexel == 4,
		Stringf("CreateTextureFromData requires 4 bytes/texel (RGBA). Got %i for \"%s\"", bytesPerTexel, name));

	D3D11_TEXTURE2D_DESC textureDesc = {};
	textureDesc.Width                = (UINT)dimensions.x;
	textureDesc.Height               = (UINT)dimensions.y;
	textureDesc.MipLevels            = 1;
	textureDesc.ArraySize            = 1;
	textureDesc.Format               = DXGI_FORMAT_R8G8B8A8_UNORM;
	textureDesc.Usage                = D3D11_USAGE_IMMUTABLE;
	textureDesc.BindFlags            = D3D11_BIND_SHADER_RESOURCE;
	textureDesc.SampleDesc.Count     = 1;

	D3D11_SUBRESOURCE_DATA textureData = {};
	textureData.pSysMem                = texelData;
	textureData.SysMemPitch            = 4 * dimensions.x;

	Texture* newTexture = CreateTextureInternal(name, dimensions, &textureDesc, &textureData);

	return newTexture;
}

Texture* D3D11RenderBackend::CreateRenderTargetTexture(char const* name, IntVec2 dimensions)
{
	D3D11_TEXTURE2D_DESC textureDesc = {};
	textureDesc.Width                = (UINT)dimensions.x;
	textureDesc.Height               = (UINT)dimensions.y;
	textureDesc.MipLevels            = 1;
	textureDesc.ArraySize            = 1;
	textureDesc.Format               = DXGI_FORMAT_R8G8B8A8_UNORM;
	textureDesc.Usage                = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags            = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	textureDesc.SampleDesc.Count     = 1;

	Texture* newTexture = CreateTextureInternal(name, dimensions, &textureDesc, nullptr);

	return newTexture;
}

Texture* D3D11RenderBackend::CreateDepthStencilTexture(char const* name, IntVec2 dimensions)
{
	D3D11_TEXTURE2D_DESC depthTextureDesc = {};
	depthTextureDesc.Width                = (UINT)dimensions.x;
	depthTextureDesc.Height               = (UINT)dimensions.y;
	depthTextureDesc.MipLevels            = 1;
	depthTextureDesc.ArraySize            = 1;
	depthTextureDesc.Usage                = D3D11_USAGE_DEFAULT;
	depthTextureDesc.Format               = DXGI_FORMAT_R24G8_TYPELESS;
	depthTextureDesc.BindFlags            = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	depthTextureDesc.SampleDesc.Count     = 1;

	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format                        = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.ViewDimension                 = D3D11_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Texture2D.MipSlice            = 0;

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format                          = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	srvDesc.ViewDimension                   = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip       = 0;
	srvDesc.Texture2D.MipLevels             = 1;

	Texture* newTexture =
		CreateTextureInternal(name, dimensions, &depthTextureDesc, nullptr, nullptr, &srvDesc, &dsvDesc);

	return newTexture;
}

void D3D11RenderBackend::DestroyTexture(Texture* texture)
{
	if (texture)
	{
		std::string textureName       = texture->m_name;
		m_texturesByName[textureName] = nullptr;

		delete texture;
	}
}

BitmapFont* D3D11RenderBackend::CreateOrGetBitmapFont(char const* fontFilePathNameWithNoExtension)
{
	std::string fontKey = std::string(fontFilePathNameWithNoExtension);
	auto        found   = m_fontsByName.find(fontKey);
	if (found != m_fontsByName.end())
	{
		return found->second;
	}

	Texture*    fontTexture   = CreateOrGetTexture(Stringf("%s.png", fontFilePathNameWithNoExtension).c_str());
	BitmapFont* newBitmapFont = new BitmapFont(fontFilePathNameWithNoExtension, *fontTexture);
	m_fontsByName[fontKey]    = newBitmapFont;
	return newBitmapFont;
}

VertexBuffer* D3D11RenderBackend::CreateVertexBuffer(const unsigned int size, unsigned int stride)
{
	return new VertexBuffer(m_d3dDevice, size, stride);
}

VertexBuffer* D3D11RenderBackend::CreateVertexBuffer(std::vector<Vertex> const& verts)
{
	if (verts.empty())
	{
		return nullptr;
	}

	unsigned int const  vertsSize    = static_cast<unsigned int>(verts.size()) * sizeof(Vertex);
	VertexBuffer* const vertexBuffer = CreateVertexBuffer(vertsSize, sizeof(Vertex));
	CopyCPUToGPU(verts.data(), vertsSize, vertexBuffer);
	return vertexBuffer;
}

ConstantBuffer* D3D11RenderBackend::CreateConstantBuffer(const unsigned int size)
{
	return new ConstantBuffer(m_d3dDevice, size);
}

IndexBuffer* D3D11RenderBackend::CreateIndexBuffer(const unsigned int size)
{
	return new IndexBuffer(m_d3dDevice, size);
}

IndexBuffer* D3D11RenderBackend::CreateIndexBuffer(std::vector<unsigned int> const& indexes)
{
	if (indexes.empty())
	{
		return nullptr;
	}

	unsigned int const indexesSize = static_cast<unsigned int>(indexes.size()) * sizeof(unsigned int);
	IndexBuffer* const indexBuffer = CreateIndexBuffer(indexesSize);
	CopyCPUToGPU(indexes.data(), indexesSize, indexBuffer);
	return indexBuffer;
}

#pragma endregion

#pragma region Public: CPU -> GPU uploads

void D3D11RenderBackend::CopyCPUToGPU(const void* data, unsigned int size, VertexBuffer* vertexBuffer)
{
	GUARANTEE_OR_DIE(m_d3dDeviceContext, "CopyCPUToGPU: m_d3dDeviceContext is null");
	GUARANTEE_OR_DIE(vertexBuffer, "CopyCPUToGPU: vertexBuffer is null");
	GUARANTEE_OR_DIE(data, "CopyCPUToGPU: data is null");
	GUARANTEE_OR_DIE(size > 0, "CopyCPUToGPU: size is 0");

	if (vertexBuffer->m_size > 0)
	{
		GUARANTEE_OR_DIE(size <= vertexBuffer->m_size, "CopyCPUToGPU: upload size exceeds vertex buffer capacity");
	}

	D3D11_MAPPED_SUBRESOURCE mapped = {};
	HRESULT hr = m_d3dDeviceContext->Map(vertexBuffer->m_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
	GUARANTEE_OR_DIE(SUCCEEDED(hr), "CopyCPUToGPU: Map failed");

	memcpy(mapped.pData, data, size);

	m_d3dDeviceContext->Unmap(vertexBuffer->m_buffer, 0);
}

void D3D11RenderBackend::CopyCPUToGPU(const void* data, unsigned int size, ConstantBuffer* constantBuffer)
{
	GUARANTEE_OR_DIE(m_d3dDeviceContext, "CopyCPUToGPU: m_d3dDeviceContext is null");
	GUARANTEE_OR_DIE(constantBuffer, "CopyCPUToGPU: vertexBuffer is null");
	GUARANTEE_OR_DIE(data, "CopyCPUToGPU: data is null");

	if (constantBuffer->m_size > 0)
	{
		GUARANTEE_OR_DIE(size <= constantBuffer->m_size, "CopyCPUToGPU: upload size exceeds constant buffer capacity");
	}

	D3D11_MAPPED_SUBRESOURCE mapped = {};
	HRESULT hr = m_d3dDeviceContext->Map(constantBuffer->m_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
	GUARANTEE_OR_DIE(SUCCEEDED(hr), "CopyCPUToGPU: Map failed");

	memcpy(mapped.pData, data, size);

	m_d3dDeviceContext->Unmap(constantBuffer->m_buffer, 0);
}

void D3D11RenderBackend::CopyCPUToGPU(const void* data, unsigned int size, IndexBuffer* indexBuffer)
{
	GUARANTEE_OR_DIE(m_d3dDeviceContext, "CopyCPUToGPU: m_d3dDeviceContext is null");
	GUARANTEE_OR_DIE(indexBuffer, "CopyCPUToGPU: vertexBuffer is null");
	GUARANTEE_OR_DIE(data, "CopyCPUToGPU: data is null");

	if (indexBuffer->m_size > 0)
	{
		GUARANTEE_OR_DIE(size <= indexBuffer->m_size, "CopyCPUToGPU: upload size exceeds constant buffer capacity");
	}

	D3D11_MAPPED_SUBRESOURCE mapped = {};
	HRESULT hr = m_d3dDeviceContext->Map(indexBuffer->m_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
	GUARANTEE_OR_DIE(SUCCEEDED(hr), "CopyCPUToGPU: Map failed");

	memcpy(mapped.pData, data, size);

	m_d3dDeviceContext->Unmap(indexBuffer->m_buffer, 0);
}

void D3D11RenderBackend::BeginEvent(std::string const& eventName)
{
	if (m_d3dAnnotation)
	{
		std::wstring wideEventName(eventName.begin(), eventName.end());
		m_d3dAnnotation->BeginEvent(wideEventName.c_str());
	}
}

void D3D11RenderBackend::EndEvent()
{
	if (m_d3dAnnotation)
	{
		m_d3dAnnotation->EndEvent();
	}
}

#pragma endregion

#pragma region Private: Texture cache internals

Texture* D3D11RenderBackend::CreateTextureFromFile(char const* imageFilePath)
{
	Image image(imageFilePath);

	return CreateTextureFromImage(image);
}

Texture* D3D11RenderBackend::GetTextureFromFileName(char const* imageFilePath)
{
	if (imageFilePath == nullptr)
	{
		return nullptr;
	}

	std::string filePath = std::string(imageFilePath);
	auto        found    = m_texturesByName.find(filePath);
	if (found != m_texturesByName.end())
	{
		return found->second;
	}

	return nullptr;
}

#pragma endregion

#pragma region Private: Shader creation internals

Shader* D3D11RenderBackend::CreateShader(char const* shaderName, char const* shaderSource)
{
	GUARANTEE_OR_DIE(m_d3dDevice, "CreateShader: m_d3dDevice is null");
	GUARANTEE_OR_DIE(shaderName && shaderName[0], "CreateShader: shaderName is null/empty");
	GUARANTEE_OR_DIE(shaderSource, "CreateShader: shaderSource is null");

	ShaderConfig config;
	config.m_name = shaderName;

	Shader* shader = new Shader(config);

	// Compile VS / PS
	std::vector<unsigned char> vsByteCode;
	std::vector<unsigned char> psByteCode;

	bool vsOK =
		CompileShaderToByteCode(vsByteCode, shaderName, shaderSource, config.m_vertexEntryPoint.c_str(), "vs_5_0");
	GUARANTEE_OR_DIE(vsOK, Stringf("Could not compile vertex shader for '%s'", shaderName));

	bool psOK =
		CompileShaderToByteCode(psByteCode, shaderName, shaderSource, config.m_pixelEntryPoint.c_str(), "ps_5_0");
	GUARANTEE_OR_DIE(psOK, Stringf("Could not compile pixel shader for '%s'", shaderName));

	// Create VS / PS
	HRESULT hr =
		m_d3dDevice->CreateVertexShader(vsByteCode.data(), vsByteCode.size(), nullptr, &shader->m_vertexShader);
	GUARANTEE_OR_DIE(SUCCEEDED(hr), Stringf("Could not create vertex shader for '%s'", shaderName));

	hr = m_d3dDevice->CreatePixelShader(psByteCode.data(), psByteCode.size(), nullptr, &shader->m_pixelShader);
	GUARANTEE_OR_DIE(SUCCEEDED(hr), Stringf("Could not create pixel shader for '%s'", shaderName));

	static D3D11_INPUT_ELEMENT_DESC const kPcutbnDesc[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	D3D11_INPUT_ELEMENT_DESC const* inputElementDesc  = kPcutbnDesc;
	UINT                            inputElementCount = (UINT)ARRAYSIZE(kPcutbnDesc);

	hr = m_d3dDevice->CreateInputLayout(inputElementDesc,
		inputElementCount,
		vsByteCode.data(),
		(UINT)vsByteCode.size(),
		&shader->m_inputLayout);
	GUARANTEE_OR_DIE(SUCCEEDED(hr), Stringf("Could not create input layout for '%s'", shaderName));

	m_cachedShaders.push_back(shader);

	return shader;
}

bool D3D11RenderBackend::CompileShaderToByteCode(std::vector<unsigned char>& outByteCode,
	char const*                                                              name,
	char const*                                                              source,
	char const*                                                              entryPoint,
	char const*                                                              target)
{
	if (source == nullptr || entryPoint == nullptr || target == nullptr)
	{
		return false;
	}

	DWORD shaderFlags = D3DCOMPILE_OPTIMIZATION_LEVEL3;
#if defined(ENGINE_DEBUG_RENDER)
	shaderFlags = D3DCOMPILE_DEBUG;
	shaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
	shaderFlags |= D3DCOMPILE_WARNINGS_ARE_ERRORS;
#endif

	ID3DBlob* shaderBlob = nullptr;
	ID3DBlob* errorBlob  = nullptr;

	HRESULT hr = D3DCompile(source,
		strlen(source),
		name,
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		entryPoint,
		target,
		shaderFlags,
		0,
		&shaderBlob,
		&errorBlob);

	if (!SUCCEEDED(hr))
	{
		if (errorBlob)
		{
			DebuggerPrintf((char*)errorBlob->GetBufferPointer());
			errorBlob->Release();
		}
		if (shaderBlob)
			shaderBlob->Release();
		return false;
	}

	outByteCode.resize(shaderBlob->GetBufferSize());
	memcpy(outByteCode.data(), shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize());

	shaderBlob->Release();
	if (errorBlob)
		errorBlob->Release();
	return true;
}

#pragma endregion

#pragma region Private: Low-level buffer binding to D3D context

void D3D11RenderBackend::BindVertexBuffer(VertexBuffer* vertexBuffer)
{
	GUARANTEE_OR_DIE(m_d3dDeviceContext, "BindVertexBuffer: m_d3dDeviceContext is null");

	if (vertexBuffer == nullptr)
	{
		ID3D11Buffer* nullBuf = nullptr;
		UINT          stride  = 0;
		UINT          offset  = 0;
		m_d3dDeviceContext->IASetVertexBuffers(0, 1, &nullBuf, &stride, &offset);
		return;
	}

	UINT          stride = vertexBuffer->GetStride();
	UINT          offset = 0;
	ID3D11Buffer* buf    = vertexBuffer->m_buffer;
	m_d3dDeviceContext->IASetVertexBuffers(0, 1, &buf, &stride, &offset);
}

void D3D11RenderBackend::BindConstantBuffer(ConstantBuffer* constantBuffer, int slot)
{
	GUARANTEE_OR_DIE(m_d3dDeviceContext, "BindConstantBuffer: m_d3dDeviceContext is null");

	if (constantBuffer == nullptr)
	{
		ID3D11Buffer* nullBuf = nullptr;
		m_d3dDeviceContext->VSSetConstantBuffers(slot, 1, &nullBuf);
		m_d3dDeviceContext->PSSetConstantBuffers(slot, 1, &nullBuf);
		return;
	}

	ID3D11Buffer* buf = constantBuffer->m_buffer;
	GUARANTEE_OR_DIE(buf, "BindConstantBuffer: constantBuffer->m_buffer is null");

	m_d3dDeviceContext->VSSetConstantBuffers(slot, 1, &buf);
	m_d3dDeviceContext->PSSetConstantBuffers(slot, 1, &buf);
}

void D3D11RenderBackend::BindIndexBuffer(IndexBuffer* indexBuffer)
{
	GUARANTEE_OR_DIE(m_d3dDeviceContext, "BindVertexBuffer: m_d3dDeviceContext is null");

	if (indexBuffer == nullptr)
	{
		ID3D11Buffer* nullBuf = nullptr;
		UINT          stride  = 0;
		UINT          offset  = 0;
		m_d3dDeviceContext->IASetIndexBuffer(nullBuf, DXGI_FORMAT_R32_UINT, 0);
		return;
	}

	ID3D11Buffer* buf = indexBuffer->m_buffer;
	m_d3dDeviceContext->IASetIndexBuffer(buf, DXGI_FORMAT_R32_UINT, 0);
}

void D3D11RenderBackend::SetViewport(IntVec2 dimensions, IntVec2 topLeft)
{
	if (dimensions.x <= 0 || dimensions.y <= 0)
	{
		return;
	}

	D3D11_VIEWPORT viewport = {};
	viewport.TopLeftX       = (FLOAT)topLeft.x;
	viewport.TopLeftY       = (FLOAT)topLeft.y;
	viewport.Width          = (FLOAT)dimensions.x;
	viewport.Height         = (FLOAT)dimensions.y;
	viewport.MinDepth       = 0.0f;
	viewport.MaxDepth       = 1.0f;

	m_d3dDeviceContext->RSSetViewports(1, &viewport);
}

void D3D11RenderBackend::ResizeBackBuffer(IntVec2 newDimensions)
{
	if (newDimensions.x <= 0 || newDimensions.y <= 0)
	{
		return;
	}

	// 1) Unbind backbuffer RTV
	UnbindAllShaderResourceViews();

	if (m_d3dRenderTargetView != nullptr)
	{
		m_d3dRenderTargetView->Release();
		m_d3dRenderTargetView = nullptr;
	}

	HRESULT hr = m_d3dSwapChain->ResizeBuffers(0, newDimensions.x, newDimensions.y, DXGI_FORMAT_UNKNOWN, 0);

	GUARANTEE_OR_DIE(SUCCEEDED(hr), "Could not resize swap chain buffers.");

	ID3D11Texture2D* backBuffer = nullptr;

	hr = m_d3dSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);

	GUARANTEE_OR_DIE(SUCCEEDED(hr), "Could not get resized swap chain buffer.");

	hr = m_d3dDevice->CreateRenderTargetView(backBuffer, nullptr, &m_d3dRenderTargetView);

	backBuffer->Release();

	GUARANTEE_OR_DIE(SUCCEEDED(hr), "Could not create resized back buffer RTV.");
}

ID3D11Device* D3D11RenderBackend::GetD3DDevice() const { return m_d3dDevice; }

ID3D11DeviceContext* D3D11RenderBackend::GetD3DDeviceContext() const { return m_d3dDeviceContext; }

bool D3D11RenderBackend::Event_WindowResized(EventArgs& args) { return true; }

void D3D11RenderBackend::ResizeViewport(IntVec2 newDimensions) {}

void D3D11RenderBackend::ClearRenderTarget(Texture* renderTarget, Rgba8 const& clearColor)
{
	if (renderTarget == nullptr || renderTarget->m_renderTargetView == nullptr)
	{
		return;
	}

	float colorAsFloats[4];
	clearColor.GetAsFloats(colorAsFloats);
	m_d3dDeviceContext->ClearRenderTargetView(renderTarget->m_renderTargetView, colorAsFloats);
}

void D3D11RenderBackend::ClearDepthStencil(Texture* depthTexture)
{
	if (depthTexture == nullptr || depthTexture->m_depthStencilView == nullptr)
	{
		return;
	}

	m_d3dDeviceContext->ClearDepthStencilView(depthTexture->m_depthStencilView,
		D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
		1.0f,
		0);
}

void D3D11RenderBackend::BindRenderTargets(Texture* colorTarget, Texture* depthTarget, Texture* normalTarget)
{
	ID3D11RenderTargetView* renderTargetViews[2] = { nullptr, nullptr };

	if (colorTarget != nullptr)
	{
		renderTargetViews[0] = colorTarget->m_renderTargetView;
	}

	if (normalTarget != nullptr)
	{
		renderTargetViews[1] = normalTarget->m_renderTargetView;
	}

	ID3D11DepthStencilView* depthStencilView = depthTarget != nullptr ? depthTarget->m_depthStencilView : nullptr;
	m_d3dDeviceContext->OMSetRenderTargets(2, renderTargetViews, depthStencilView);
}

void D3D11RenderBackend::BindRenderTarget(Texture* colorTarget, Texture* depthTarget)
{
	ID3D11RenderTargetView* renderTargetView = colorTarget != nullptr ? colorTarget->m_renderTargetView : nullptr;
	ID3D11DepthStencilView* depthStencilView = depthTarget != nullptr ? depthTarget->m_depthStencilView : nullptr;
	m_d3dDeviceContext->OMSetRenderTargets(1, &renderTargetView, depthStencilView);
}

void D3D11RenderBackend::BindPostProcessInputs(Texture* colorInput, Texture* depthInput, Texture* normalInput)
{
	BindTexture(colorInput, 0);
	BindSampler(SamplerMode::POINT_CLAMP, 0);
	BindTexture(depthInput, 1);
	BindSampler(SamplerMode::POINT_CLAMP, 1);
	BindTexture(normalInput, 2);
	BindSampler(SamplerMode::POINT_CLAMP, 2);
}

void D3D11RenderBackend::DrawFullscreenTriangle(Shader* shader, wchar_t const* eventName)
{
	BindShader(shader);
	m_d3dAnnotation->BeginEvent(eventName);
	DrawVertexArray(3, GetFullscreenTriangleTopLeftUV());
	m_d3dAnnotation->EndEvent();
}

void D3D11RenderBackend::UnbindAllShaderResourceViews()
{
	ID3D11RenderTargetView*   nullRTV      = nullptr;
	ID3D11ShaderResourceView* nullSrvs[16] = { nullptr };
	m_d3dDeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
	m_d3dDeviceContext->OMSetRenderTargets(1, &nullRTV, nullptr);
	m_d3dDeviceContext->PSSetShaderResources(0, 16, nullSrvs);
	m_d3dDeviceContext->VSSetShaderResources(0, 16, nullSrvs);
}

void D3D11RenderBackend::BindBackBuffer()
{
	m_d3dDeviceContext->OMSetRenderTargets(1, &m_d3dRenderTargetView, nullptr);
}
Texture* D3D11RenderBackend::CreateTextureInternal(char const* name,
	IntVec2                                                    dimensions,
	D3D11_TEXTURE2D_DESC const*                                textureDesc,
	D3D11_SUBRESOURCE_DATA const*                              initialData,
	D3D11_RENDER_TARGET_VIEW_DESC const*                       rtvDesc,
	D3D11_SHADER_RESOURCE_VIEW_DESC const*                     srvDesc,
	D3D11_DEPTH_STENCIL_VIEW_DESC const*                       dsvDesc)
{
	GUARANTEE_OR_DIE(dimensions.x > 0 && dimensions.y > 0,
		Stringf("CreateTextureFromData failed for \"%s\" - illegal texture dimensions (%i x %i)",
			name,
			dimensions.x,
			dimensions.y));

	GUARANTEE_OR_DIE(textureDesc, Stringf("CreateTextureFromData failed for \"%s\" - textureDesc is null", name));

	Texture* newTexture      = new Texture();
	newTexture->m_name       = name;
	newTexture->m_dimensions = dimensions;

	HRESULT hr = m_d3dDevice->CreateTexture2D(textureDesc, initialData, &newTexture->m_texture);

	GUARANTEE_OR_DIE(SUCCEEDED(hr), "Could not create texture.");

	if ((textureDesc->BindFlags & D3D11_BIND_RENDER_TARGET) != 0)
	{
		m_d3dDevice->CreateRenderTargetView(newTexture->m_texture, rtvDesc, &newTexture->m_renderTargetView);
	}

	if ((textureDesc->BindFlags & D3D11_BIND_SHADER_RESOURCE) != 0)
	{
		m_d3dDevice->CreateShaderResourceView(newTexture->m_texture, srvDesc, &newTexture->m_shaderResourceView);
	}

	if ((textureDesc->BindFlags & D3D11_BIND_DEPTH_STENCIL) != 0)
	{
		m_d3dDevice->CreateDepthStencilView(newTexture->m_texture, dsvDesc, &newTexture->m_depthStencilView);
	}

	m_texturesByName[newTexture->m_name] = newTexture;

	return newTexture;
}

#pragma endregion
