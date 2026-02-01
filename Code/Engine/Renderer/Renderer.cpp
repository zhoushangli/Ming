#include "Engine/Renderer/Renderer.hpp"

#include "Engine/Core/Engine.hpp"
#include "Engine/Core/Vertex.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Renderer/Texture.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "ThirdParty/stb/stb_image.h"

#include <Windows.h>
#include <d3dcompiler.h>
#include <d3d11.h>

#pragma comment( lib, "d3d11.lib" )
#pragma comment( lib, "dxgi.lib" )
#pragma comment( lib, "d3dcompiler.lib" )

#if defined( ENGINE_DEBUG_RENDER )
#include <dxgidebug.h>
#pragma comment( lib, "dxguid.lib" )
#endif

HGLRC g_openGLRenderingContext = nullptr;

const char* defaultShaderSource = R"(
struct VS_INPUT
{
    float3 position : POSITION;
};

struct VS_OUTPUT
{
    float4 position : SV_Position;
};

VS_OUTPUT VertexMain(VS_INPUT input)
{
    VS_OUTPUT output;
    output.position = float4(input.position, 1.0f);
    return output;
}

float4 PixelMain(VS_OUTPUT input) : SV_Target
{
    return float4(1.f, 1.f, 1.f, 1.f);
}
)";

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

    // Create device and swap chain
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

    // Get back buffer texture
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

    // Create debug module
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

    m_currentShader = CreateShader("Default", defaultShaderSource);
    BindShader(m_currentShader);
    
//     Vertex vertices[] = {
//         Vertex(Vec3(-0.50f, -0.50f, 0.0f), Rgba8(255, 255, 255, 255), Vec2(0.0f, 0.0f)),
//         Vertex(Vec3(0.00f,  0.50f, 0.0f), Rgba8(255, 255, 255, 255), Vec2(0.0f, 0.0f)),
//         Vertex(Vec3(0.50f, -0.50f, 0.0f), Rgba8(255, 255, 255, 255), Vec2(0.0f, 0.0f)),
//     };
// 
//     m_currentVertexBuffer = CreateVertexBuffer(sizeof(vertices), sizeof(Vertex));
//     CopyCPUToGPU(vertices, sizeof(vertices), m_currentVertexBuffer);
//     BindVertexBuffer(m_currentVertexBuffer);

    // Set rasterizer state
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
}

void Renderer::Shutdown()
{
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
    blendMode;
}


void Renderer::BeginCamera(Camera const& camera)
{	
    Vec2 cameraTopLeft = Vec2(camera.GetLeft(), camera.GetBottom()); // Top-Left in D3D is equal to Bottom-Left in our camera
    Vec2 camDimensions = camera.GetDimensions();

    // Set viewport
    D3D11_VIEWPORT viewport = {};
    viewport.TopLeftX = cameraTopLeft.x;
    viewport.TopLeftY = cameraTopLeft.y;
    viewport.Width = camDimensions.x;
    viewport.Height = camDimensions.y;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;

    DebuggerPrintf("%f --- %f \n", (float)g_engine->m_window->GetClientDimensions().x, (float)g_engine->m_window->GetClientDimensions().y);

    m_deviceContext->RSSetViewports(1, &viewport);
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
}

void Renderer::DrawVertexBuffer(VertexBuffer* vertexBuffer, unsigned int vertexCount)
{
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


Texture* Renderer::CreateTextureFromFile(char const* imageFilePath)
{
	IntVec2 dimensions = IntVec2::ZERO;		// This will be filled in for us to indicate image width & height
	int bytesPerTexel = 0;					// ...and how many color components the image had (e.g. 3=RGB=24bit, 4=RGBA=32bit)

	// Load (and decompress) the image RGB(A) bytes from a file on disk into a memory buffer (array of bytes)
	stbi_set_flip_vertically_on_load(true); // We prefer uvTexCoords has origin (0,0) at BOTTOM LEFT
	unsigned char* texelData = stbi_load(imageFilePath, &dimensions.x, &dimensions.y, &bytesPerTexel, 0);
    stbi_set_flip_vertically_on_load(false);

	// Check if the load was successful
	GUARANTEE_OR_DIE(texelData, Stringf("Failed to load image \"%s\"", imageFilePath));

	Texture* newTexture = CreateTextureFromData(imageFilePath, dimensions, bytesPerTexel, texelData);

	// Free the raw image texel data now that we've sent a copy of it down to the GPU to be stored in video memory
	stbi_image_free(texelData);

	return newTexture;
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
        nullptr,
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
        m_deviceContext->VSSetShader(nullptr, nullptr, 0);
        m_deviceContext->PSSetShader(nullptr, nullptr, 0);
        m_deviceContext->IASetInputLayout(nullptr);
        return;
    }

    m_deviceContext->IASetInputLayout(shader->m_inputLayout);
    m_deviceContext->VSSetShader(shader->m_vertexShader, nullptr, 0);
    m_deviceContext->PSSetShader(shader->m_pixelShader, nullptr, 0);
}

VertexBuffer* Renderer::CreateVertexBuffer(const unsigned int size, unsigned int stride)
{
    return new VertexBuffer(m_device, size, stride);
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

Texture* Renderer::CreateTextureFromData(char const* name, IntVec2 dimensions, int bytesPerTexel, uint8_t* texelData)
{
    bytesPerTexel;

    // Check if the load was successful
    GUARANTEE_OR_DIE(texelData, Stringf("CreateTextureFromData failed for \"%s\" - texelData was null!", name));
    GUARANTEE_OR_DIE(dimensions.x > 0 && dimensions.y > 0, Stringf("CreateTextureFromData failed for \"%s\" - illegal texture dimensions (%i x %i)", name, dimensions.x, dimensions.y));

    Texture* newTexture = new Texture();
    newTexture->m_name = name; // NOTE: m_name must be a std::string, otherwise it may point to temporary data!
    newTexture->m_dimensions = dimensions;

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
	texture;
}

void Renderer::DrawVertexArray(std::vector<Vertex> const& verts) const
{
	DrawVertexArray(static_cast<int>(verts.size()), verts.data());
}
