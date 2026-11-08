#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Render/Renderer.hpp"
#include "MingEngine/Engine/Render/BuiltinShaders.hpp"
#include "MingEngine/Core/Render/Vertex.hpp"
#include "MingEngine/Core/Render/VertexUtils.hpp"
#include "MingEngine/Core/IO/ResourceLoader.hpp"
#include "MingEngine/Scene/RegisterAllTypes.hpp"
#include "MingEngine/Scene/GUI/ThemeDatabase.hpp"
#include "RenderReadback.hpp"

#include <cstdio>

namespace
{
Color ReadBackBuffer(ID3D11Resource* resource)
{
	using Microsoft::WRL::ComPtr;
	ComPtr<ID3D11Texture2D> source;
	resource->QueryInterface(IID_PPV_ARGS(&source));
	ComPtr<ID3D11Device> device;
	source->GetDevice(&device);
	ComPtr<ID3D11DeviceContext> context;
	device->GetImmediateContext(&context);
	D3D11_TEXTURE2D_DESC description;
	source->GetDesc(&description);
	description.Usage = D3D11_USAGE_STAGING;
	description.BindFlags = 0;
	description.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	description.MiscFlags = 0;
	ComPtr<ID3D11Texture2D> staging;
	if (FAILED(device->CreateTexture2D(&description, nullptr, &staging)))
	{
		throw std::runtime_error("Cannot allocate back buffer readback.");
	}

	context->CopyResource(staging.Get(), source.Get());
	D3D11_MAPPED_SUBRESOURCE mapped;
	if (FAILED(context->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped)))
	{
		throw std::runtime_error("Cannot read back buffer pixels.");
	}

	auto const* pixel = static_cast<uint8_t const*>(mapped.pData) + 16 * mapped.RowPitch + 16 * 4;
	Color const result(pixel[0], pixel[1], pixel[2], pixel[3]);
	context->Unmap(staging.Get(), 0);
	return result;
}
}

int main(int argc, char** argv)
{
	if (argc != 2)
	{
		return 2;
	}

	int failures = 0;
	int checks = 0;
	auto check = [&](bool result, char const* message)
	{
		++checks;
		if (!result)
		{
			++failures;
			std::printf("FAIL: %s\n", message);
		}
	};
	EngineConfig config;
	config.m_fileSystemConfig.m_resourceRoot = std::filesystem::absolute(argv[1]);
	std::filesystem::create_directories(config.m_fileSystemConfig.m_resourceRoot);
	config.m_scriptSystemConfig.m_isEnabled = false;
	config.m_audioConfig.m_isEnable = false;
	config.m_networkConfig.m_isEnable = false;
	config.m_rendererServerConfig.m_isEnable = false;
	config.m_windowConfig.m_appName = "Render Composition Verification";
	config.m_windowConfig.m_isBorderlessFullscreen = false;
	Engine engine(config);
	ClassDatabase::Startup();
	RegisterAllTypes();
	engine.Startup();
	Renderer renderer(RendererServerConfig{});
	renderer.Startup();
	{
		ViewportData viewport;
		renderer.ResizeViewport(&viewport, IntVec2(64, 64));
		auto shader = renderer.GetBuiltinShaderResource("DefaultUI", BuiltinShaders::DefaultUI);
		std::vector<Vertex> vertices;
		AddVertsForQuad3D(vertices, Vector3(8, 8, 0), Vector3(24, 8, 0), Vector3(24, 24, 0),
			Vector3(8, 24, 0), Color(0, 255, 0, 128));
		VertexBuffer* buffer = renderer.CreateVertexBuffer(vertices.data(),
			static_cast<unsigned>(vertices.size() * sizeof(Vertex)), sizeof(Vertex));
		RenderRequest request;
		request.m_vertexBuffer = buffer;
		request.m_shader = shader->GetShader();
		request.m_blendMode = BlendMode::ALPHA;
		request.m_depthMode = DepthMode::READ_ONLY_ALWAYS;
		request.m_rasterizerMode = RasterizerMode::SOLID_CULL_NONE;
		auto& requests = viewport.m_renderRequests[static_cast<size_t>(RenderRequestPass::UI)];
		requests.push_back(request);
		viewport.m_clearColor = Color(32, 32, 32, 255);
		renderer.ClearSceneTargets(&viewport);
		renderer.RenderViewport(&viewport, nullptr);
		check(ReadPixel(viewport.m_viewportOutputTexture, 16, 16).a == 255,
			"translucent UI preserves an opaque target alpha");
		viewport.m_clearColor = Color(0, 0, 0, 0);
		renderer.ClearSceneTargets(&viewport);
		renderer.RenderViewport(&viewport, nullptr);
		check(ReadPixel(viewport.m_viewportOutputTexture, 16, 16).a == 128,
			"one translucent layer retains coverage on a transparent target");
		requests.push_back(request);
		renderer.ClearSceneTargets(&viewport);
		renderer.RenderViewport(&viewport, nullptr);
		check(ReadPixel(viewport.m_viewportOutputTexture, 16, 16).a == 192,
			"overlapping translucent layers accumulate alpha coverage");

		using Microsoft::WRL::ComPtr;
		ComPtr<ID3D11Resource> output;
		viewport.m_viewportOutputTexture->GetShaderResourceView()->GetResource(&output);
		ComPtr<ID3D11Device> device;
		output->GetDevice(&device);
		ComPtr<ID3D11DeviceContext> context;
		device->GetImmediateContext(&context);
		renderer.BindBackBuffer();
		ComPtr<ID3D11RenderTargetView> backView;
		context->OMGetRenderTargets(1, &backView, nullptr);
		ComPtr<ID3D11Resource> back;
		backView->GetResource(&back);
		uint8_t const rgba[] = { 16, 144, 16, 128 };
		GPUTexture* source = renderer.CreateGPUTexture("CompositionProbe", IntVec2(1, 1), 4, rgba);
		float const red[] = { 1, 0, 0, 1 };
		float const blue[] = { 0, 0, 1, 1 };
		context->ClearRenderTargetView(backView.Get(), red);
		renderer.CopyTextureToBackBuffer(source);
		Color const fromRed = ReadBackBuffer(back.Get());
		context->ClearRenderTargetView(backView.Get(), blue);
		renderer.CopyTextureToBackBuffer(source);
		Color const fromBlue = ReadBackBuffer(back.Get());
		check(fromRed == Color(16, 144, 16, 128), "final copy writes the current pixels without alpha blending");
		check(fromBlue == fromRed, "final output is independent of previous back buffer pixels");
		requests.clear();
		viewport.m_clearColor = Color(32, 32, 32, 255);
		renderer.ClearSceneTargets(&viewport);
		renderer.RenderViewport(&viewport, nullptr);
		check(ReadPixel(viewport.m_viewportOutputTexture, 16, 16) == viewport.m_clearColor,
			"removing translucent drawing restores the background without a trail");
		context->ClearState();
		renderer.DestroyTexture(source);
		delete buffer;
		renderer.DestroyViewportResources(&viewport);
	}
	ThemeDatabase::Shutdown();
	ResourceLoader::Shutdown();
	renderer.Shutdown();
	engine.Shutdown();
	ClassDatabase::Shutdown();
	std::printf("RENDER_COMPOSITION checks=%d failures=%d\n", checks, failures);
	return failures == 0 ? 0 : 1;
}
