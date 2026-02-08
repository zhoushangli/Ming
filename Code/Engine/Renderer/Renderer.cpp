#include "Engine/Renderer/Renderer.hpp"

#include "Engine/Core/Engine.hpp"
#include "Engine/Core/Vertex.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Renderer/Texture.hpp"
#include "Engine/Renderer/VertexBuffer.hpp"
#include "Engine/Renderer/ConstantBuffer.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "ThirdParty/stb/stb_image.h"

#include <Windows.h>
#include <d3dcompiler.h>
#include <d3d11.h>

#if defined(OPAQUE)
#undef OPAQUE
#endif


#pragma comment( lib, "d3d11.lib" )
#pragma comment( lib, "dxgi.lib" )
#pragma comment( lib, "d3dcompiler.lib" )

#if defined( ENGINE_DEBUG_RENDER )
#include <dxgidebug.h>
#pragma comment( lib, "dxguid.lib" )
#endif

HGLRC g_openGLRenderingContext = nullptr;

const char* k_defaultShaderSource = R"(
    cbuffer CameraConstants : register(b2)
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

    Texture2D    diffuseTexture : register(t0);
    SamplerState diffuseSampler : register(s0);

    struct VS_INPUT
    {
        float3 localPosition : POSITION;
        float4 color : COLOR;
        float2 uv : TEXCOORD0;
    };

    struct VS_OUTPUT
    {
        float4 position : SV_Position;
        float4 color : COLOR;
        float2 uv : TEXCOORD0;
    };

    float Interpolate(float start, float end, float fraction)
    {
        return start * (1.0f - fraction) + end * fraction;
    }

    float GetFractionWithinRange(float value, float start, float end)
    {
        return (value - start) / (end - start);
    }

    float RangeMap(float inValue, float inStart, float inEnd, float outStart, float outEnd)
    {
        float fraction = GetFractionWithinRange(inValue, inStart, inEnd);
        return Interpolate(outStart, outEnd, fraction);
    }

    VS_OUTPUT VertexMain(VS_INPUT input)
    {
        float4 localPosition = float4(input.localPosition, 1);

        float4 clipPosition;
        clipPosition.x = RangeMap(localPosition.x, OrthoMinX, OrthoMaxX, -1.0f, 1.0f);
        clipPosition.y = RangeMap(localPosition.y, OrthoMinY, OrthoMaxY, -1.0f, 1.0f);
        clipPosition.z = RangeMap(localPosition.z, OrthoMinZ, OrthoMaxZ,  0.0f, 1.0f);
        clipPosition.w = localPosition.w;

        VS_OUTPUT o;
        o.position = clipPosition;
        o.color    = input.color;
        o.uv       = input.uv;
        return o;
    }

    float4 PixelMain(VS_OUTPUT input) : SV_Target0
    {
        float4 textureColor = diffuseTexture.Sample(diffuseSampler, input.uv);
        float4 color = textureColor * input.color;
        clip(color.a - 0.01f);
        return color;
    }
)";


const uint8_t k_defaultTexture[16] =
{
    0xFF, 0xFF, 0xFF, 0xFF,   // (0,0)
    0xFF, 0xFF, 0xFF, 0xFF,   // (1,0)
    0xFF, 0xFF, 0xFF, 0xFF,   // (0,1)
    0xFF, 0xFF, 0xFF, 0xFF    // (1,1)
};

Renderer::Renderer(RendererConfig config) : m_config(config)
{

}

Renderer::~Renderer()
{
}

void Renderer::Startup()
{
    unsigned int deviceFlags = 0;
#if defined(ENGINE_DEBUG_RENDER)
    deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

#pragma region Create device and swap chain

    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    swapChainDesc.BufferDesc.Width = g_engine->m_window->GetClientDimensions().x;
    swapChainDesc.BufferDesc.Height = g_engine->m_window->GetClientDimensions().y;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 2;
    swapChainDesc.OutputWindow = (HWND)g_engine->m_window->GetHwnd();
    swapChainDesc.Windowed = true;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    HRESULT hr;
    hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, NULL, deviceFlags,
        nullptr, 0, D3D11_SDK_VERSION, &swapChainDesc,
        &m_swapChain, &m_device, nullptr, &m_deviceContext);

    if (!SUCCEEDED(hr))
    {
        ERROR_AND_DIE("Could not create D3D 11 device and swap chain.");
    }

#pragma endregion

#pragma region Get back buffer texture

    ID3D11Texture2D* backBuffer;
    hr = m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
    if (!SUCCEEDED(hr))
    {
        ERROR_AND_DIE("Could not get swap chain buffer.");
    }

    hr = m_device->CreateRenderTargetView(backBuffer, NULL, &m_renderTargetView);
    if (!SUCCEEDED(hr))
    {
        ERROR_AND_DIE("Could create render target view for swap chain buffer.");
    }

    backBuffer->Release();

#pragma endregion

#pragma region Create debug module

#if defined(ENGINE_DEBUG_RENDER)
    m_dxgiDebugModule = (void*)::LoadLibraryA("dxgidebug.dll");
    if (m_dxgiDebugModule == nullptr)
    {
        ERROR_AND_DIE("Could not load dxgidebug.dll.");
    }

    typedef HRESULT(WINAPI* GetDebugModuleCB)(REFIID, void**);
    ((GetDebugModuleCB)::GetProcAddress(
        (HMODULE)m_dxgiDebugModule,
        "DXGIGetDebugInterface"
    ))(__uuidof(IDXGIDebug), &m_dxgiDebug);

    if (m_dxgiDebug == nullptr)
    {
        ERROR_AND_DIE("Could not load debug module.");
    }
#endif

#pragma endregion

#pragma region Default Shader Set rasterizer state

    D3D11_RASTERIZER_DESC rasterizerDesc = {};
    rasterizerDesc.FillMode = D3D11_FILL_SOLID;
    rasterizerDesc.CullMode = D3D11_CULL_NONE;
    rasterizerDesc.FrontCounterClockwise = false;
    rasterizerDesc.DepthBias = 0;
    rasterizerDesc.DepthBiasClamp = 0.0f;
    rasterizerDesc.SlopeScaledDepthBias = 0.0f;
    rasterizerDesc.DepthClipEnable = true;
    rasterizerDesc.ScissorEnable = false;
    rasterizerDesc.MultisampleEnable = false;
    rasterizerDesc.AntialiasedLineEnable = true;

    hr = m_device->CreateRasterizerState(&rasterizerDesc, &m_rasterizerState);
    if (!SUCCEEDED(hr))
    {
        ERROR_AND_DIE("Could not create rasterizer state.");
    }

    m_deviceContext->RSSetState(m_rasterizerState);

    m_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

#pragma endregion

#pragma region Create camera constant buffer

    m_cameraCBO = CreateConstantBuffer(sizeof(CameraConstants));

#pragma endregion

#pragma region Set blend states

    D3D11_BLEND_DESC blendDesc = { };
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = blendDesc.RenderTarget[0].SrcBlend;
    blendDesc.RenderTarget[0].DestBlendAlpha = blendDesc.RenderTarget[0].DestBlend;
    blendDesc.RenderTarget[0].BlendOpAlpha = blendDesc.RenderTarget[0].BlendOp;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    // OPAQUE
    hr = m_device->CreateBlendState(
        &blendDesc,
        &m_blendStates[(int)(BlendMode::OPAQUE)]
    );

    if (!SUCCEEDED(hr))
    {
        ERROR_AND_DIE("CreateBlendState for BlendMode::OPAQUE failed.");
    }

    // ALPHA
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;

    hr = m_device->CreateBlendState(
        &blendDesc,
        &m_blendStates[(int)(BlendMode::ALPHA)]
    );

    if (!SUCCEEDED(hr))
    {
        ERROR_AND_DIE("CreateBlendState for BlendMode::ALPHA failed.");
    }

    // ADDITIVE
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;

    hr = m_device->CreateBlendState(
        &blendDesc,
        &m_blendStates[(int)(BlendMode::ADDITIVE)]
    );

    if (!SUCCEEDED(hr))
    {
        ERROR_AND_DIE("CreateBlendState for BlendMode::ADDITIVE failed.");
    }

#pragma endregion

#pragma region Set sampler states

    D3D11_SAMPLER_DESC samplerDesc = {};

    // POINT_CLAMP
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

    hr = m_device->CreateSamplerState(
        &samplerDesc,
        &m_samplerStates[(int)SamplerMode::POINT_CLAMP]
    );

    if (!SUCCEEDED(hr))
    {
        ERROR_AND_DIE("CreateSamplerState for SamplerMode::POINT_CLAMP failed.");
    }

    // BILINEAR_CLAMP
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;

    hr = m_device->CreateSamplerState(
        &samplerDesc,
        &m_samplerStates[(int)SamplerMode::BILINEAR_CLAMP]
    );

#pragma endregion

#pragma region Default Shader

    m_defaultShader = CreateShader("Default", k_defaultShaderSource);
    BindShader(m_defaultShader);

#pragma endregion

#pragma region Default Texture

    m_defaultTexture = CreateTextureFromData(
        "Default",
        IntVec2(2, 2),
        4,
        (uint8_t*)k_defaultTexture
    );
    BindTexture(m_defaultTexture);

#pragma endregion
}

void Renderer::Shutdown()
{
    m_currentCamera = nullptr;
    m_currentShader = nullptr;
    m_currentVertexBuffer = nullptr;

    for (auto& blendState : m_blendStates)
    {
        if (blendState)
        {
            blendState->Release();
        }
    }

    for (auto& shader : m_loadedShaders)
    {
        delete shader;
    }
    m_loadedShaders.clear();

    for (auto& pair : m_loadedTexturesDict)
    {
        delete pair.second;
    }
    m_loadedTexturesDict.clear();

    for (auto& pair : m_loadedFontsDict)
    {
        delete pair.second;
    }
    m_loadedFontsDict.clear();

    m_rasterizerState->Release();
    m_renderTargetView->Release();
    m_swapChain->Release();
    m_deviceContext->Release();
    m_device->Release();

    // Report error leaks and release debug module
#if defined(ENGINE_DEBUG_RENDER)
    ((IDXGIDebug*)m_dxgiDebug)->ReportLiveObjects(
        DXGI_DEBUG_ALL,
        (DXGI_DEBUG_RLO_FLAGS)(
            DXGI_DEBUG_RLO_DETAIL |
            DXGI_DEBUG_RLO_IGNORE_INTERNAL
            )
    );

    ((IDXGIDebug*)m_dxgiDebug)->Release();
    m_dxgiDebug = nullptr;

    ::FreeLibrary((HMODULE)m_dxgiDebugModule);
    m_dxgiDebugModule = nullptr;
#endif
}

void Renderer::BeginFrame()
{
    // Set render target
    m_deviceContext->OMSetRenderTargets(1, &m_renderTargetView, nullptr);
}

void Renderer::EndFrame()
{
    // Present
    HRESULT hr;
    hr = m_swapChain->Present(0, 0);
    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
    {
        ERROR_AND_DIE("Device has been lost, application will now terminate.");
    }
}

void Renderer::CreateRenderingContext()
{
}

void Renderer::ClearScreen(Rgba8 const& clearColor)
{
    // Clear the screen
    float colorAsFloats[4];
    clearColor.GetAsFloats(colorAsFloats);
    m_deviceContext->ClearRenderTargetView(m_renderTargetView, colorAsFloats);
}

void Renderer::SetBlendMode(BlendMode blendMode)
{
    m_desiredBlendMode = blendMode;
}

void Renderer::SetSamplerMode(SamplerMode samplerMode)
{
    m_desiredSamplerMode = samplerMode;
}

void Renderer::SetStatesIfChanged()
{
    GUARANTEE_OR_DIE(m_deviceContext, "SetStatesIfChanged: m_deviceContext is null");

    // Blend state
    ID3D11BlendState* desiredBlendState = m_blendStates[(int)m_desiredBlendMode];
    if (m_blendState != desiredBlendState)
    {
        m_blendState = desiredBlendState;

        float blendFactor[4] = { 0.f, 0.f, 0.f, 0.f };
        UINT sampleMask = 0xffffffff;

        m_deviceContext->OMSetBlendState(m_blendState, blendFactor, sampleMask);
    }

    // Sampler state
    ID3D11SamplerState* desiredSamplerState = m_samplerStates[(int)m_desiredSamplerMode];
    if (m_samplerState != desiredSamplerState)
    {
        m_samplerState = desiredSamplerState;
        m_deviceContext->PSSetSamplers(0, 1, &m_samplerState);
    }
}

void Renderer::BeginCamera(Camera const& camera)
{	

    // Set viewport
    Vec2 camDimensions = (Vec2)g_engine->m_window->GetClientDimensions();

    D3D11_VIEWPORT viewport = {};
    viewport.TopLeftX = 0.f;
    viewport.TopLeftY = 0.f;
    viewport.Width = camDimensions.x;
    viewport.Height = camDimensions.y;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;

    m_deviceContext->RSSetViewports(1, &viewport);

    CameraConstants cameraData = CameraConstants();
    cameraData.OrthoMinX = camera.GetLeft();
    cameraData.OrthoMaxX = camera.GetRight();
    cameraData.OrthoMinY = camera.GetBottom();
    cameraData.OrthoMaxY = camera.GetTop();
    cameraData.OrthoMinZ = 0.0f;
    cameraData.OrthoMaxZ = 1.0f;
    cameraData.pad0 = 0.0f; // Padding to ensure 16-byte alignment
    cameraData.pad1 = 0.0f;

    CopyCPUToGPU(&cameraData, sizeof(cameraData), m_cameraCBO);
    BindConstantBuffer(m_cameraCBO);
}

void Renderer::EndCamera()
{

}

void Renderer::DrawVertexArray(int numVertexes, Vertex const* vertexes) const
{
    if (numVertexes % 3 != 0 || vertexes == nullptr) 
    {
        return;
    }

    unsigned int size = numVertexes * sizeof(Vertex);

    VertexBuffer* currentVertexBuffer = g_engine->m_renderer->CreateVertexBuffer(size, sizeof(Vertex));
    g_engine->m_renderer->CopyCPUToGPU(vertexes, size, currentVertexBuffer);
    g_engine->m_renderer->DrawVertexBuffer(currentVertexBuffer, numVertexes);
}

void Renderer::DrawVertexArray(std::vector<Vertex> const& verts) const
{
    DrawVertexArray(static_cast<int>(verts.size()), verts.data());
}

void Renderer::DrawVertexBuffer(VertexBuffer* vertexBuffer, unsigned int vertexCount)
{
    SetStatesIfChanged();
    BindVertexBuffer(vertexBuffer);
    m_deviceContext->Draw(vertexCount, 0);
}

Texture* Renderer::CreateOrGetTextureFromFile(char const* imageFilePath)
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

Texture* Renderer::CreateTextureFromImage(const Image& image)
{
    return CreateTextureFromData(
        image.GetImageFilePath().c_str(),
        image.GetDimensions(),
        4,
        (uint8_t*)image.GetRawData()
    );
}

Texture* Renderer::CreateTextureFromFile(char const* imageFilePath)
{
    Image image(imageFilePath);

	return CreateTextureFromImage(image);
}

Texture* Renderer::GetTextureFromFileName(char const* imageFilePath)
{
    if (imageFilePath == nullptr) 
	{
        return nullptr;
    }

    std::string filePath = std::string(imageFilePath);
    auto found = m_loadedTexturesDict.find(filePath);
    if (found != m_loadedTexturesDict.end())
    {
        return found->second;
    }

    return nullptr;
}

Shader* Renderer::CreateShader(char const* shaderName, char const* shaderSource)
{
    GUARANTEE_OR_DIE(m_device, "CreateShader: m_device is null");
    GUARANTEE_OR_DIE(shaderName && shaderName[0], "CreateShader: shaderName is null/empty");
    GUARANTEE_OR_DIE(shaderSource, "CreateShader: shaderSource is null");

    ShaderConfig config;
    config.m_name = shaderName;

    Shader* shader = new Shader(config);

    // Compile VS / PS
    std::vector<unsigned char> vsByteCode;
    std::vector<unsigned char> psByteCode;

    bool vsOK = CompileShaderToByteCode(vsByteCode, shaderName, shaderSource, config.m_vertexEntryPoint.c_str(), "vs_5_0");
    GUARANTEE_OR_DIE(vsOK, Stringf("Could not compile vertex shader for '%s'", shaderName));

    bool psOK = CompileShaderToByteCode(psByteCode, shaderName, shaderSource, config.m_pixelEntryPoint.c_str(), "ps_5_0");
    GUARANTEE_OR_DIE(psOK, Stringf("Could not compile pixel shader for '%s'", shaderName));

    // Create VS / PS
    HRESULT hr = m_device->CreateVertexShader(
        vsByteCode.data(),
        vsByteCode.size(),
        nullptr,
        &shader->m_vertexShader
    );
    GUARANTEE_OR_DIE(SUCCEEDED(hr), Stringf("Could not create vertex shader for '%s'", shaderName));

    hr = m_device->CreatePixelShader(
        psByteCode.data(),
        psByteCode.size(),
        nullptr,
        &shader->m_pixelShader
    );
    GUARANTEE_OR_DIE(SUCCEEDED(hr), Stringf("Could not create pixel shader for '%s'", shaderName));

    // Create Input Layout (Vertex_PCU)
    D3D11_INPUT_ELEMENT_DESC inputElementDesc[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,                            D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM,  0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    hr = m_device->CreateInputLayout(
        inputElementDesc,
        (UINT)ARRAYSIZE(inputElementDesc),
        vsByteCode.data(),
        (UINT)vsByteCode.size(),
        &shader->m_inputLayout
    );
    GUARANTEE_OR_DIE(SUCCEEDED(hr), Stringf("Could not create input layout for '%s'", shaderName));

    m_loadedShaders.push_back(shader);

    return shader;
}


Shader* Renderer::CreateShader(char const* shaderName)
{
    GUARANTEE_OR_DIE(shaderName && shaderName[0], "CreateShader(shaderName): shaderName is null/empty");

    std::string shaderFilename = std::string(shaderName) + ".hlsl";

    std::string shaderSource;
    int bytesRead = FileReadToString(shaderSource, shaderFilename);

    GUARANTEE_OR_DIE(bytesRead > 0, Stringf("Failed to read shader file \"%s\"", shaderFilename.c_str()));

    return CreateShader(shaderName, shaderSource.c_str());
}

bool Renderer::CompileShaderToByteCode(
    std::vector<unsigned char>& outByteCode,
    char const* name,
    char const* source,
    char const* entryPoint,
    char const* target
)
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
    ID3DBlob* errorBlob = nullptr;

    HRESULT hr = D3DCompile(
        source,
        strlen(source),
        name,
        nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        entryPoint,
        target,
        shaderFlags,
        0,
        &shaderBlob,
        &errorBlob
    );

    if (!SUCCEEDED(hr))
    {
        if (errorBlob)
        {
            DebuggerPrintf((char*)errorBlob->GetBufferPointer());
            errorBlob->Release();
        }
        if (shaderBlob) shaderBlob->Release();
        return false;
    }

    outByteCode.resize(shaderBlob->GetBufferSize());
    memcpy(outByteCode.data(), shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize());

    shaderBlob->Release();
    if (errorBlob) errorBlob->Release();
    return true;
}

void Renderer::BindShader(Shader* shader)
{
    GUARANTEE_OR_DIE(m_deviceContext, "BindShader: m_deviceContext is null");

    if (shader == nullptr)
    {
        shader = m_defaultShader;
    }

    m_deviceContext->IASetInputLayout(shader->m_inputLayout);
    m_deviceContext->VSSetShader(shader->m_vertexShader, nullptr, 0);
    m_deviceContext->PSSetShader(shader->m_pixelShader, nullptr, 0);
}

VertexBuffer* Renderer::CreateVertexBuffer(const unsigned int size, unsigned int stride)
{
    return new VertexBuffer(m_device, size, stride);
}

ConstantBuffer* Renderer::CreateConstantBuffer(const unsigned int size)
{
    return new ConstantBuffer(m_device, size);
}

void Renderer::CopyCPUToGPU(const void* data, unsigned int size, VertexBuffer* vertexBuffer)
{
    GUARANTEE_OR_DIE(m_deviceContext, "CopyCPUToGPU: m_deviceContext is null");
    GUARANTEE_OR_DIE(vertexBuffer, "CopyCPUToGPU: vertexBuffer is null");
    GUARANTEE_OR_DIE(data, "CopyCPUToGPU: data is null");
    GUARANTEE_OR_DIE(size > 0, "CopyCPUToGPU: size is 0");

    if (vertexBuffer->m_size > 0)
    {
        GUARANTEE_OR_DIE(size <= vertexBuffer->m_size, "CopyCPUToGPU: upload size exceeds vertex buffer capacity");
    }

    D3D11_MAPPED_SUBRESOURCE mapped = {};
    HRESULT hr = m_deviceContext->Map(vertexBuffer->m_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    GUARANTEE_OR_DIE(SUCCEEDED(hr), "CopyCPUToGPU: Map failed");

    memcpy(mapped.pData, data, size);

    m_deviceContext->Unmap(vertexBuffer->m_buffer, 0);
}

void Renderer::CopyCPUToGPU(const void* data, unsigned int size, ConstantBuffer* constantBuffer)
{
    GUARANTEE_OR_DIE(m_deviceContext, "CopyCPUToGPU: m_deviceContext is null");
    GUARANTEE_OR_DIE(constantBuffer, "CopyCPUToGPU: vertexBuffer is null");
    GUARANTEE_OR_DIE(data, "CopyCPUToGPU: data is null");

    if (constantBuffer->m_size > 0)
    {
        GUARANTEE_OR_DIE(size <= constantBuffer->m_size, "CopyCPUToGPU: upload size exceeds constant buffer capacity");
    }

    D3D11_MAPPED_SUBRESOURCE mapped = {};
    HRESULT hr = m_deviceContext->Map(constantBuffer->m_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    GUARANTEE_OR_DIE(SUCCEEDED(hr), "CopyCPUToGPU: Map failed");

    memcpy(mapped.pData, data, size);

    m_deviceContext->Unmap(constantBuffer->m_buffer, 0);
}

void Renderer::BindVertexBuffer(VertexBuffer* vertexBuffer)
{
    GUARANTEE_OR_DIE(m_deviceContext, "BindVertexBuffer: m_deviceContext is null");

    if (vertexBuffer == nullptr)
    {
        ID3D11Buffer* nullBuf = nullptr;
        UINT stride = 0;
        UINT offset = 0;
        m_deviceContext->IASetVertexBuffers(0, 1, &nullBuf, &stride, &offset);
        return;
    }

    UINT stride = vertexBuffer->m_stride;
    UINT offset = 0;
    ID3D11Buffer* buf = vertexBuffer->m_buffer;
    m_deviceContext->IASetVertexBuffers(0, 1, &buf, &stride, &offset);
}

void Renderer::BindConstantBuffer(ConstantBuffer* constantBuffer)
{
    GUARANTEE_OR_DIE(m_deviceContext, "BindConstantBuffer: m_deviceContext is null");

    if (constantBuffer == nullptr)
    {
        ID3D11Buffer* nullBuf = nullptr;
        m_deviceContext->VSSetConstantBuffers(k_cameraConstantsSlot, 1, &nullBuf);
        m_deviceContext->PSSetConstantBuffers(k_cameraConstantsSlot, 1, &nullBuf);
        return;
    }

    ID3D11Buffer* buf = constantBuffer->m_buffer;
    GUARANTEE_OR_DIE(buf, "BindConstantBuffer: constantBuffer->m_buffer is null");

    m_deviceContext->VSSetConstantBuffers(k_cameraConstantsSlot, 1, &buf);
    m_deviceContext->PSSetConstantBuffers(k_cameraConstantsSlot, 1, &buf);
}

Texture* Renderer::CreateTextureFromData(char const* name, IntVec2 dimensions, int bytesPerTexel, uint8_t* texelData)
{
    GUARANTEE_OR_DIE(m_device, "CreateTextureFromData: m_device is null");
    GUARANTEE_OR_DIE(texelData, Stringf("CreateTextureFromData failed for \"%s\" - texelData was null!", name));
    GUARANTEE_OR_DIE(dimensions.x > 0 && dimensions.y > 0,
        Stringf("CreateTextureFromData failed for \"%s\" - illegal texture dimensions (%i x %i)", name, dimensions.x, dimensions.y));

    // We only support RGBA8 format for now, so require 4 bytes per texel
    GUARANTEE_OR_DIE(bytesPerTexel == 4,
        Stringf("CreateTextureFromData requires 4 bytes/texel (RGBA). Got %i for \"%s\"", bytesPerTexel, name));

    Texture* newTexture = new Texture();
    newTexture->m_name = name;
    newTexture->m_dimensions = dimensions;

    D3D11_TEXTURE2D_DESC textureDesc = {};
    textureDesc.Width = (UINT)dimensions.x;
    textureDesc.Height = (UINT)dimensions.y;
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.Usage = D3D11_USAGE_IMMUTABLE;
    textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA textureData = {};
    textureData.pSysMem = texelData;
    textureData.SysMemPitch = 4 * dimensions.x; 

    HRESULT hr = m_device->CreateTexture2D(&textureDesc, &textureData, &newTexture->m_texture);
    if (!SUCCEEDED(hr))
    {
        delete newTexture;
        ERROR_AND_DIE(Stringf("CreateTexture2D failed for image file \"%s\".", name));
    }

    hr = m_device->CreateShaderResourceView(newTexture->m_texture, nullptr, &newTexture->m_shaderResourceView);
    if (!SUCCEEDED(hr))
    {
        delete newTexture;
        ERROR_AND_DIE(Stringf("CreateShaderResourceView failed for image file \"%s\".", name));
    }

    m_loadedTexturesDict[newTexture->m_name] = newTexture;
    return newTexture;
}


BitmapFont* Renderer::CreateOrGetBitmapFont(char const* fontFilePathNameWithNoExtension)
{
    std::string fontKey = std::string(fontFilePathNameWithNoExtension);
    auto found = m_loadedFontsDict.find(fontKey);
    if (found != m_loadedFontsDict.end())
    {
        return found->second;
    }

    Texture* fontTexture = CreateOrGetTextureFromFile(Stringf("%s.png", fontFilePathNameWithNoExtension).c_str());
    BitmapFont* newBitmapFont = new BitmapFont(fontFilePathNameWithNoExtension, *fontTexture);
    m_loadedFontsDict[fontKey] = newBitmapFont;
    return newBitmapFont;
}

void Renderer::BindTexture(Texture* texture)
{
    GUARANTEE_OR_DIE(m_deviceContext, "BindTexture: m_deviceContext is null");

    if (texture == nullptr)
    {
        texture = m_defaultTexture;
    }

    ID3D11ShaderResourceView* srv = texture->m_shaderResourceView;
    m_deviceContext->PSSetShaderResources(0, 1, &srv);
}

