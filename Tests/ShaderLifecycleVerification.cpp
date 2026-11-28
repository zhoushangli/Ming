#define NOMINMAX
#include "MingEngine/Engine/Render/BuiltinShaders.hpp"

#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

namespace
{
using Microsoft::WRL::ComPtr;
ComPtr<ID3D11Device> device;
ComPtr<ID3D11DeviceContext> context;
int checks = 0;

void Check(bool passed, char const* message)
{
	++checks;
	if (!passed)
	{
		throw std::runtime_error(message);
	}
}

void Require(HRESULT result)
{
	Check(SUCCEEDED(result), "D3D11 operation failed");
}

bool Near(float a, float b)
{
	return std::abs(a - b) < 0.0001f;
}

ComPtr<ID3DBlob> Compile(std::string const& source, char const* entry, char const* target,
	char const* path = "Surface.hlsl", std::string* diagnostics = nullptr)
{
	ComPtr<ID3DBlob> code;
	ComPtr<ID3DBlob> errors;
	HRESULT result = D3DCompile(source.data(), source.size(), path, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		entry, target, D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_WARNINGS_ARE_ERRORS,
		0, &code, &errors);
	if (diagnostics != nullptr)
	{
		*diagnostics = errors ? static_cast<char const*>(errors->GetBufferPointer()) : "";
		return SUCCEEDED(result) ? code : nullptr;
	}

	if (FAILED(result))
	{
		throw std::runtime_error(errors ? static_cast<char const*>(errors->GetBufferPointer()) : "compile failed");
	}
	return code;
}

ComPtr<ID3D11Buffer> Buffer(void const* data, UINT size, UINT bind)
{
	D3D11_BUFFER_DESC description = {};
	description.ByteWidth = size;
	description.BindFlags = bind;
	D3D11_SUBRESOURCE_DATA initial = { data, 0, 0 };
	ComPtr<ID3D11Buffer> buffer;
	Require(device->CreateBuffer(&description, data ? &initial : nullptr, &buffer));
	return buffer;
}

void Constant(UINT slot, void const* data, UINT size)
{
	auto buffer = Buffer(data, size, D3D11_BIND_CONSTANT_BUFFER);
	ID3D11Buffer* pointer = buffer.Get();
	context->VSSetConstantBuffers(slot, 1, &pointer);
	context->PSSetConstantBuffers(slot, 1, &pointer);
}

void Identity(float* matrix)
{
	for (int i = 0; i < 16; ++i)
	{
		matrix[i] = i % 5 == 0 ? 1.f : 0.f;
	}
}

void BindConstants()
{
	float camera[80] = {};
	for (int i = 0; i < 5; ++i)
	{
		Identity(camera + i * 16);
	}
	// 1) Keep view direction finite while leaving the projection path simple.
	camera[4 * 16 - 2] = 2.f;
	Constant(2, camera, sizeof(camera));
	float model[20] = {};
	Identity(model);
	model[16] = 0.5f;
	model[17] = 0.75f;
	model[18] = 1.f;
	model[19] = 0.8f;
	Constant(3, model, sizeof(model));
	std::array<float, 460> lights = {};
	lights[2] = -1.f;
	lights[3] = 0.5f;
	lights[4] = lights[5] = lights[6] = 1.f;
	Constant(1, lights.data(), static_cast<UINT>(sizeof(lights)));
}

void BindVertices(ID3DBlob* code, bool singlePoint)
{
	D3D11_INPUT_ELEMENT_DESC const elements[] = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 36, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 48, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 60, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};
	ComPtr<ID3D11InputLayout> layout;
	Require(device->CreateInputLayout(elements, 6, code->GetBufferPointer(), code->GetBufferSize(), &layout));
	context->IASetInputLayout(layout.Get());
	float vertices[3][18] = {
		{-1,-1,0.5f, 0.8f,0.6f,0.4f,0.5f, 0.25f,0.5f, 1,0,0, 0,1,0, 0,0,1},
		{-1, 3,0.5f, 0.8f,0.6f,0.4f,0.5f, 0.25f,0.5f, 1,0,0, 0,1,0, 0,0,1},
		{ 3,-1,0.5f, 0.8f,0.6f,0.4f,0.5f, 0.25f,0.5f, 1,0,0, 0,1,0, 0,0,1}
	};
	if (singlePoint)
	{
		vertices[0][0] = 2;
		vertices[0][1] = 3;
		vertices[0][2] = 4;
	}
	auto buffer = Buffer(vertices, sizeof(vertices), D3D11_BIND_VERTEX_BUFFER);
	ID3D11Buffer* pointer = buffer.Get();
	UINT stride = sizeof(vertices[0]);
	UINT offset = 0;
	context->IASetVertexBuffers(0, 1, &pointer, &stride, &offset);
	context->IASetPrimitiveTopology(singlePoint ? D3D11_PRIMITIVE_TOPOLOGY_POINTLIST : D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void VerifyVertex()
{
	// 1) Use distinct model, view and projection transforms to check callback ordering.
	float model[20] = {};
	Identity(model);
	model[0] = 2;
	model[5] = 3;
	model[10] = 4;
	model[12] = 10;
	model[13] = 20;
	model[14] = 30;
	Constant(3, model, sizeof(model));
	float camera[80] = {};
	for (int i = 0; i < 5; ++i)
	{
		Identity(camera + i * 16);
	}
	camera[12] = -10;
	camera[13] = -20;
	camera[14] = -30;
	camera[32] = camera[37] = camera[42] = 0.1f;
	Constant(2, camera, sizeof(camera));
	std::string source = BuiltinShaders::BuildSurfaceShaderSource(
		"void vertex(inout VertexContext v) { v.modelPosition.y += 1; v.uv *= 2; v.color.r = 0.25; }\n"
		"void fragment(inout FragmentContext f) {}", "MoveUp.hlsl");
	source += "\n[maxvertexcount(1)] void Capture(point VertexToFragment v[1], inout PointStream<VertexToFragment> stream) { stream.Append(v[0]); }";
	auto vertexCode = Compile(source, "VertexMain", "vs_5_0");
	auto geometryCode = Compile(source, "Capture", "gs_5_0");
	ComPtr<ID3D11VertexShader> vertex;
	Require(device->CreateVertexShader(vertexCode->GetBufferPointer(), vertexCode->GetBufferSize(), nullptr, &vertex));
	context->VSSetShader(vertex.Get(), nullptr, 0);
	D3D11_SO_DECLARATION_ENTRY const declarations[] = {
		{0, "SV_Position", 0, 0, 4, 0}, {0, "WORLDPOS", 0, 0, 3, 0},
		{0, "COLOR", 0, 0, 4, 0}, {0, "TEXCOORD", 0, 0, 2, 0}
	};
	UINT stride = 13 * sizeof(float);
	ComPtr<ID3D11GeometryShader> geometry;
	Require(device->CreateGeometryShaderWithStreamOutput(geometryCode->GetBufferPointer(), geometryCode->GetBufferSize(),
		declarations, 4, &stride, 1, D3D11_SO_NO_RASTERIZED_STREAM, nullptr, &geometry));
	context->GSSetShader(geometry.Get(), nullptr, 0);
	auto output = Buffer(nullptr, stride, D3D11_BIND_STREAM_OUTPUT);
	ID3D11Buffer* pointer = output.Get();
	UINT offset = 0;
	context->SOSetTargets(1, &pointer, &offset);
	BindVertices(vertexCode.Get(), true);
	context->Draw(1, 0);
	context->SOSetTargets(0, nullptr, nullptr);
	context->GSSetShader(nullptr, nullptr, 0);
	D3D11_BUFFER_DESC description = {};
	description.ByteWidth = stride;
	description.Usage = D3D11_USAGE_STAGING;
	description.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	ComPtr<ID3D11Buffer> staging;
	Require(device->CreateBuffer(&description, nullptr, &staging));
	context->CopyResource(staging.Get(), output.Get());
	D3D11_MAPPED_SUBRESOURCE mapped = {};
	Require(context->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped));
	float values[13];
	std::memcpy(values, mapped.pData, sizeof(values));
	context->Unmap(staging.Get(), 0);
	Check(Near(values[0], 0.4f) && Near(values[1], 1.2f) && Near(values[2], 1.6f) && Near(values[3], 1), "modified position reaches model, view and clip transforms");
	Check(Near(values[4], 14) && Near(values[5], 32) && Near(values[6], 46), "modified world position is forwarded");
	Check(Near(values[7], 0.25f) && Near(values[11], 0.5f) && Near(values[12], 1), "vertex color and UV are forwarded");
	BindConstants();
}

std::array<float, 8> Draw(std::string const& callbacks, bool completeSource = false)
{
	auto source = completeSource ? callbacks : BuiltinShaders::BuildSurfaceShaderSource(callbacks, "FragmentTest.hlsl");
	auto vertexCode = Compile(source, "VertexMain", "vs_5_0");
	auto fragmentCode = Compile(source, "FragmentMain", "ps_5_0");
	ComPtr<ID3D11VertexShader> vertex;
	ComPtr<ID3D11PixelShader> fragment;
	Require(device->CreateVertexShader(vertexCode->GetBufferPointer(), vertexCode->GetBufferSize(), nullptr, &vertex));
	Require(device->CreatePixelShader(fragmentCode->GetBufferPointer(), fragmentCode->GetBufferSize(), nullptr, &fragment));
	context->VSSetShader(vertex.Get(), nullptr, 0);
	context->PSSetShader(fragment.Get(), nullptr, 0);
	BindVertices(vertexCode.Get(), false);

	D3D11_TEXTURE2D_DESC description = {};
	description.Width = description.Height = description.MipLevels = description.ArraySize = description.SampleDesc.Count = 1;
	description.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	description.BindFlags = D3D11_BIND_RENDER_TARGET;
	ComPtr<ID3D11Texture2D> targets[2];
	ComPtr<ID3D11RenderTargetView> views[2];
	float clear[4] = {-9,-9,-9,-9};
	for (int i = 0; i < 2; ++i)
	{
		Require(device->CreateTexture2D(&description, nullptr, &targets[i]));
		Require(device->CreateRenderTargetView(targets[i].Get(), nullptr, &views[i]));
		context->ClearRenderTargetView(views[i].Get(), clear);
	}
	ID3D11RenderTargetView* pointers[] = {views[0].Get(), views[1].Get()};
	context->OMSetRenderTargets(2, pointers, nullptr);
	D3D11_VIEWPORT viewport = {0,0,1,1,0,1};
	context->RSSetViewports(1, &viewport);
	D3D11_RASTERIZER_DESC rasterizer = {};
	rasterizer.FillMode = D3D11_FILL_SOLID;
	rasterizer.CullMode = D3D11_CULL_NONE;
	rasterizer.DepthClipEnable = true;
	ComPtr<ID3D11RasterizerState> state;
	Require(device->CreateRasterizerState(&rasterizer, &state));
	context->RSSetState(state.Get());
	context->Draw(3, 0);
	context->OMSetRenderTargets(0, nullptr, nullptr);

	description.BindFlags = 0;
	description.Usage = D3D11_USAGE_STAGING;
	description.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	ComPtr<ID3D11Texture2D> staging;
	Require(device->CreateTexture2D(&description, nullptr, &staging));
	std::array<float, 8> result;
	for (int i = 0; i < 2; ++i)
	{
		context->CopyResource(staging.Get(), targets[i].Get());
		D3D11_MAPPED_SUBRESOURCE mapped = {};
		Require(context->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped));
		std::memcpy(result.data() + i * 4, mapped.pData, 4 * sizeof(float));
		context->Unmap(staging.Get(), 0);
	}
	return result;
}

void VerifyFragments()
{
	// 1) Bind a two-texel texture so changing vertex UV has an observable result.
	float texels[] = {0.2f,0.4f,0.6f,0.5f, 1,0,0,1};
	D3D11_TEXTURE2D_DESC description = {};
	description.Width = 2;
	description.Height = description.MipLevels = description.ArraySize = description.SampleDesc.Count = 1;
	description.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	description.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	D3D11_SUBRESOURCE_DATA initial = {texels, sizeof(texels), 0};
	ComPtr<ID3D11Texture2D> texture;
	ComPtr<ID3D11ShaderResourceView> view;
	Require(device->CreateTexture2D(&description, &initial, &texture));
	Require(device->CreateShaderResourceView(texture.Get(), nullptr, &view));
	ID3D11ShaderResourceView* pointer = view.Get();
	context->PSSetShaderResources(0, 1, &pointer);
	D3D11_SAMPLER_DESC samplerDescription = {};
	samplerDescription.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	samplerDescription.AddressU = samplerDescription.AddressV = samplerDescription.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDescription.MaxLOD = D3D11_FLOAT32_MAX;
	ComPtr<ID3D11SamplerState> sampler;
	Require(device->CreateSamplerState(&samplerDescription, &sampler));
	ID3D11SamplerState* samplerPointer = sampler.Get();
	context->PSSetSamplers(0, 1, &samplerPointer);
	auto baseline = Draw("void vertex(inout VertexContext v) {}\nvoid fragment(inout FragmentContext f) {}");
	Check(Near(baseline[0], 0.2f * 0.8f * 0.5f * 0.7f) && Near(baseline[1], 0.4f * 0.6f * 0.75f * 0.7f)
		&& Near(baseline[2], 0.6f * 0.4f * 0.7f) && Near(baseline[3], 0.5f * 0.5f * 0.8f), "empty callbacks preserve texture, tint, lighting and alpha");
	Check(Near(baseline[4], 0.5f) && Near(baseline[5], 0.5f) && Near(baseline[6], 1), "default scene normal encoding");
	auto red = Draw("void vertex(inout VertexContext v) {}\nvoid fragment(inout FragmentContext f) { f.color.rgb = float3(1,0,0); }");
	Check(Near(red[0], 0.7f) && Near(red[1], 0) && Near(red[2], 0), "fragment color is lit after callback");
	auto normal = Draw("void vertex(inout VertexContext v) {}\nvoid fragment(inout FragmentContext f) { f.worldNormal = float3(0,1,0); }");
	Check(Near(normal[0], baseline[0] * 0.2f / 0.7f) && Near(normal[5], 1) && Near(normal[6], 0.5f), "modified normal controls lighting and scene output");
	auto vertexNormal = Draw("void vertex(inout VertexContext v) { v.modelNormal = float3(0,1,0); }\nvoid fragment(inout FragmentContext f) {}");
	Check(vertexNormal == normal, "vertex normal reaches fragment lighting and scene output");
	auto uv = Draw("void vertex(inout VertexContext v) { v.uv.x = 0.75; v.color.r = 0.25; }\nvoid fragment(inout FragmentContext f) {}");
	Check(Near(uv[0], 0.25f * 0.5f * 0.7f) && Near(uv[1], 0) && Near(uv[3], 0.4f), "vertex UV and color affect default sampling");
	auto readonly = Draw("void vertex(inout VertexContext v) {}\nvoid fragment(inout FragmentContext f) { f.uv = 0.75; f.vertexColor = 0; f.worldPosition = 123; }");
	Check(readonly == baseline, "read-only fields do not change engine results");
	for (char const* body : {"f.color.a = 0.009;", "clip(-1);"})
	{
		auto discarded = Draw(std::string("void vertex(inout VertexContext v) {}\nvoid fragment(inout FragmentContext f) {") + body + "}");
		Check(discarded[0] == -9 && discarded[4] == -9, "alpha and explicit clip discard both outputs");
	}
	auto boundary = Draw("void vertex(inout VertexContext v) {}\nvoid fragment(inout FragmentContext f) { f.color.a = 0.01; }");
	Check(Near(boundary[3], 0.01f), "alpha threshold is inclusive");
	float screen[] = {1, 1, 0.1f, 100};
	Constant(4, screen, sizeof(screen));
	for (auto builtin : {BuiltinShaders::DefaultUI, BuiltinShaders::DefaultUnlit, BuiltinShaders::GizmosAxis,
		BuiltinShaders::GizmosGrid, BuiltinShaders::PostProcessCopy, BuiltinShaders::TransformGizmosArc})
	{
		auto result = Draw(std::string(builtin), true);
		Check(std::isfinite(result[0]) && result[0] >= 0.f && result[3] > 0.f,
			"special built-in shader writes a finite visible fragment after entry rename");
	}
}

void VerifyCompilation(std::filesystem::path const& directory)
{
	for (auto source : {BuiltinShaders::DefaultLit, BuiltinShaders::DefaultUI, BuiltinShaders::DefaultUnlit,
		BuiltinShaders::GizmosAxis, BuiltinShaders::GizmosGrid, BuiltinShaders::PostProcessCopy, BuiltinShaders::TransformGizmosArc})
	{
		Compile(std::string(source), "VertexMain", "vs_5_0");
		Compile(std::string(source), "FragmentMain", "ps_5_0");
	}
	std::filesystem::create_directories(directory);
	auto path = (directory / "Relative.hlsl").string();
	std::ofstream(directory / "Offset.hlsli") << "static const float Offset = 1.0;\n";
	auto source = BuiltinShaders::BuildSurfaceShaderSource(
		"#include \"Offset.hlsli\"\nvoid vertex(inout VertexContext v) { v.modelPosition.y += Offset; }\nvoid fragment(inout FragmentContext f) {}", path);
	Compile(source, "VertexMain", "vs_5_0", path.c_str());
	Compile(BuiltinShaders::BuildSurfaceShaderSource("\xEF\xBB\xBF" + std::string(
		"void vertex(inout VertexContext v) {}\nvoid fragment(inout FragmentContext f) {}"), path),
		"FragmentMain", "ps_5_0", path.c_str());
	std::string errors;
	source = BuiltinShaders::BuildSurfaceShaderSource("void vertex(inout VertexContext v) {}\nvoid fragment(inout FragmentContext f) {\n    missingValue = 1;\n}\n", path);
	Check(!Compile(source, "FragmentMain", "ps_5_0", path.c_str(), &errors), "invalid user source fails");
	Check(errors.find("Relative.hlsl(3,") != std::string::npos, "diagnostic points to user file line 3");
	source = BuiltinShaders::BuildSurfaceShaderSource("void vertex(inout VertexContext v) {}", path);
	Check(!Compile(source, "FragmentMain", "ps_5_0", path.c_str(), &errors), "missing callback fails compilation");
}
}

int main(int argc, char** argv)
{
	try
	{
		Require(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION,
			&device, nullptr, &context));
		VerifyCompilation(argc > 1 ? argv[1] : "../Temporary/ShaderLifecycleVerification");
		BindConstants();
		VerifyVertex();
		VerifyFragments();
		context->ClearState();
		std::printf("SHADER_LIFECYCLE: PASS (D3D11 WARP compilation, stream output and render-target readback)\n");
		return 0;
	}
	catch (std::exception const& error)
	{
		std::fprintf(stderr, "SHADER_LIFECYCLE: FAIL %s\n", error.what());
		return 1;
	}
}
