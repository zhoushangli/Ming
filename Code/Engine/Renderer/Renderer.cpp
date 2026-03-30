#include "Engine/Renderer/Renderer.hpp"

#include "Engine/Core/Engine.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Core/Vertex_PCUTBN.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Renderer/Texture.hpp"
#include "Engine/Renderer/VertexBuffer.hpp"
#include "Engine/Renderer/ConstantBuffer.hpp"
#include "Engine/Renderer/IndexBuffer.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "ThirdParty/stb/stb_image.h"

#include <Windows.h>
#include <d3dcompiler.h>
#include <d3d11.h>

#if defined(OPAQUE)
#undef OPAQUE
#endif

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

#if defined(ENGINE_DEBUG_RENDER)
#include <dxgidebug.h>
#pragma comment(lib, "dxguid.lib")
#endif

HGLRC g_openGLRenderingContext = nullptr;

const uint8_t k_defaultTexture[16] =
    {
        0xFF, 0xFF, 0xFF, 0xFF, // (0,0)
        0xFF, 0xFF, 0xFF, 0xFF, // (1,0)
        0xFF, 0xFF, 0xFF, 0xFF, // (0,1)
        0xFF, 0xFF, 0xFF, 0xFF  // (1,1)
};

//------------------------------------------------------------------------------------------------
// Lifetime and frame loop
Renderer::Renderer(RendererConfig config) : m_config(config)
{
}

Renderer::~Renderer()
{
}

#pragma region Public: Lifetime and frame loop
void Renderer::Startup()
{
    unsigned int deviceFlags = 0;
#if defined(ENGINE_DEBUG_RENDER)
    deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

#pragma region Startup: Create device and swap chain

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

#pragma region Startup: Get back buffer texture

    ID3D11Texture2D *backBuffer;
    hr = m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void **)&backBuffer);
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

#pragma region Startup: Create debug module

#if defined(ENGINE_DEBUG_RENDER)
    m_dxgiDebugModule = (void *)::LoadLibraryA("dxgidebug.dll");
    if (m_dxgiDebugModule == nullptr)
    {
        ERROR_AND_DIE("Could not load dxgidebug.dll.");
    }

    typedef HRESULT(WINAPI * GetDebugModuleCB)(REFIID, void **);
    ((GetDebugModuleCB)::GetProcAddress(
        (HMODULE)m_dxgiDebugModule,
        "DXGIGetDebugInterface"))(__uuidof(IDXGIDebug), &m_dxgiDebug);

    if (m_dxgiDebug == nullptr)
    {
        ERROR_AND_DIE("Could not load debug module.");
    }
#endif

#pragma endregion

#pragma region Startup: Rasterizer states

    // SOLID CULL NONE
    D3D11_RASTERIZER_DESC rasterizerDesc = {};
    rasterizerDesc.FillMode = D3D11_FILL_SOLID;
    rasterizerDesc.CullMode = D3D11_CULL_NONE;
    rasterizerDesc.FrontCounterClockwise = true;
    rasterizerDesc.DepthClipEnable = true;
    rasterizerDesc.AntialiasedLineEnable = true;
    hr = m_device->CreateRasterizerState(
        &rasterizerDesc,
        &m_rasterizerStates[(int)RasterizerMode::SOLID_CULL_NONE]);

    // SOLID CULL BACK
    rasterizerDesc.FillMode = D3D11_FILL_SOLID;
    rasterizerDesc.CullMode = D3D11_CULL_BACK;
    hr = m_device->CreateRasterizerState(
        &rasterizerDesc,
        &m_rasterizerStates[(int)RasterizerMode::SOLID_CULL_BACK]);

    // WIREFRAME CULL NONE
    rasterizerDesc.FillMode = D3D11_FILL_WIREFRAME;
    rasterizerDesc.CullMode = D3D11_CULL_NONE;
    hr = m_device->CreateRasterizerState(
        &rasterizerDesc,
        &m_rasterizerStates[(int)RasterizerMode::WIREFRAME_CULL_NONE]);

    // WIREFRAME CULL BACK
    rasterizerDesc.FillMode = D3D11_FILL_WIREFRAME;
    rasterizerDesc.CullMode = D3D11_CULL_BACK;
    hr = m_device->CreateRasterizerState(
        &rasterizerDesc,
        &m_rasterizerStates[(int)RasterizerMode::WIREFRAME_CULL_BACK]);

    m_deviceContext->RSSetState(m_rasterizerStates[(int)RasterizerMode::SOLID_CULL_BACK]);

    m_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

#pragma endregion

#pragma region Startup: Create buffers

    m_vertexBufferPCU = CreateVertexBuffer(sizeof(Vertex_PCU) * 3, sizeof(Vertex_PCU));
    m_vertexBufferPCUTBN = CreateVertexBuffer(sizeof(Vertex_PCUTBN) * 3, sizeof(Vertex_PCUTBN));
    m_currentIndexBuffer = CreateIndexBuffer(sizeof(unsigned int) * 3);
    m_lightCBO = CreateConstantBuffer(sizeof(LightConstants));
    m_cameraCBO = CreateConstantBuffer(sizeof(CameraConstants));
    m_modelCBO = CreateConstantBuffer(sizeof(ModelConstants));

#pragma endregion

#pragma region Startup: Create blend states

    D3D11_BLEND_DESC blendDesc = {};
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
        &m_blendStates[(int)(BlendMode::OPAQUE)]);

    // ALPHA
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;

    hr = m_device->CreateBlendState(
        &blendDesc,
        &m_blendStates[(int)(BlendMode::ALPHA)]);

    // ADDITIVE
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;

    hr = m_device->CreateBlendState(
        &blendDesc,
        &m_blendStates[(int)(BlendMode::ADDITIVE)]);

#pragma endregion

#pragma region Startup: Create sampler states

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
        &m_samplerStates[(int)SamplerMode::POINT_CLAMP]);

    // BILINEAR_CLAMP
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;

    hr = m_device->CreateSamplerState(
        &samplerDesc,
        &m_samplerStates[(int)SamplerMode::BILINEAR_CLAMP]);

#pragma endregion

#pragma region Startup: Create depth stencil state

    // Create depth stencil texture and view
    D3D11_TEXTURE2D_DESC depthTextureDesc = {};
    depthTextureDesc.Width = g_engine->m_window->GetClientDimensions().x;
    depthTextureDesc.Height = g_engine->m_window->GetClientDimensions().y;
    depthTextureDesc.MipLevels = 1;
    depthTextureDesc.ArraySize = 1;
    depthTextureDesc.Usage = D3D11_USAGE_DEFAULT;
    depthTextureDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthTextureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    depthTextureDesc.SampleDesc.Count = 1;

    hr = m_device->CreateTexture2D(&depthTextureDesc, nullptr, &m_depthStencilTexture);
    hr = m_device->CreateDepthStencilView(m_depthStencilTexture, nullptr, &m_depthStencilDSV);

    // DISABLED
    D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {};
    hr = m_device->CreateDepthStencilState(
        &depthStencilDesc,
        &m_depthStencilStates[(int)DepthMode::DISABLED]);

    // READ_ONLY_ALWAYS
    depthStencilDesc.DepthEnable = TRUE;
    depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    depthStencilDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;

    hr = m_device->CreateDepthStencilState(
        &depthStencilDesc,
        &m_depthStencilStates[(int)DepthMode::READ_ONLY_ALWAYS]);

    // READ_ONLY_LESS_EQUAL
    depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;

    hr = m_device->CreateDepthStencilState(
        &depthStencilDesc,
        &m_depthStencilStates[(int)DepthMode::READ_ONLY_LESS_EQUAL]);

    // READ_WRITE_LESS_EQUAL
    depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;

    hr = m_device->CreateDepthStencilState(
        &depthStencilDesc,
        &m_depthStencilStates[(int)DepthMode::READ_WRITE_LESS_EQUAL]);

#pragma endregion

#pragma region Startup: Create default shader

    m_defaultShader = CreateShader("Data/Shaders/Default");
    BindShader(m_defaultShader);

#pragma endregion

#pragma region Startup: Create default texture

    m_defaultTexture = CreateTextureFromData(
        "Default",
        IntVec2(2, 2),
        4,
        (uint8_t *)k_defaultTexture);
    BindTexture(m_defaultTexture);

#pragma endregion
}

void Renderer::Shutdown()
{
    m_currentCamera = nullptr;
    m_currentShader = nullptr;

    delete m_modelCBO;
    m_modelCBO = nullptr;

    delete m_cameraCBO;
    m_cameraCBO = nullptr;

    delete m_vertexBufferPCU;
    m_vertexBufferPCU = nullptr;

    delete m_vertexBufferPCUTBN;
    m_vertexBufferPCUTBN = nullptr;

    delete m_currentIndexBuffer;
    m_currentIndexBuffer = nullptr;

    for (auto &rasterizerState : m_rasterizerStates)
    {
        if (rasterizerState)
        {
            rasterizerState->Release();
            rasterizerState = nullptr;
        }
    }

    for (auto &blendState : m_blendStates)
    {
        if (blendState)
        {
            blendState->Release();
            blendState = nullptr;
        }
    }

    for (auto &samplerState : m_samplerStates)
    {
        if (samplerState)
        {
            samplerState->Release();
            samplerState = nullptr;
        }
    }

    for (auto &shader : m_loadedShaders)
    {
        delete shader;
    }
    m_loadedShaders.clear();

    for (auto &pair : m_loadedTexturesDict)
    {
        delete pair.second;
    }
    m_loadedTexturesDict.clear();

    for (auto &pair : m_loadedFontsDict)
    {
        delete pair.second;
    }
    m_loadedFontsDict.clear();

    m_renderTargetView->Release();
    m_swapChain->Release();
    m_deviceContext->Release();
    m_device->Release();

    // Report error leaks and release debug module
#if defined(ENGINE_DEBUG_RENDER)
    ((IDXGIDebug *)m_dxgiDebug)->ReportLiveObjects(DXGI_DEBUG_ALL, (DXGI_DEBUG_RLO_FLAGS)(DXGI_DEBUG_RLO_DETAIL | DXGI_DEBUG_RLO_IGNORE_INTERNAL));

    ((IDXGIDebug *)m_dxgiDebug)->Release();
    m_dxgiDebug = nullptr;

    ::FreeLibrary((HMODULE)m_dxgiDebugModule);
    m_dxgiDebugModule = nullptr;
#endif
}

void Renderer::BeginFrame()
{
    // Set render target
    m_deviceContext->OMSetRenderTargets(1, &m_renderTargetView, m_depthStencilDSV);
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

#pragma endregion

#pragma region Public: Camera and pipeline state

void Renderer::BeginCamera(Camera const &camera)
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
    cameraData.WorldToCameraTransform = camera.GetWorldToCameraTransform();
    cameraData.CameraToRenderTransform = camera.GetCameraToRenderTransform();
    cameraData.RenderToClipTransform = camera.GetRenderToClipTransform();

    CopyCPUToGPU(&cameraData, sizeof(cameraData), m_cameraCBO);
    BindConstantBuffer(m_cameraCBO, k_cameraConstantsSlot);
}

void Renderer::EndCamera()
{
}

void Renderer::ClearScreen(Rgba8 const &clearColor)
{
    // Clear the screen
    float colorAsFloats[4];
    clearColor.GetAsFloats(colorAsFloats);
    m_deviceContext->ClearRenderTargetView(m_renderTargetView, colorAsFloats);
    m_deviceContext->ClearDepthStencilView(m_depthStencilDSV, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

void Renderer::SetBlendMode(BlendMode blendMode)
{
    m_desiredBlendMode = blendMode;
}

void Renderer::SetSamplerMode(SamplerMode samplerMode)
{
    m_desiredSamplerMode = samplerMode;
}

void Renderer::SetRasterizerMode(RasterizerMode rasterizerMode)
{
    m_desiredRasterizerMode = rasterizerMode;
}

void Renderer::SetDepthMode(DepthMode depthMode)
{
    m_desiredDepthMode = depthMode;
}

void Renderer::SetStatesIfChanged()
{
    GUARANTEE_OR_DIE(m_deviceContext, "SetStatesIfChanged: m_deviceContext is null");

    // Blend state
    ID3D11BlendState *desiredBlendState = m_blendStates[(int)m_desiredBlendMode];
    if (m_blendState != desiredBlendState)
    {
        m_blendState = desiredBlendState;

        float blendFactor[4] = {0.f, 0.f, 0.f, 0.f};
        UINT sampleMask = 0xffffffff;

        m_deviceContext->OMSetBlendState(m_blendState, blendFactor, sampleMask);
    }

    // Sampler state
    ID3D11SamplerState *desiredSamplerState = m_samplerStates[(int)m_desiredSamplerMode];
    if (m_samplerState != desiredSamplerState)
    {
        m_samplerState = desiredSamplerState;
        m_deviceContext->PSSetSamplers(0, 1, &m_samplerState);
    }

    ID3D11RasterizerState *desiredRasterizerState = m_rasterizerStates[(int)m_desiredRasterizerMode];
    if (m_rasterizerState != desiredRasterizerState)
    {
        m_rasterizerState = desiredRasterizerState;
        m_deviceContext->RSSetState(m_rasterizerState);
    }

    ID3D11DepthStencilState *desiredDepthStencilState = m_depthStencilStates[(int)m_desiredDepthMode];
    if (m_depthStencilState != desiredDepthStencilState)
    {
        m_depthStencilState = desiredDepthStencilState;
        m_deviceContext->OMSetDepthStencilState(m_depthStencilState, 0);
    }
}

#pragma endregion

#pragma region Public: Draw entry points

void Renderer::DrawVertexArray(int numVertexes, Vertex_PCU const *vertexes) const
{
    if (numVertexes % 3 != 0 || vertexes == nullptr)
    {
        return;
    }

    unsigned int size = numVertexes * sizeof(Vertex_PCU);

    m_vertexBufferPCU->Resize(size);
    g_engine->m_renderer->CopyCPUToGPU(vertexes, size, m_vertexBufferPCU);
    g_engine->m_renderer->DrawVertexBuffer(m_vertexBufferPCU, numVertexes);
}

void Renderer::DrawVertexArray(std::vector<Vertex_PCU> const &verts) const
{
    DrawVertexArray(static_cast<int>(verts.size()), verts.data());
}

void Renderer::DrawVertexArray(std::vector<Vertex_PCU> const &verts, std::vector<unsigned int> const &vertIndexes) const
{
    unsigned int vertsNum = static_cast<unsigned int>(verts.size());
    unsigned int indexesNum = static_cast<unsigned int>(vertIndexes.size());
    unsigned int vertsSize = vertsNum * sizeof(Vertex_PCU);
    unsigned int indexesSize = indexesNum * sizeof(unsigned int);

    m_vertexBufferPCU->Resize(vertsSize);
    m_currentIndexBuffer->Resize(indexesSize);

    g_engine->m_renderer->CopyCPUToGPU(verts.data(), vertsSize, m_vertexBufferPCU);
    g_engine->m_renderer->CopyCPUToGPU(vertIndexes.data(), indexesSize, m_currentIndexBuffer);

    g_engine->m_renderer->DrawIndexedVertexBuffer(m_vertexBufferPCU, m_currentIndexBuffer, indexesNum);
}

void Renderer::DrawVertexArray(int numVertexes, Vertex_PCUTBN const *vertexes) const
{
    if (numVertexes % 3 != 0 || vertexes == nullptr)
    {
        return;
    }

    unsigned int size = numVertexes * sizeof(Vertex_PCUTBN);

    m_vertexBufferPCUTBN->Resize(size);
    g_engine->m_renderer->CopyCPUToGPU(vertexes, size, m_vertexBufferPCUTBN);
    g_engine->m_renderer->DrawVertexBuffer(m_vertexBufferPCUTBN, numVertexes);
}

void Renderer::DrawVertexArray(std::vector<Vertex_PCUTBN> const &verts) const
{
    DrawVertexArray(static_cast<int>(verts.size()), verts.data());
}

void Renderer::DrawVertexArray(std::vector<Vertex_PCUTBN> const &verts, std::vector<unsigned int> const &vertIndexes) const
{
    unsigned int vertsNum = static_cast<unsigned int>(verts.size());
    unsigned int indexesNum = static_cast<unsigned int>(vertIndexes.size());
    unsigned int vertsSize = vertsNum * sizeof(Vertex_PCUTBN);
    unsigned int indexesSize = indexesNum * sizeof(unsigned int);

    m_vertexBufferPCUTBN->Resize(vertsSize);
    m_currentIndexBuffer->Resize(indexesSize);

    g_engine->m_renderer->CopyCPUToGPU(verts.data(), vertsSize, m_vertexBufferPCUTBN);
    g_engine->m_renderer->CopyCPUToGPU(vertIndexes.data(), indexesSize, m_currentIndexBuffer);

    g_engine->m_renderer->DrawIndexedVertexBuffer(m_vertexBufferPCUTBN, m_currentIndexBuffer, indexesNum);
}

void Renderer::DrawVertexBuffer(VertexBuffer *vertexBuffer, unsigned int vertexCount)
{
    SetStatesIfChanged();
    BindVertexBuffer(vertexBuffer);
    m_deviceContext->Draw(vertexCount, 0);
}

void Renderer::DrawIndexedVertexBuffer(VertexBuffer *vertexBuffer, IndexBuffer *indexBuffer, unsigned int indexCount)
{
    SetStatesIfChanged();
    BindVertexBuffer(vertexBuffer);
    BindIndexBuffer(indexBuffer);
    m_deviceContext->DrawIndexed(indexCount, 0, 0);
}

#pragma endregion

#pragma region Public: High-level bind helpers used by gameplay/render features

void Renderer::BindTexture(Texture *texture)
{
    GUARANTEE_OR_DIE(m_deviceContext, "BindTexture: m_deviceContext is null");

    if (texture == nullptr)
    {
        texture = m_defaultTexture;
    }

    ID3D11ShaderResourceView *srv = texture->m_shaderResourceView;
    m_deviceContext->PSSetShaderResources(0, 1, &srv);
    m_deviceContext->PSGetSamplers(0, 1, &m_samplerState);
}

void Renderer::BindShader(Shader *shader)
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

void Renderer::BindModelConstants(Matrix4x4 const &modelToWorldTransform, Rgba8 const &modelColor)
{
    ModelConstants modelData = ModelConstants();
    modelData.ModelToWorldTransform = modelToWorldTransform;
    modelData.ModelColor[0] = modelColor.r / 255.f;
    modelData.ModelColor[1] = modelColor.g / 255.f;
    modelData.ModelColor[2] = modelColor.b / 255.f;
    modelData.ModelColor[3] = modelColor.a / 255.f;

    CopyCPUToGPU(&modelData, sizeof(modelData), m_modelCBO);
    BindConstantBuffer(m_modelCBO, k_modelConstantsSlot);
}

void Renderer::BindLightConstants(Vec3 const &sunDirection, float const sunIntensity, float const ambientIntensity)
{
    LightConstants lightData = LightConstants();
    lightData.SunDirection = sunDirection;
    lightData.SunIntensity = sunIntensity;
    lightData.AmbientIntensity = ambientIntensity;

    CopyCPUToGPU(&lightData, sizeof(lightData), m_lightCBO);
    BindConstantBuffer(m_lightCBO, k_lightConstantsSlot);
}

#pragma endregion

#pragma region Public: GPU resource creation and cache access

Shader *Renderer::CreateShader(char const *shaderName, VertexType vertexType)
{
    GUARANTEE_OR_DIE(shaderName && shaderName[0], "CreateShader(shaderName): shaderName is null/empty");

    std::string shaderFilename = std::string(shaderName) + ".hlsl";

    std::string shaderSource;
    int bytesRead = FileReadToString(shaderSource, shaderFilename);

    GUARANTEE_OR_DIE(bytesRead > 0, Stringf("Failed to read shader file \"%s\"", shaderFilename.c_str()));

    return CreateShader(shaderName, shaderSource.c_str(), vertexType);
}

Texture *Renderer::CreateOrGetTextureFromFile(char const *imageFilePath)
{
    // See if we already have this texture previously loaded
    Texture *existingTexture = GetTextureFromFileName(imageFilePath); // You need to write this
    if (existingTexture)
    {
        return existingTexture;
    }

    // Never seen this texture before!  Let's load it.
    Texture *newTexture = CreateTextureFromFile(imageFilePath);
    return newTexture;
}

Texture *Renderer::CreateTextureFromImage(const Image &image)
{
    return CreateTextureFromData(
        image.GetImageFilePath().c_str(),
        image.GetDimensions(),
        4,
        (uint8_t *)image.GetRawData());
}

Texture *Renderer::CreateTextureFromData(char const *name, IntVec2 dimensions, int bytesPerTexel, uint8_t *texelData)
{
    GUARANTEE_OR_DIE(m_device, "CreateTextureFromData: m_device is null");
    GUARANTEE_OR_DIE(texelData, Stringf("CreateTextureFromData failed for \"%s\" - texelData was null!", name));
    GUARANTEE_OR_DIE(dimensions.x > 0 && dimensions.y > 0,
                     Stringf("CreateTextureFromData failed for \"%s\" - illegal texture dimensions (%i x %i)", name, dimensions.x, dimensions.y));

    // We only support RGBA8 format for now, so require 4 bytes per texel
    GUARANTEE_OR_DIE(bytesPerTexel == 4,
                     Stringf("CreateTextureFromData requires 4 bytes/texel (RGBA). Got %i for \"%s\"", bytesPerTexel, name));

    Texture *newTexture = new Texture();
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

BitmapFont *Renderer::CreateOrGetBitmapFont(char const *fontFilePathNameWithNoExtension)
{
    std::string fontKey = std::string(fontFilePathNameWithNoExtension);
    auto found = m_loadedFontsDict.find(fontKey);
    if (found != m_loadedFontsDict.end())
    {
        return found->second;
    }

    Texture *fontTexture = CreateOrGetTextureFromFile(Stringf("%s.png", fontFilePathNameWithNoExtension).c_str());
    BitmapFont *newBitmapFont = new BitmapFont(fontFilePathNameWithNoExtension, *fontTexture);
    m_loadedFontsDict[fontKey] = newBitmapFont;
    return newBitmapFont;
}

VertexBuffer *Renderer::CreateVertexBuffer(const unsigned int size, unsigned int stride)
{
    return new VertexBuffer(m_device, size, stride);
}

ConstantBuffer *Renderer::CreateConstantBuffer(const unsigned int size)
{
    return new ConstantBuffer(m_device, size);
}

IndexBuffer *Renderer::CreateIndexBuffer(const unsigned int size)
{
    return new IndexBuffer(m_device, size);
}

#pragma endregion

#pragma region Public: CPU -> GPU uploads

void Renderer::CopyCPUToGPU(const void *data, unsigned int size, VertexBuffer *vertexBuffer)
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

void Renderer::CopyCPUToGPU(const void *data, unsigned int size, ConstantBuffer *constantBuffer)
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

void Renderer::CopyCPUToGPU(const void *data, unsigned int size, IndexBuffer *indexBuffer)
{
    GUARANTEE_OR_DIE(m_deviceContext, "CopyCPUToGPU: m_deviceContext is null");
    GUARANTEE_OR_DIE(indexBuffer, "CopyCPUToGPU: vertexBuffer is null");
    GUARANTEE_OR_DIE(data, "CopyCPUToGPU: data is null");

    if (indexBuffer->m_size > 0)
    {
        GUARANTEE_OR_DIE(size <= indexBuffer->m_size, "CopyCPUToGPU: upload size exceeds constant buffer capacity");
    }

    D3D11_MAPPED_SUBRESOURCE mapped = {};
    HRESULT hr = m_deviceContext->Map(indexBuffer->m_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    GUARANTEE_OR_DIE(SUCCEEDED(hr), "CopyCPUToGPU: Map failed");

    memcpy(mapped.pData, data, size);

    m_deviceContext->Unmap(indexBuffer->m_buffer, 0);
}

#pragma endregion

#pragma region Private: Texture cache internals

Texture *Renderer::CreateTextureFromFile(char const *imageFilePath)
{
    Image image(imageFilePath);

    return CreateTextureFromImage(image);
}

Texture *Renderer::GetTextureFromFileName(char const *imageFilePath)
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

#pragma endregion

#pragma region Private: Shader creation internals

Shader *Renderer::CreateShader(char const *shaderName, char const *shaderSource, VertexType vertexType)
{
    GUARANTEE_OR_DIE(m_device, "CreateShader: m_device is null");
    GUARANTEE_OR_DIE(shaderName && shaderName[0], "CreateShader: shaderName is null/empty");
    GUARANTEE_OR_DIE(shaderSource, "CreateShader: shaderSource is null");

    ShaderConfig config;
    config.m_name = shaderName;

    Shader *shader = new Shader(config);

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
        &shader->m_vertexShader);
    GUARANTEE_OR_DIE(SUCCEEDED(hr), Stringf("Could not create vertex shader for '%s'", shaderName));

    hr = m_device->CreatePixelShader(
        psByteCode.data(),
        psByteCode.size(),
        nullptr,
        &shader->m_pixelShader);
    GUARANTEE_OR_DIE(SUCCEEDED(hr), Stringf("Could not create pixel shader for '%s'", shaderName));

    D3D11_INPUT_ELEMENT_DESC const *inputElementDesc = nullptr;
    UINT inputElementCount = 0;

    switch (vertexType)
    {
    case VertexType::PCU:
    {
        static D3D11_INPUT_ELEMENT_DESC const pcuDesc[] =
            {
                {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
                {"COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
                {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
            };

        inputElementDesc = pcuDesc;
        inputElementCount = (UINT)ARRAYSIZE(pcuDesc);
        break;
    }

    case VertexType::PCUTBN:
    {
        static D3D11_INPUT_ELEMENT_DESC const pcutbnDesc[] =
            {
                {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
                {"COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
                {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
                {"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
                {"BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
                {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
            };

        inputElementDesc = pcutbnDesc;
        inputElementCount = (UINT)ARRAYSIZE(pcutbnDesc);
        break;
    }

    default:
    {
        ERROR_AND_DIE("Unsupported vertex type");
        break;
    }
    }

    hr = m_device->CreateInputLayout(
        inputElementDesc,
        inputElementCount,
        vsByteCode.data(),
        (UINT)vsByteCode.size(),
        &shader->m_inputLayout);
    GUARANTEE_OR_DIE(SUCCEEDED(hr), Stringf("Could not create input layout for '%s'", shaderName));

    m_loadedShaders.push_back(shader);

    return shader;
}

bool Renderer::CompileShaderToByteCode(
    std::vector<unsigned char> &outByteCode,
    char const *name,
    char const *source,
    char const *entryPoint,
    char const *target)
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

    ID3DBlob *shaderBlob = nullptr;
    ID3DBlob *errorBlob = nullptr;

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
        &errorBlob);

    if (!SUCCEEDED(hr))
    {
        if (errorBlob)
        {
            DebuggerPrintf((char *)errorBlob->GetBufferPointer());
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

void Renderer::BindVertexBuffer(VertexBuffer *vertexBuffer)
{
    GUARANTEE_OR_DIE(m_deviceContext, "BindVertexBuffer: m_deviceContext is null");

    if (vertexBuffer == nullptr)
    {
        ID3D11Buffer *nullBuf = nullptr;
        UINT stride = 0;
        UINT offset = 0;
        m_deviceContext->IASetVertexBuffers(0, 1, &nullBuf, &stride, &offset);
        return;
    }

    UINT stride = vertexBuffer->GetStride();
    UINT offset = 0;
    ID3D11Buffer *buf = vertexBuffer->m_buffer;
    m_deviceContext->IASetVertexBuffers(0, 1, &buf, &stride, &offset);
}

void Renderer::BindConstantBuffer(ConstantBuffer *constantBuffer, int slot)
{
    GUARANTEE_OR_DIE(m_deviceContext, "BindConstantBuffer: m_deviceContext is null");

    if (constantBuffer == nullptr)
    {
        ID3D11Buffer *nullBuf = nullptr;
        m_deviceContext->VSSetConstantBuffers(slot, 1, &nullBuf);
        m_deviceContext->PSSetConstantBuffers(slot, 1, &nullBuf);
        return;
    }

    ID3D11Buffer *buf = constantBuffer->m_buffer;
    GUARANTEE_OR_DIE(buf, "BindConstantBuffer: constantBuffer->m_buffer is null");

    m_deviceContext->VSSetConstantBuffers(slot, 1, &buf);
    m_deviceContext->PSSetConstantBuffers(slot, 1, &buf);
}

void Renderer::BindIndexBuffer(IndexBuffer *indexBuffer)
{
    GUARANTEE_OR_DIE(m_deviceContext, "BindVertexBuffer: m_deviceContext is null");

    if (indexBuffer == nullptr)
    {
        ID3D11Buffer *nullBuf = nullptr;
        UINT stride = 0;
        UINT offset = 0;
        m_deviceContext->IASetVertexBuffers(0, 1, &nullBuf, &stride, &offset);
        return;
    }

    ID3D11Buffer *buf = indexBuffer->m_buffer;
    m_deviceContext->IASetIndexBuffer(buf, DXGI_FORMAT_R32_UINT, 0);
}

#pragma endregion
