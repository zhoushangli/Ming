#include "MingEngine/Scene/Resource/StandardMaterial2D.hpp"
#include "MingEngine/Scene/Resource/StandardMaterial3D.hpp"
#include "MingEngine/Core/Object/ClassDatabase.hpp"
#define NOMINMAX
#include "MingEngine/Engine/Application/App.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Render/GPUTexture.hpp"
#include "MingEngine/Core/IO/ResourceLoader.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"
#include "MingEngine/Scene/Core/Viewport.hpp"
#include "MingEngine/Scene/3D/Camera3D.hpp"
#include "MingEngine/Scene/3D/Light3D.hpp"
#include "MingEngine/Scene/3D/MeshInstance3D.hpp"
#include "MingEngine/Scene/GUI/Label.hpp"
#include "MingEngine/Scene/GUI/TextureRect.hpp"
#include "MingEngine/Core/IO/Image.hpp"
#include "MingEngine/Scene/Resource/ShaderMaterial.hpp"
#include "MingEngine/Scene/Resource/SurfaceTool.hpp"
#include "ThirdParty/GLFW/glfw3.h"
#include <d3d11.h>
#include <wrl/client.h>
#include <filesystem>
#include <fstream>
#include <cstdio>
#include <cmath>
#include <stdexcept>

namespace
{
class SnapshotMaterial : public ShaderMaterial
{
protected:
	void PrepareShader() override
	{
		if (m_target.IsValid())
		{
			m_target->SetShaderParameter("rgb", Vector3(0.f, 0.f, 1.f));
			m_target = nullptr;
		}
	}

public:
	Ref<ShaderMaterial> m_target;
};

Ref<SnapshotMaterial> snapshotMaterial;
std::vector<Ref<ShaderMaterial>> parameterMaterials;
std::vector<Label*> meshLabels;
std::vector<Label*> materialLabels;
std::vector<TextureRect*> canvasSamples;
MeshInstance3D* standardPreview = nullptr;

void UpdatePreviewLayout(App& app)
{
	float height = static_cast<float>(app.GetEngine()->m_windowSystem->GetClientDimensions().y);
	for (size_t index = 0; index < meshLabels.size(); ++index)
	{
		meshLabels[index]->SetPosition(Vector2((2.f + (index % 3) * 3.f) * height / 8.f - 190.f,
			(1.f - (index < 3 ? 3.f : 6.f) / 8.f) * height + height * 0.75f / 8.f));
	}
	for (size_t index = 0; index < materialLabels.size(); ++index)
	{
		materialLabels[index]->SetPosition(Vector2(32.f + index * 220.f, height - 132.f));
	}
	for (size_t index = 0; index < canvasSamples.size(); ++index)
	{
		canvasSamples[index]->SetPosition(Vector2(32.f + index * 220.f, height - 100.f));
	}
	if (standardPreview != nullptr && height > 0.f)
	{
		standardPreview->SetLocalPosition(Vector3(772.f * 8.f / height, 60.f * 8.f / height, 0.f));
	}
}


void Check(bool passed, char const* message)
{
	if (!passed)
	{
		throw std::runtime_error(message);
	}
}

void VerifyParsing()
{
	MShaderParseResult result;
	std::string error;
	auto parse = [&](std::string const& source)
	{
		return MShaderParser::Parse(source, "Case.mshader", result, error);
	};
	Check(parse("shader_type spatial;"), "shader type alone is valid");
	Check(!result.m_hasVertex && !result.m_hasFragment && result.m_uniforms.empty(), "both callbacks may be omitted");
	Check(parse("\xEF\xBB\xBF// uniform fake {\nshader_type spatial;\nuniform float a = -1.25e-2;\nuniform float2 b = float2(+1, 2.f);\nuniform float3 c;\nuniform float4 d = float4(1,2,3,4);\nstruct Extra { float x; };\nfloat helper(float x) { if (x > 0) { return x; } return 0; }\nvoid vertex(inout VertexContext arbitrary) { arbitrary.modelPosition.x += helper(a); }"), "top level parser preserves ordinary HLSL");
	Check(result.m_hasVertex && !result.m_hasFragment && result.m_uniforms.size() == 4, "callbacks and uniform declarations recognized");
	Check(std::abs(result.m_uniforms[0].m_defaultValue.As<float>() + 0.0125f) < 0.00001f, "signed exponent default");
	Check(result.m_uniforms[1].m_defaultValue.As<Vector2>() == Vector2(1.f, 2.f), "vector default");
	Check(result.m_uniforms[2].m_defaultValue.As<Vector3>() == Vector3::Zero, "implicit zero vector");
	Check(parse("shader_type spatial;\ngroup_uniforms Appearance;\nuniform float opacity : hint_range(0,1) = 0.8;\nuniform float4 tint : hint_color = float4(0.12345,0.5,0.8,1);\ngroup_uniforms Effect;\nuniform int mode : hint_enum(\"Normal\",\"Glow\") = 1;\nuniform int count : hint_range(-10,10,2) = -10;\ngroup_uniforms;\nuniform int limit = -2147483648;\nuniform int maximum = 2147483647;"), "all uniform hints and group syntax parse");
	Check(result.m_uniforms[0].m_hint == PropertyInfo::Hint::Range && result.m_uniforms[0].m_hintData == "0,1,0.001", "float range gets a default step");
	Check(result.m_uniforms[1].m_defaultValue.As<Color>().r == 0.12345f && result.m_uniforms[1].m_hint == PropertyInfo::Hint::Color, "color uses float Color without losing precision");
	Check(result.m_uniforms[2].m_hintData == "Normal:0,Glow:1" && result.m_uniforms[2].m_defaultValue.As<int>() == 1, "enum retains labels and integer value");
	Check(result.m_uniforms[0].m_group == "Appearance" && result.m_uniforms[2].m_group == "Effect" && result.m_uniforms[4].m_group.empty(), "group changes and termination do not rename parameters");
	Check(result.m_uniforms[1].m_byteSize == 16 && result.m_uniforms[2].m_byteSize == 4 && result.m_uniforms[2].m_offset == 32, "payload bytes remain distinct from padded slots");
	Check(result.m_uniformSource.find("int mode : packoffset(c2)") != std::string::npos, "integer stays an HLSL integer");
	Check(!parse("shader_type spatial;\ngroup_uniforms A;\nuniform int count = 2147483648;") && error.find("Case.mshader:3:") == 0, "integer overflow reports original source line");
	for (char const* invalid : {
		"uniform int x = -2147483649;", "uniform int x = 1.0;", "uniform int x = 1e2;",
		"uniform float x : hint_color;", "uniform float4 x : hint_range(0,1);",
		"uniform float x : hint_enum(\"A\");", "uniform int x : hint_enum();", "uniform int x : hint_enum(\"\");",
		"uniform float x : hint_range(1,0);", "uniform float x : hint_range(0,1,0);",
		"uniform float x : hint_range(0,1,-1);", "uniform int x : hint_range(0,1,0.1);",
		"uniform float x : unknown;", "uniform float4 x : hint_color, hint_color;",
		"uniform float4 x : hint_color : hint_color;", "group_uniforms A.B;",
		"group_uniforms", "uniform float x :", "uniform float x : hint_range(0,",
		"uniform int x : hint_enum(\"A\",", "void helper() {} group_uniforms A;" })
	{
		Check(!parse(std::string("shader_type spatial; ") + invalid), "invalid hint, integer or group rejected");
		Check(result.m_uniforms.empty() && !error.empty(), "parse failures clear partial declarations");
	}
	for (char const* invalid : {
		"", "shader_type canvas_item;", "shader_type spatial; shader_type spatial;",
		"shader_type spatial; uniform bool x;", "shader_type spatial; uniform float x; uniform float x;",
		"shader_type spatial; uniform float x = 1 + 2;", "shader_type spatial; uniform float2 x = float2(1);",
		"shader_type spatial; uniform float x[2];", "shader_type spatial; uniform float x, y;",
		"shader_type spatial; void vertex(VertexContext v) {}", "shader_type spatial; void vertex(inout FragmentContext v) {}",
		"shader_type spatial; void vertex(inout VertexContext v);", "shader_type spatial; void vertex(inout VertexContext v) {} void vertex(inout VertexContext v) {}",
		"shader_type spatial; void helper() {} uniform float x;", "shader_type spatial; void helper() { (]; }",
		"shader_type spatial; /* unfinished", "shader_type spatial; void fragment(inout FragmentContext f) {" })
	{
		Check(!parse(invalid) && error.find("Case.mshader:") == 0, "invalid source rejected with source location");
	}
	Check(parse("#define SCALE 2\nshader_type spatial;\nuniform float x;\nvoid fragment(inout FragmentContext f) { f.color *= SCALE; }"), "preprocessor directive passes through");
	Check(result.m_source.find("#define SCALE 2") != std::string::npos, "directive source preserved");
	Check(parse("#define SCALE \\\n 2\nshader_type spatial;"), "multiline directive ignored by declaration scan");
	std::string large = "shader_type spatial;";
	for (int i = 0; i < 4096; ++i)
	{
		large += "uniform float p" + std::to_string(i) + ";";
	}
	Check(parse(large), "maximum supported constant buffer accepted");
	Check(!parse(large + "uniform float overflow;"), "oversized constant buffer rejected");

	Check(parse("shader_type spatial; uniform float value = 2.0; uniform float2 offset;"), "material metadata source");
	auto shader = CreateRef<ShaderResource>();
	shader->SetUniforms(result.m_uniforms);
	auto material = CreateRef<ShaderMaterial>();
	material->SetShaderResource(shader);
	Check(material->GetShaderParameter("value").As<float>() == 2.f, "material starts with shader defaults");
	Check(material->SetShaderParameter("value", 3.f), "valid material parameter");
	Check(!material->SetShaderParameter("value", 3), "strict parameter type");
	Check(!material->SetShaderParameter("unknown", 3.f), "unknown parameter rejected");
	Check(material->GetShaderParameter("value").As<float>() == 3.f, "rejected setter preserves value");
	Check(material->SetShaderParameter("value", 3.f), "same value accepted");
	auto replacement = CreateRef<ShaderResource>();
	replacement->SetUniforms(result.m_uniforms);
	material->SetShaderResource(replacement);
	material->SetShaderResource(shader);
	Check(material->GetShaderParameter("value").As<float>() == 3.f, "shader replacement preserves matching values");
	Check(material->SetShaderParameter("value", 4.f), "set before reload");
	auto reloaded = CreateRef<ShaderResource>();
	result.m_uniforms[0].m_defaultValue = Variant(5.f);
	reloaded->SetUniforms(result.m_uniforms);
	Check(shader->MoveFrom(std::move(*reloaded)), "reload shader metadata");
	Check(material->GetShaderParameter("value").As<float>() == 4.f, "resource revision preserves matching values");

	Check(ClassDatabase::GetConstructor("MaterialResource") == nullptr, "base material cannot be constructed through reflection");
	auto standard2D = CreateRef<StandardMaterial2D>();
	auto standard3D = CreateRef<StandardMaterial3D>();
	Check(!standard2D->HasMethod("SetDepthMode") && !standard2D->HasMethod("SetDiffuseTexture")
		&& !standard2D->HasMethod("SetShaderResource"), "2D API excludes 3D and shader replacement properties");
	Check(standard3D->HasMethod("SetNormalTexture") && !standard3D->HasMethod("SetShaderParameter")
		&& !standard3D->HasMethod("SetShaderResource"), "3D exposes only its fixed contract");
	Check(!material->HasMethod("SetTint") && material->HasMethod("SetShaderParameter"), "custom material has no standard surface API");
	standard2D->SetTint(Color::Blue);
	Check(standard2D->GetTint() == Color::Blue && standard3D->GetTint() == Color::White, "independent standard parameter caches");
	auto texture = CreateRef<TextureResource>();
	standard3D->SetDiffuseTexture(texture);
	auto standardCopy = CreateRef<StandardMaterial3D>();
	Check(standardCopy->MoveFrom(std::move(*standard3D)) && standardCopy->GetDiffuseTexture() == texture,
		"resource copy retains texture ownership and values");
	standardCopy->SetTint(Color::Red);
	Check(standard3D->GetTint() == Color::White, "copy uses an independent cache");
	Check(!standardCopy->MoveFrom(std::move(*standard2D)), "resource copy rejects another concrete material type");

	auto copied = CreateRef<ShaderMaterial>();
	Check(copied->MoveFrom(std::move(*material)) && copied->GetShaderParameter("value").As<float>() == 4.f,
		"custom material copy preserves parameter overrides");
	copied->SetShaderParameter("value", 7.f);
	Check(material->GetShaderParameter("value").As<float>() == 4.f, "custom material copies do not share mutable values");
	Check(parse("shader_type spatial; uniform float2 value = float2(1,2); uniform float added = 9;"), "changed schema source");
	shader->SetUniforms(result.m_uniforms);
	Check(material->GetShaderParameter("value").As<Vector2>() == Vector2(1.f, 2.f), "type changes restore new defaults");
	Check(material->GetShaderParameter("added").As<float>() == 9.f, "new uniform uses declaration default");
	Check(material->GetShaderParameter("offset").IsEmpty(), "removed uniform is discarded");
	material->SetShaderResource({});
	Check(material->GetShaderParameter("added").IsEmpty(), "clearing shader clears parameters");
	std::printf("MSHADER: PASS parser, defaults, signature validation, limits and material cache\n");
}

void Populate(App& app)
{
	Node* root = app.GetEngine()->m_sceneSystem->GetSceneTree()->GetScene();
	auto* overlay = MemNew<Control>();
	root->AddNode(overlay);
	auto* camera = MemNew<Camera3D>();
	camera->SetOrthogonal(8.f, 0.f, 10.f);
	camera->SetLocalPosition(Vector3(0.f, 0.f, -1.f));
	root->AddNode(camera);
	auto* sun = MemNew<DirectionalLight3D>();
	sun->SetWorldForward(Vector3(0.f, 0.f, 1.f));
	sun->SetIntensity(0.6f);
	root->AddNode(sun);
	auto makeMesh = [](Color const& color)
	{
		auto surface = CreateRef<SurfaceTool>();
		surface->SetColor(color);
		surface->SetNormal(Vector3(0.f, 0.f, -1.f));
		surface->SetTangent(Vector3(1.f, 0.f, 0.f));
		surface->SetBitangent(Vector3(0.f, 1.f, 0.f));
		surface->AddVertex(Vector3(-0.6f, -0.6f, 0.5f));
		surface->AddVertex(Vector3(-0.6f, 0.6f, 0.5f));
		surface->AddVertex(Vector3(0.6f, 0.6f, 0.5f));
		surface->AddVertex(Vector3(0.6f, -0.6f, 0.5f));
		for (uint32_t index : {0, 1, 2, 0, 2, 3})
		{
			surface->AddIndex(index);
		}
		return surface->CreateMeshResource();
	};
	auto mesh = makeMesh(Color::White);
	char const* shaders[] = {"res://Empty.mshader", "res://SmallUniforms.mshader", "res://Red.mshader"};
	char const* labels[] = {"Empty callbacks", "Vertex: model Y + 1", "Fragment: red surface"};
	for (int i = 0; i < 6; ++i)
	{
		auto shader = Ref<ShaderResource>(ResourceLoader::Load(i < 3 ? shaders[i] : "res://Uniforms.mshader"));
		Check(shader.IsValid() && shader->GetShader() != nullptr, "load MShader through ResourceLoader");
		if (i >= 4)
		{
			Check(shader == parameterMaterials[0]->GetShaderResource(), "materials share the same cached ShaderResource");
		}
		Ref<ShaderMaterial> material;
		if (i == 5)
		{
			snapshotMaterial = CreateRef<SnapshotMaterial>();
			material = snapshotMaterial;
		}
		else
		{
			material = CreateRef<ShaderMaterial>();
		}
		material->SetShaderResource(shader);
		material->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
		if (i >= 3)
		{
			parameterMaterials.push_back(material);
			if (i == 4)
			{
				Check(material->SetShaderParameter("rgb", Vector3(1.f, 0.f, 0.f)), "set independent red material");
				Check(material->SetShaderParameter("mode", -7), "set signed integer shader parameter");
			}
			if (i == 5)
			{
				Check(material->SetShaderParameter("height", 0.5f), "set scalar");
				Check(material->SetShaderParameter("offset", Vector2(0.25f, 0.f)), "set float2");
				Check(material->SetShaderParameter("multiplier", Color(-0.25f, 1.5f, 0.12345f, 0.87654f)), "set extended float Color");
				Check(material->SetShaderParameter("rgb", Vector3(1.f, 1.f, 1.f)), "set float3");
				Check(material->SetShaderParameter("mode", -8), "select extended color verification");
			}
		}
		auto* instance = MemNew<MeshInstance3D>();
		instance->SetMeshResource(i == 5 ? makeMesh(Color(-0.25f, 1.5f, 0.12345f, 0.87654f)) : mesh);
		instance->SetMaterialOverride(material);
		instance->SetLocalPosition(Vector3(2.f + (i % 3) * 3.f, i < 3 ? 3.f : 6.f, 0.f));
		root->AddNode(instance);
		auto* label = MemNew<Label>();
		label->SetText(i < 3 ? labels[i] : i == 3 ? "Uniform default / runtime update" : i == 4 ? "Shared shader: red" : "Float vertex + uniform: blue");
		float const height = static_cast<float>(app.GetEngine()->m_windowSystem->GetClientDimensions().y);
		label->SetPosition(Vector2((2.f + (i % 3) * 3.f) * height / 8.f - 190.f, (1.f - (i < 3 ? 3.f : 6.f) / 8.f) * height + height * 0.75f / 8.f));
		label->SetHorizontalAlignment(HorizontalAlignment::Center);
		label->UpdateSize(Vector2(380.f, 32.f));
		overlay->AddNode(label);
		meshLabels.push_back(label);
	}
	float height = static_cast<float>(app.GetEngine()->m_windowSystem->GetClientDimensions().y);
	auto image = CreateRef<Image>();
	Check(image->LoadFromRGBA8(IntVec2(2, 2), {
		255, 255, 255, 255, 40, 80, 180, 255,
		40, 80, 180, 255, 255, 255, 255, 255 }), "2D preview checker image");
	auto texture = CreateRef<TextureResource>();
	texture->m_image = image;
	Check(texture->InitGPUResources(), "2D preview checker texture");
	char const* titles[] = { "2D default", "2D nearest", "2D tint + linear", "3D standard" };
	for (int index = 0; index < 4; ++index)
	{
		auto* label = MemNew<Label>();
		label->SetText(titles[index]);
		materialLabels.push_back(label);
		label->SetPosition(Vector2(32.f + index * 220.f, height - 132.f));
		overlay->AddNode(label);
		if (index < 3)
		{
			auto* sample = MemNew<TextureRect>();
			sample->SetTexture(texture);
			sample->SetExpandMode(TextureRect::ExpandMode::IgnoreSize);
			sample->SetPosition(Vector2(32.f + index * 220.f, height - 100.f));
			sample->UpdateSize(Vector2(160.f, 80.f));
			if (index != 0)
			{
				auto material = CreateRef<StandardMaterial2D>();
				material->SetTextureFilter(index == 1 ? TextureFilter::Nearest : TextureFilter::Linear);
				material->SetTint(index == 1 ? Color::White : Color(1.f, 128.f / 255.f, 128.f / 255.f));
				sample->SetMaterial(material);
			}
			overlay->AddNode(sample);
			canvasSamples.push_back(sample);
		}
		else
		{
			auto material = CreateRef<StandardMaterial3D>();
			material->SetTint(Color(1.f, 180.f / 255.f, 60.f / 255.f));
			material->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
			auto* instance = MemNew<MeshInstance3D>();
			instance->SetMeshResource(mesh);
			instance->SetMaterialOverride(material);
			standardPreview = instance;
			instance->SetLocalScale(Vector3(0.5f, 0.5f, 0.5f));
			instance->SetLocalPosition(Vector3(772.f * 8.f / height, 60.f * 8.f / height, 0.f));
			root->AddNode(instance);
		}
	}

	auto sharedCanvasMaterial = CreateRef<StandardMaterial2D>();
	for (int index = 0; index < 2; ++index)
	{
		auto sampleImage = CreateRef<Image>();
		Check(sampleImage->LoadFromRGBA8(IntVec2(1, 1),
			index == 0 ? std::vector<uint8_t>{255, 0, 0, 255} : std::vector<uint8_t>{0, 255, 0, 255}),
			"shared Canvas material sample image");
		auto sampleTexture = CreateRef<TextureResource>();
		sampleTexture->m_image = sampleImage;
		Check(sampleTexture->InitGPUResources(), "shared Canvas material sample texture");
		auto* sample = MemNew<TextureRect>();
		sample->SetTexture(sampleTexture);
		sample->SetMaterial(sharedCanvasMaterial);
		sample->SetExpandMode(TextureRect::ExpandMode::IgnoreSize);
		sample->SetPosition(Vector2(32.f + index * 32.f, 32.f));
		sample->UpdateSize(Vector2(16.f, 16.f));
		overlay->AddNode(sample);
	}
}

void VerifyImage(App& app, std::filesystem::path const& directory, bool updated = false)
{
	using Microsoft::WRL::ComPtr;
	auto* viewport = static_cast<Viewport*>(app.GetEngine()->m_sceneSystem->GetSceneTree()->GetRoot());
	auto* texture = app.GetEngine()->m_renderServer->ViewportGetTexture(viewport->GetViewportRID());
	Check(texture != nullptr, "rendered viewport texture");
	ComPtr<ID3D11Resource> resource;
	texture->GetShaderResourceView()->GetResource(&resource);
	ComPtr<ID3D11Texture2D> source;
	Check(SUCCEEDED(resource.As(&source)), "viewport texture readback");
	ComPtr<ID3D11Device> device;
	source->GetDevice(&device);
	ComPtr<ID3D11DeviceContext> context;
	device->GetImmediateContext(&context);
	ComPtr<ID3D11Buffer> vertexConstants;
	ComPtr<ID3D11Buffer> pixelConstants;
	context->VSGetConstantBuffers(6, 1, vertexConstants.GetAddressOf());
	context->PSGetConstantBuffers(6, 1, pixelConstants.GetAddressOf());
	Check(!vertexConstants && !pixelConstants, "draws without material constants unbind b6 in both stages");
	D3D11_TEXTURE2D_DESC description;
	source->GetDesc(&description);
	description.Usage = D3D11_USAGE_STAGING;
	description.BindFlags = description.MiscFlags = 0;
	description.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	ComPtr<ID3D11Texture2D> staging;
	Check(SUCCEEDED(device->CreateTexture2D(&description, nullptr, &staging)), "create readback staging texture");
	context->CopyResource(staging.Get(), source.Get());
	D3D11_MAPPED_SUBRESOURCE mapped;
	Check(SUCCEEDED(context->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped)), "map rendered image");
	auto pixel = [&](float x, float y)
	{
		UINT column = static_cast<UINT>(x * description.Height / 8.f);
		UINT row = static_cast<UINT>((1.f - y / 8.f) * description.Height);
		auto* value = static_cast<unsigned char*>(mapped.pData) + row * mapped.RowPitch + column * 4;
		return Color(NormalizeByte(value[0]), NormalizeByte(value[1]), NormalizeByte(value[2]), NormalizeByte(value[3]));
	};
	Color canvasRed = pixel(40.f * 8.f / description.Height, 8.f - 40.f * 8.f / description.Height);
	Color canvasGreen = pixel(72.f * 8.f / description.Height, 8.f - 40.f * 8.f / description.Height);
	Color standard = pixel(772.f * 8.f / description.Height, 60.f * 8.f / description.Height);
	Color empty = pixel(2.f, 3.f);
	Color moved = pixel(5.f, 4.f);
	Color original = pixel(5.f, 3.f);
	Color red = pixel(8.f, 3.f);
	Color firstParameter = pixel(2.f, 6.f);
	Color secondParameter = pixel(5.f, 6.f);
	Color mixedParameter = pixel(8.25f, 6.5f);
	std::ofstream image(directory / "preview.ppm", std::ios::binary);
	image << "P6\n" << description.Width << ' ' << description.Height << "\n255\n";
	for (UINT y = 0; y < description.Height; ++y)
	{
		auto* row = static_cast<unsigned char*>(mapped.pData) + y * mapped.RowPitch;
		for (UINT x = 0; x < description.Width; ++x)
		{
			image.write(reinterpret_cast<char const*>(row + x * 4), 3);
		}
	}
	context->Unmap(staging.Get(), 0);
	Check(canvasRed.r > 240.f / 255.f && canvasRed.g < 5.f / 255.f && canvasGreen.g > 240.f / 255.f && canvasGreen.r < 5.f / 255.f,
		"Canvas commands sharing a material retain independent textures");
	Check(standard.r > 100.f / 255.f && standard.g > standard.b && standard.r > standard.g, "standard 3D tint reaches the built-in shader");
	Check(empty.r > 100.f / 255.f && empty.g > 100.f / 255.f && empty.b > 100.f / 255.f, "empty callbacks render default Lit white fallback");
	Check(moved.r > 100.f / 255.f && moved.g > 100.f / 255.f && original.r < 100.f / 255.f, "vertex deformation moves the engine-rendered mesh by one unit");
	Check(red.r > 100.f / 255.f && red.g < 5.f / 255.f && red.b < 5.f / 255.f, "fragment callback renders a lit red surface");
	Check(updated ? firstParameter.b > 100.f / 255.f && firstParameter.g < 5.f / 255.f : firstParameter.g > 100.f / 255.f && firstParameter.b < 5.f / 255.f, "default and runtime parameter values reach GPU");
	Check(secondParameter.r > 100.f / 255.f && secondParameter.g < 5.f / 255.f && secondParameter.b < 5.f / 255.f, "shared Shader keeps material parameters independent");
	Check(mixedParameter.b > 100.f / 255.f && mixedParameter.r < 5.f / 255.f && mixedParameter.g < 5.f / 255.f, "extended vertex and uniform colors reach both GPU stages without quantization or clamping");
}
}

int main(int argc, char** argv)
{
	try
	{
		MingRunConfig config;
		config.mode = MingRunMode::Game;
		config.projectPath = argc > 1 ? std::filesystem::absolute(argv[1]) : std::filesystem::absolute(argv[0]).parent_path().parent_path() / "Temporary" / "ShaderLifecycleVerification" / "Project";
		App app(config);
		app.Startup();
		Check(!app.IsQuitting(), "preview startup");
		bool preview = argc == 1 || (argc > 2 && std::string(argv[2]) == "--preview");
		glfwSetWindowTitle(app.GetEngine()->m_windowSystem->GetGLFWWindow(), "MShader Preview");
		if (!preview)
		{
			glfwHideWindow(app.GetEngine()->m_windowSystem->GetGLFWWindow());
		}
		Check(!ResourceLoader::Load("res://Bad.mshader").IsValid(), "HLSL compilation failure returns no resource");
		VerifyParsing();
		Populate(app);
		for (int frame = 0; frame < 4; ++frame)
		{
			app.RunFrame();
		}
		VerifyImage(app, config.projectPath.parent_path());
		snapshotMaterial->m_target = parameterMaterials[0];
		app.RunFrame();
		Check(parameterMaterials[0]->GetShaderParameter("rgb").As<Vector3>() == Vector3(0.f, 0.f, 1.f),
			"later request collection updates the earlier material");
		VerifyImage(app, config.projectPath.parent_path());
		for (int frame = 0; frame < 3; ++frame)
		{
			app.RunFrame();
		}
		VerifyImage(app, config.projectPath.parent_path(), true);
		std::printf("SHADER_ENGINE: PASS resource loading, mesh rendering and image readback\n");
		std::fflush(stdout);
		if (preview)
		{
			while (!app.IsQuitting())
			{
				UpdatePreviewLayout(app);
				app.RunFrame();
			}
		}
		snapshotMaterial = nullptr;
		parameterMaterials.clear();
		app.Shutdown();
		return 0;
	}
	catch (std::exception const& error)
	{
		std::fprintf(stderr, "SHADER_ENGINE: FAIL %s\n", error.what());
		return 1;
	}
}
