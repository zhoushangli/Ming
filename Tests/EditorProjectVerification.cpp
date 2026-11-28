#include "MingEngine/Scene/Resource/StandardMaterial2D.hpp"
#include "MingEngine/Scene/Resource/StandardMaterial3D.hpp"
#define NOMINMAX
#include "MingEngine/Engine/Application/App.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Application/ProjectSettings.hpp"
#include "MingEngine/Engine/Scene/SceneSystem.hpp"
#include "MingEngine/Engine/Script/ScriptSystem.hpp"
#include "MingEngine/Engine/Window/WindowSystem.hpp"
#include "MingEngine/Editor/EditorNode.hpp"
#include "MingEngine/Editor/UI/Controls/EditorSpinSlider.hpp"
#include "MingEngine/Editor/UI/EditorUI.hpp"
#include "MingEngine/Core/IO/ResourceImporter.hpp"
#include "MingEngine/Core/IO/ResourceLoader.hpp"
#include "MingEngine/Core/IO/ResourceSaver.hpp"
#include "MingEngine/Engine/Render/GPUTexture.hpp"
#include <d3d11.h>
#undef GetClassName
#include "MingEngine/Core/IO/ResourceFormatText.hpp"
#include "MingEngine/Editor/EditorChecker.hpp"
#include "MingEngine/Scene/3D/MeshInstance3D.hpp"
#include "MingEngine/Scene/Resource/SurfaceTool.hpp"
#include "MingEngine/Scene/Core/PackedScene.hpp"

#include "MingEngine/Core/IO/Image.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"
#include "MingEngine/Scene/Core/Viewport.hpp"
#include "MingEngine/Scene/GUI/Button.hpp"
#include "MingEngine/Scene/GUI/ColorPickerButton.hpp"
#include "MingEngine/Scene/GUI/SpinBox.hpp"
#include "MingEngine/Scene/GUI/SplitContainer.hpp"
#include "MingEngine/Scene/Resource/TextureResource.hpp"
#include "MingEngine/Scene/Resource/ShaderMaterial.hpp"
#include "MingEngine/Scene/GUI/TextureRect.hpp"
#include "MingEngine/Scene/GUI/SubViewportContainer.hpp"
#include "MingEngine/Scene/GUI/OptionButton.hpp"
#include "MingEngine/Scene/GUI/PanelContainer.hpp"
#include "MingEngine/Scene/GUI/Label.hpp"
#include "MingEngine/Editor/EditorThemeManager.hpp"
#include "MingEngine/Engine/File/FileSystem.hpp"
#include "MingEngine/Core/OS/WorkerThreadPool.hpp"
#include "ThirdParty/GLFW/glfw3.h"
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <functional>
#include <stdexcept>
#include <thread>
#include "MingEngine/Engine/Application/Main_Windows.cpp"

namespace
{
int checks = 0;
void Check(bool passed, char const* message)
{
	++checks;
	if (!passed)
	{
		std::fprintf(stderr, "FAILED: %s\n", message);
		throw std::runtime_error(message);
	}
}

void Write(std::filesystem::path const& path, std::string const& text)
{
	std::ofstream output(path);
	output << text;
	Check(output.good(), "write fixture");
}

void Image(std::filesystem::path const& path, uint8_t red, bool white = false)
{
	uint8_t bytes[21] = {};
	bytes[2] = 2;
	bytes[12] = 1;
	bytes[14] = 1;
	bytes[16] = 24;
	bytes[18] = white ? 255 : 0;
	bytes[19] = white ? 255 : 0;
	bytes[20] = red;
	std::ofstream output(path, std::ios::binary);
	output.write(reinterpret_cast<char const*>(bytes), sizeof(bytes));
}

void Wait(App& app, std::function<bool()> const& condition, char const* message)
{
	auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
	while (!condition() && std::chrono::steady_clock::now() < deadline)
	{
		app.RunFrame();
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	Check(condition(), message);
}

Button* FindButton(Node* node, String const& text)
{
	if (node == nullptr)
	{
		return nullptr;
	}
	if (auto* button = dynamic_cast<Button*>(node); button != nullptr && button->GetText() == text)
	{
		return button;
	}
	for (Node* child : node->GetChildren())
	{
		if (auto* found = FindButton(child, text))
		{
			return found;
		}
	}
	return nullptr;
}

void Choose(Viewport* viewport, String const& text)
{
	auto* button = FindButton(viewport->GetGuiPopup(), text);
	Check(button != nullptr, "dialog choice exists");
	button->GrabFocus();
	InputEvent event;
	event.m_type = InputEventType::MouseButton;
	event.m_mouseButton = GLFW_MOUSE_BUTTON_LEFT;
	event.m_position = button->GetSize() * 0.5f;
	event.m_pressed = true;
	button->GuiInput(event);
	event.m_pressed = false;
	button->GuiInput(event);
}

uint64_t Key(Node* node)
{
	auto const id = node->GetObjectID();
	return ((static_cast<uint64_t>(id.GetUID()) << 32) | id.GetIndex()) + 1;
}


LineEdit* FindTextField(Node* node)
{
	if (auto* field = dynamic_cast<LineEdit*>(node))
	{
		return field;
	}

	for (Node* child : node->GetChildren())
	{
		if (auto* field = FindTextField(child))
		{
			return field;
		}
	}

	return nullptr;
}

Node* FindNamed(Node* node, String const& name)
{
	if (node->GetName() == name)
	{
		return node;
	}

	for (Node* child : node->GetChildren())
	{
		if (auto* found = FindNamed(child, name))
		{
			return found;
		}
	}

	return nullptr;
}

void CaptureMaterialFrame(Engine& engine, std::string const& path)
{
	auto* viewport = static_cast<Viewport*>(engine.m_sceneSystem->GetSceneTree()->GetRoot());
	auto* texture  = engine.m_renderServer->ViewportGetTexture(viewport->GetViewportRID());
	Check(texture != nullptr, "rendered viewport texture exists");
	ID3D11Resource* resource = nullptr;
	texture->GetShaderResourceView()->GetResource(&resource);
	ID3D11Texture2D* source = nullptr;
	Check(
		SUCCEEDED(resource->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&source))),
		"readback texture");
	resource->Release();
	ID3D11Device* device = nullptr;
	source->GetDevice(&device);
	ID3D11DeviceContext* context = nullptr;
	device->GetImmediateContext(&context);
	D3D11_TEXTURE2D_DESC description;
	source->GetDesc(&description);
	description.Usage          = D3D11_USAGE_STAGING;
	description.BindFlags      = 0;
	description.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	description.MiscFlags      = 0;
	ID3D11Texture2D* staging   = nullptr;
	Check(SUCCEEDED(device->CreateTexture2D(&description, nullptr, &staging)), "create readback staging texture");
	context->CopyResource(staging, source);
	D3D11_MAPPED_SUBRESOURCE mapped;
	Check(SUCCEEDED(context->Map(staging, 0, D3D11_MAP_READ, 0, &mapped)), "map rendered pixels");

	std::ofstream output(path, std::ios::binary);
	output << "P6\n" << description.Width << ' ' << description.Height << "\n255\n";
	for (unsigned int y = 0; y < description.Height; ++y)
	{
		auto const* row = static_cast<unsigned char const*>(mapped.pData) + y * mapped.RowPitch;
		for (unsigned int x = 0; x < description.Width; ++x)
		{
			output.write(reinterpret_cast<char const*>(row + x * 4), 3);
		}
	}
	Check(output.good(), "write GPU frame evidence");
	context->Unmap(staging, 0);
	staging->Release();
	context->Release();
	device->Release();
	source->Release();
}

Color ReadCanvasPixel(App& app, Viewport* viewport, int x, int y)
{
	app.RunFrame();
	GPUTexture* texture = app.GetEngine()->m_renderServer->ViewportGetTexture(viewport->GetViewportRID());
	Check(texture != nullptr, "sampling viewport has a rendered texture");
	ID3D11Resource* resource = nullptr;
	texture->GetShaderResourceView()->GetResource(&resource);
	ID3D11Texture2D* source = nullptr;
	Check(SUCCEEDED(resource->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&source))), "sampling texture readback");
	resource->Release();
	ID3D11Device* device = nullptr;
	source->GetDevice(&device);
	ID3D11DeviceContext* context = nullptr;
	device->GetImmediateContext(&context);
	D3D11_TEXTURE2D_DESC description;
	source->GetDesc(&description);
	description.Usage = D3D11_USAGE_STAGING;
	description.BindFlags = 0;
	description.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	description.MiscFlags = 0;
	ID3D11Texture2D* staging = nullptr;
	Check(SUCCEEDED(device->CreateTexture2D(&description, nullptr, &staging)), "sampling staging texture");
	context->CopyResource(staging, source);
	D3D11_MAPPED_SUBRESOURCE mapped;
	Check(SUCCEEDED(context->Map(staging, 0, D3D11_MAP_READ, 0, &mapped)), "sampling pixels are readable");
	auto const* pixel = static_cast<unsigned char const*>(mapped.pData) + y * mapped.RowPitch + x * 4;
	Color result(NormalizeByte(pixel[0]), NormalizeByte(pixel[1]), NormalizeByte(pixel[2]), NormalizeByte(pixel[3]));
	context->Unmap(staging, 0);
	staging->Release();
	context->Release();
	device->Release();
	source->Release();
	return result;
}

void VerifyOptionButton(App& app, EditorNode* editor, std::filesystem::path const& evidence)
{
	auto* viewport = static_cast<Viewport*>(editor->GetSceneTree()->GetRoot());
	InputEvent event;
	event.m_type = InputEventType::WindowFocus;
	event.m_focused = true;
	viewport->ProcessInputEvent(event);
	auto* host = MemNew<Control>();
	host->SetMouseFilter(Control::MouseFilter::Ignore);
	viewport->AddNode(host);
	auto* option = MemNew<OptionButton>();
	option->SetClipText(true);
	option->SetOptions("Inherit:0,Nearest:7,Linear:12");
	option->SetValueNoSignal(7);
	option->SetPosition(Vector2(400.f, 180.f));
	option->UpdateSize(Vector2(220.f, 32.f));
	host->AddNode(option);
	int changes = 0;
	option->SetValueChangedCallback([&changes](int) { ++changes; });
	for (float scale : { 1.f, 1.25f })
	{
		option->SetTheme(EditorThemeManager::CreateScaledTheme(EditorThemeManager::CreateTheme(), scale));
		option->OpenOptions();
		auto* popup = dynamic_cast<PopupPanel*>(viewport->GetGuiPopup());
		Check(popup != nullptr, "enum opens a native popup");
		auto* nearest = FindButton(popup, "Nearest");
		auto* linear = FindButton(popup, "Linear");
		Check(nearest != nullptr && linear != nullptr && nearest->IsPressed(), "enum highlights the current item on open");
		auto* mark = dynamic_cast<PanelContainer*>(FindNamed(nearest, "OptionSelection"));
		Check(mark != nullptr, "enum selection has a separate radio marker");
		auto selectedStyle = mark->GetThemeStyleBox("panel");
		event.m_type = InputEventType::Key;
		event.m_pressed = true;
		event.m_keyCode = GLFW_KEY_DOWN;
		viewport->ProcessInputEvent(event);
		Check(option->GetValue() == 7 && changes == 0 && linear->IsPressed()
			&& mark->GetThemeStyleBox("panel") == selectedStyle, "keyboard navigation moves highlight without changing the checked value");
		event.m_keyCode = GLFW_KEY_ESCAPE;
		viewport->ProcessInputEvent(event);
		Check(viewport->GetGuiPopup() == nullptr && option->HasFocus() && option->GetValue() == 7,
			"escape cancels enum navigation and restores focus");
	}
	option->OpenOptions();
	auto* popup = dynamic_cast<PopupPanel*>(viewport->GetGuiPopup());
	auto* linear = FindButton(popup, "Linear");
	event.m_type = InputEventType::MouseMotion;
	event.m_position = linear->GetGlobalPosition() + linear->GetSize() * 0.5f;
	viewport->ProcessInputEvent(event);
	Check(linear->IsPressed() && option->GetValue() == 7, "mouse hover updates enum highlight without committing");
	app.RunFrame();
	CaptureMaterialFrame(*app.GetEngine(), (evidence / "enum-popup.ppm").string());
	event.m_type = InputEventType::Key;
	event.m_keyCode = GLFW_KEY_ENTER;
	viewport->ProcessInputEvent(event);
	Check(option->GetValue() == 12 && changes == 1 && viewport->GetGuiPopup() == nullptr,
		"enter commits the hovered sparse enum value exactly once");
	option->OpenOptions();
	Choose(viewport, "Inherit");
	Check(option->GetValue() == 0 && changes == 2, "mouse click commits an enum item");
	std::string many;
	for (int i = 0; i < 24; ++i)
	{
		if (!many.empty())
		{
			many += ',';
		}
		many += "Long texture filter option " + std::to_string(i) + ':' + std::to_string(i);
	}
	option->SetOptions(many);
	Vector2 const bounds(viewport->GetOutputResolution());
	option->SetPosition(bounds - Vector2(120.f, 40.f));
	option->OpenOptions();
	popup = dynamic_cast<PopupPanel*>(viewport->GetGuiPopup());
	auto* panel = dynamic_cast<Control*>(FindNamed(popup, "OptionPopup"));
	Check(panel != nullptr && panel->GetSize().x >= option->GetSize().x
		&& panel->GetGlobalPosition().x + panel->GetSize().x <= bounds.x
		&& panel->GetGlobalPosition().y + panel->GetSize().y <= bounds.y, "enum popup fits the screen and expands to its option labels");
	event.m_type = InputEventType::Key;
	event.m_keyCode = GLFW_KEY_UP;
	viewport->ProcessInputEvent(event);
	event.m_keyCode = GLFW_KEY_ENTER;
	viewport->ProcessInputEvent(event);
	Check(option->GetValue() == 23, "long enum menus scroll to keyboard-selected items");
	host->QueueFree();
	app.RunFrame();

	auto* canvas = MemNew<Control>();
	editor->GetSceneTree()->GetScene()->AddNode(canvas);
	auto samplingMaterial = CreateRef<StandardMaterial2D>();
	Check(ResourceSaver::Save("res://SamplingInspector.tres", samplingMaterial), "sampling inspector fixture saves");
	canvas->SetMaterial(samplingMaterial);
	auto& inspector = editor->m_editorUI->GetPanel<InspectorPanel>();
	inspector.EditResource(samplingMaterial);
	app.RunFrame();
	app.RunFrame();
	auto* preview2D = dynamic_cast<TextureRect*>(FindNamed(&inspector, "MaterialPreview2D"));
	Check(preview2D != nullptr && preview2D->GetTexture().IsValid(), "2D inspector preview has a texture");
	auto* previewViewport = dynamic_cast<SubViewport*>(preview2D->GetParent());
	Check(previewViewport != nullptr, "2D inspector preview uses a canvas viewport");
	IntVec2 previewSize = previewViewport->GetOutputResolution();
	Color previewPixel = ReadCanvasPixel(app, previewViewport, previewSize.x / 2, previewSize.y / 2);
	Check(previewPixel.b > previewPixel.r + 10.f / 255.f, "2D inspector preview renders its material checker");
	for (auto const& name : { "texture_filter", "texture_repeat" })
	{
		Node* field = FindNamed(inspector.GetContentControl(), name);
		Check(field != nullptr, "material sampling properties retain serialized names");
		Node* labelArea = FindNamed(field, "PropertyLabelArea");
		auto* title = dynamic_cast<Label*>(labelArea->GetChildren().front());
		Check(title != nullptr && title->GetText() == String(std::string(name) == "texture_filter" ? "Filter" : "Repeat"),
			"texture group displays short property labels");
	}
	Node* filterField = FindNamed(inspector.GetContentControl(), "texture_filter");
	auto* filter = dynamic_cast<OptionButton*>(FindButton(filterField, "Linear"));
	Check(filter != nullptr, "texture filter uses the shared enum control");
	filter->OpenOptions();
	app.RunFrame();
	CaptureMaterialFrame(*app.GetEngine(), (evidence / "enum-inspector.ppm").string());
	filter->CloseOptions();
	canvas->QueueFree();
	app.RunFrame();
}

void VerifyFileRemovals(App& app, EditorNode* editor, std::filesystem::path const& project)
{
	FileSystem* files = app.GetEngine()->m_fileSystem;
	auto hasFile = [files](VirtualPath const& path)
	{
		std::function<bool(FileEntry const*)> find = [&](FileEntry const* entry)
		{
			if (entry->GetVirtualPath() == path)
			{
				return true;
			}
			for (auto const& child : entry->GetChildren())
			{
				if (find(child.get()))
				{
					return true;
				}
			}
			return false;
		};
		return find(files->GetResourceRootEntry());
	};
	Write(project / "ScanRename.tres", "[ming_resource type=\"ShaderMaterial\" format=1]\n[resource]\n");
	editor->CheckProject();
	Wait(app, [&]() { return hasFile(VirtualPath("res://ScanRename.tres")); }, "scanner discovers removal fixture");
	auto& output = editor->m_editorUI->GetPanel<OutputPanel>();
	size_t const messages = output.GetMessageCount();
	VirtualPath renamed;
	std::string error;
	Check(files->Rename(VirtualPath("res://ScanRename.tres"), "ScanRenamed.tres", renamed, error), "rename fixture resource");
	editor->CheckProject();
	Wait(app, [&]() { return hasFile(renamed) && !hasFile(VirtualPath("res://ScanRename.tres")); }, "scanner accepts renamed resource");
	Check(output.GetMessageCount() == messages, "normal rename does not log a missing project file");
	std::filesystem::create_directories(project / "Moved");
	VirtualPath moved;
	Check(files->Move(renamed, VirtualPath("res://Moved"), moved, error), "move fixture resource");
	editor->CheckProject();
	Wait(app, [&]() { return hasFile(moved) && !hasFile(renamed); }, "scanner accepts moved resource");
	Check(output.GetMessageCount() == messages, "normal move does not log a missing project file");
	Check(files->Remove(moved, error), "delete fixture resource");
	editor->CheckProject();
	Wait(app, [&]() { return !hasFile(moved); }, "scanner removes deleted resource");
	Check(output.GetMessageCount() == messages, "normal deletion does not log a missing project file");
}

void VerifyCanvasSampling(App& app)
{
	auto image = CreateRef<::Image>();
	Check(image->LoadFromRGBA8(IntVec2(2, 1), { 255, 0, 0, 255, 0, 0, 255, 255 }), "sampling fixture image");
	Check(image->GetColorAt(0, 0) == Color::Red && image->GetColorAt(1, 0) == Color::Blue,
		"RGBA8 image pixels convert to normalized float Color");
	auto texture = CreateRef<TextureResource>();
	texture->m_image = image;
	Check(texture->InitGPUResources(), "sampling fixture texture upload");
	auto* viewport = MemNew<SubViewport>();
	viewport->UpdateResolution(IntVec2(64, 16));
	app.GetEngine()->m_sceneSystem->GetSceneTree()->GetRoot()->AddNode(viewport);
	auto* first = MemNew<Control>();
	auto parentMaterial = CreateRef<StandardMaterial2D>();
	parentMaterial->SetTextureFilter(TextureFilter::Nearest);
	parentMaterial->SetTint(Color::Green);
	first->SetMaterial(parentMaterial);
	viewport->AddNode(first);
	auto* nearest = MemNew<TextureRect>();
	nearest->SetTexture(texture);
	nearest->SetExpandMode(TextureRect::ExpandMode::IgnoreSize);
	nearest->UpdateSize(Vector2(32.f, 16.f));
	first->AddNode(nearest);
	Color pixel = ReadCanvasPixel(app, viewport, 12, 8);
	Check(pixel.r > 80.f / 255.f && pixel.b > 20.f / 255.f, "empty child material uses default linear sampling and does not inherit tint");
	auto material = CreateRef<StandardMaterial2D>();
	material->SetTextureFilter(TextureFilter::Nearest);
	nearest->SetMaterial(material);
	pixel = ReadCanvasPixel(app, viewport, 12, 8);
	Check(pixel.r > 240.f / 255.f && pixel.b < 10.f / 255.f, "assigned material controls nearest sampling");
	material->SetTextureFilter(TextureFilter::Linear);
	pixel = ReadCanvasPixel(app, viewport, 12, 8);
	Check(pixel.r > 80.f / 255.f && pixel.b > 20.f / 255.f, "material edits affect existing draw commands");
	material->SetTextureFilter(TextureFilter::Nearest);
	nearest->SetStretchMode(TextureRect::StretchMode::Tile);
	material->SetTextureRepeat(TextureRepeat::Enabled);
	pixel = ReadCanvasPixel(app, viewport, 4, 8);
	Check(pixel.r > 240.f / 255.f && pixel.b < 10.f / 255.f, "material repeat samples tiled UVs");
	material->SetTextureRepeat(TextureRepeat::Disabled);
	pixel = ReadCanvasPixel(app, viewport, 4, 8);
	Check(pixel.b > 240.f / 255.f && pixel.r < 10.f / 255.f, "disabled repeat clamps tiled UVs");
	material->SetTint(Color(1.f, 1.f, 128.f / 255.f, 1.f));
	pixel = ReadCanvasPixel(app, viewport, 4, 8);
	Check(pixel.b > 115.f / 255.f && pixel.b < 140.f / 255.f, "material tint multiplies command texture color");
	nearest->SetMaterial(CreateRef<StandardMaterial3D>());
	Check(nearest->GetMaterial() == material, "2D rejects a 3D material without changing the assignment");
	auto* meshInstance = MemNew<MeshInstance3D>();
	meshInstance->SetMaterialOverride(material);
	Check(!meshInstance->GetMaterialOverride().IsValid(), "3D rejects a 2D material");
	MemDelete(meshInstance);
	nearest->SetVisible(false);
	auto* embedded = MemNew<SubViewportContainer>();
	embedded->UpdateSize(Vector2(32.f, 16.f));
	first->AddNode(embedded);
	auto* source = MemNew<TextureRect>();
	source->SetTexture(texture);
	source->UpdateSize(Vector2(2.f, 1.f));
	embedded->GetSubViewport()->AddNode(source);
	app.RunFrame();
	embedded->GetSubViewport()->UpdateResolution(IntVec2(2, 1));
	auto viewportMaterial = CreateRef<StandardMaterial2D>();
	viewportMaterial->SetTextureFilter(TextureFilter::Nearest);
	embedded->SetMaterial(viewportMaterial);
	pixel = ReadCanvasPixel(app, viewport, 12, 8);
	Check(pixel.r > 240.f / 255.f && pixel.b < 10.f / 255.f, "viewport texture uses its node material sampling");
	viewportMaterial->SetTextureFilter(TextureFilter::Linear);
	pixel = ReadCanvasPixel(app, viewport, 12, 8);
	Check(pixel.r > 80.f / 255.f && pixel.b > 20.f / 255.f, "viewport texture supports linear sampling");
	viewport->QueueFree();
	app.RunFrame();
}

void VerifyTextResources(App& app, std::filesystem::path const& project, EditorNode* editor)
{
	std::filesystem::create_directories(project / "TextResources");
	VirtualPath const materialPath("res://TextResources/brick.tres");
	Ref<StandardMaterial3D> original = CreateRef<StandardMaterial3D>();
	original->SetName("Brick\n\"quoted\"\\path");
	Color const extended(-0.25f, 1.5f, 0.12345f, 0.87654f);
	original->SetTint(extended);
	Check((extended * 2.f).g == 3.f && (extended - 0.5f).r == -0.75f,
		"Color arithmetic retains extended floating point values");
	Check(DenormalizeByte(-0.25f) == 0 && DenormalizeByte(1.5f) == 255 && DenormalizeByte(0.5f) == 128,
		"byte conversion clamps and rounds only at the boundary");
	Image(project / "TextResources" / "white.tga", 255, true);
	Check(ResourceImporter::EnsureImported("res://TextResources/white.tga"), "resource fixture imports its own image");
	Ref<TextureResource> texture(ResourceLoader::Load("res://TextResources/white.tga"));
	Check(texture.IsValid(), "text fixture loads its own texture dependency");
	original->SetDiffuseTexture(texture);
	original->SetNormalTexture(texture);
	Check(ResourceSaver::Save(materialPath, original), "generic saver writes material");
	Ref<StandardMaterial3D> material(ResourceLoader::Load(materialPath));
	Check(material.IsValid() && material->GetName() == original->GetName(), "name and escapes roundtrip");
	Check(material->GetTint() == original->GetTint(), "float RGBA roundtrip");
	Check(material->GetDiffuseTexture() == texture && material->GetNormalTexture() == texture, "external references share cached identity");
	Check(!material->HasMethod("SetShaderResource"), "standard material hides shader replacement");
	Check(material->GetVirtualPath() == materialPath, "loader supplies resource path");

	Ref<ShaderMaterial> shaderMaterial = CreateRef<ShaderMaterial>();
	shaderMaterial->SetBlendMode(BlendMode::ALPHA);
	Check(ResourceSaver::Save("res://TextResources/shader.tres", shaderMaterial), "ShaderMaterial saves through the text resource writer");
	Ref<ShaderMaterial> loadedShader(ResourceLoader::LoadUncached("res://TextResources/shader.tres"));
	Check(loadedShader.IsValid() && !loadedShader->GetShaderResource().IsValid() && loadedShader->GetBlendMode() == BlendMode::ALPHA,
		"ShaderMaterial preserves its concrete type and draw state");

	std::string text;
	std::string error;
	Check(ResourceFormatSaverText::Write(material, text, error), "material encodes without IO");
	auto rejects = [&](std::string const& contents)
	{
		error.clear();
		return !ResourceFormatLoaderText::Parse(contents, materialPath, error).IsValid() && error.find("brick.tres:") != std::string::npos;
	};
	Check(rejects("[ming_resource type=\"MissingClass\" format=1]\n[resource]\n"), "unknown resource class reports location");
	Check(rejects("[ming_resource type=\"StandardMaterial3D\" format=2]\n[resource]\n"), "future format is rejected");
	Check(rejects("[ming_resource type=\"StandardMaterial3D\" format=1]\n[resource]\ntint = [1,2,3,4]\n"), "plain arrays are not typed colors");
	Check(rejects("[ming_resource type=\"StandardMaterial3D\" format=1]\n[resource]\ntint = Color(1,0,0)\n"), "wrong color component count is rejected");
	Check(rejects("[ming_resource type=\"StandardMaterial3D\" format=1]\n[resource]\nmissing = 1\n"), "unknown property is rejected");
	Check(rejects("[ming_resource type=\"StandardMaterial3D\" format=1]\n[resource]\nblend_mode = 999\n"), "setter rejected values fail loading");
	Check(rejects("[ming_resource type=\"StandardMaterial3D\" format=1]\n[resource]\nblend_mode = 4294967296\n"), "int overflow is rejected");

	Ref<ShaderMaterial> anonymous = CreateRef<ShaderMaterial>();
	anonymous->SetShaderResource(app.GetEngine()->m_renderServer->GetDefaultLitShaderResource());
	Check(!ResourceFormatSaverText::Write(anonymous, text, error), "unsaved resource dependencies are rejected");
	Write(project / "TextResources" / "cycle.tres", "[ming_resource type=\"StandardMaterial3D\" format=1]\n[ext_resource type=\"StandardMaterial3D\" path=\"res://TextResources/cycle.tres\" id=\"1\"]\n[resource]\n");
	Check(!ResourceLoader::Load("res://TextResources/cycle.tres").IsValid(), "recursive dependencies fail without recursion overflow");

	Ref<SurfaceTool> surface = CreateRef<SurfaceTool>();
	surface->AddVertex(Vector3(0.f, 0.f, 0.f));
	surface->AddVertex(Vector3(0.f, 1.f, 0.f));
	surface->AddVertex(Vector3(0.f, 0.f, 1.f));
	surface->Index();
	Ref<MeshResource> mesh = surface->CreateMeshResource();
	Check(ResourceSaver::Save("res://TextResources/triangle.mesh", mesh), "specialized mesh saver remains available");
	Check(!ResourceFormatSaverText::Write(mesh, text, error), "text writer refuses incomplete mesh serialization");
	Ref<StandardMaterial3D> other = CreateRef<StandardMaterial3D>();
	other->SetTint(Color::Blue);
	Check(ResourceSaver::Save("res://TextResources/blue.tres", other), "second independent material saves");
	Node* root = MemNew<Node>();
	root->SetName("TextRoot");
	auto* first = MemNew<MeshInstance3D>();
	first->SetName("First");
	first->SetMeshResource(mesh);
	first->SetMaterialOverride(material);
	root->AddNode(first);
	auto* second = MemNew<MeshInstance3D>();
	second->SetName("Second");
	second->SetMeshResource(mesh);
	second->SetMaterialOverride(other);
	root->AddNode(second);
	auto* canvas = MemNew<Control>();
	canvas->SetName("Canvas");
	auto canvasMaterial = CreateRef<StandardMaterial2D>();
	canvasMaterial->SetTextureFilter(TextureFilter::Nearest);
	canvasMaterial->SetTextureRepeat(TextureRepeat::Enabled);
	Check(ResourceSaver::Save("res://TextResources/canvas.tres", canvasMaterial), "2D material saves");
	canvas->SetMaterial(canvasMaterial);
	root->AddNode(canvas);
	Ref<PackedScene> packed = CreateRef<PackedScene>();
	Check(packed->Pack(root) && ResourceSaver::Save("res://TextResources/shared.tscn", packed), "scene text saves node material references");
	MemDelete(root);
	Ref<PackedScene> loaded(ResourceLoader::LoadUncached("res://TextResources/shared.tscn"));
	Check(loaded.IsValid(), "new scene text loads");
	root = loaded->Instantiate();
	Check(root != nullptr, "new scene instantiates");
	first = dynamic_cast<MeshInstance3D*>(root->FindChildByName("First"));
	second = dynamic_cast<MeshInstance3D*>(root->FindChildByName("Second"));
	Check(first != nullptr && second != nullptr && first->GetMeshResource() == second->GetMeshResource(), "scene restores shared mesh identity");
	Check(first->GetMaterialOverride() == material && first->GetMaterialOverride() != second->GetMaterialOverride(), "per node material overrides persist independently");
	Check(!first->GetMeshResource()->GetMaterial().IsValid(), "overrides do not mutate mesh default");
	canvas = dynamic_cast<Control*>(root->FindChildByName("Canvas"));
	Check(canvas != nullptr && Ref<StandardMaterial2D>(canvas->GetMaterial()).IsValid()
		&& Ref<StandardMaterial2D>(canvas->GetMaterial())->GetTextureFilter() == TextureFilter::Nearest
		&& Ref<StandardMaterial2D>(canvas->GetMaterial())->GetTextureRepeat() == TextureRepeat::Enabled, "CanvasItem material reference and sampling survive scene serialization");
	MemDelete(root);

	auto& inspector = editor->m_editorUI->GetPanel<InspectorPanel>();
	inspector.EditResource(material);
	app.RunFrame();
	app.RunFrame();
	auto* saveResource = dynamic_cast<Button*>(FindNamed(&inspector, "SaveResource"));
	auto* resourceOptions = dynamic_cast<Button*>(FindNamed(&inspector, "ResourceOptions"));
	auto* resourceBack = dynamic_cast<Button*>(FindNamed(&inspector, "ResourceBack"));
	auto* resourceForward = dynamic_cast<Button*>(FindNamed(&inspector, "ResourceForward"));
	Check(resourceForward != nullptr && resourceForward->GetIcon().IsValid() && resourceForward->IsDisabled(),
		"resource forward action starts disabled without forward history");
	Check(saveResource != nullptr && resourceOptions != nullptr && resourceBack != nullptr
		&& saveResource->GetText().IsEmpty() && resourceOptions->GetText().IsEmpty() && resourceBack->GetText().IsEmpty()
		&& saveResource->GetIcon().IsValid() && resourceOptions->GetIcon().IsValid() && resourceBack->GetIcon().IsValid(),
		"resource toolbar actions display icons without redundant labels");
	Check(resourceBack->GetGlobalPosition().x > resourceOptions->GetGlobalPosition().x + resourceOptions->GetSize().x
		&& resourceBack->GetGlobalPosition().x + resourceBack->GetSize().x
		<= inspector.GetGlobalPosition().x + inspector.GetSize().x,
		"resource back navigation is separated on the right inside the inspector");
	for (Button* action : { saveResource, resourceOptions, resourceBack, resourceForward })
	{
		Check(!action->GetTooltipText().IsEmpty()
			&& action->GetThemeConstant("icon_size") < editor->m_editorUI->GetRootControl()->GetThemeConstant("icon_size", "Button")
			&& action->GetThemeConstant("minimum_height") < editor->m_editorUI->GetRootControl()->GetThemeConstant("minimum_height", "EditorPanelAction"),
			"inspector actions have tooltips and compact theme dimensions");
	}
	Check(inspector.GetThemeIcon("StandardMaterial2D", "EditorIcons").IsValid()
		&& inspector.GetThemeIcon("StandardMaterial3D", "EditorIcons").IsValid()
		&& inspector.GetThemeIcon("StandardMaterial2D", "EditorIcons") != inspector.GetThemeIcon("StandardMaterial3D", "EditorIcons"),
		"standard material types resolve distinct theme icons");
	InputEvent optionsClick;
	optionsClick.m_type = InputEventType::MouseButton;
	optionsClick.m_mouseButton = GLFW_MOUSE_BUTTON_LEFT;
	optionsClick.m_position = resourceOptions->GetSize() * 0.5f;
	optionsClick.m_pressed = true;
	resourceOptions->GuiInput(optionsClick);
	optionsClick.m_pressed = false;
	resourceOptions->GuiInput(optionsClick);
	app.RunFrame();
	auto* resourceViewport = static_cast<Viewport*>(editor->GetSceneTree()->GetRoot());
	Check(FindButton(resourceViewport->GetGuiPopup(), "Save As...") != nullptr, "resource options expose Save As");
	Choose(resourceViewport, "Save As...");
	app.RunFrame();
	Choose(resourceViewport, "Cancel");
	app.RunFrame();
	inspector.EditResource(canvasMaterial);
	app.RunFrame();
	inspector.GoBack();
	app.RunFrame();
	Check(inspector.GetUndoHistory() == material->GetObjectID() && !resourceForward->IsDisabled(),
		"back restores the previous resource and enables forward");
	inspector.GoForward();
	app.RunFrame();
	Check(inspector.GetUndoHistory() == canvasMaterial->GetObjectID() && resourceForward->IsDisabled(),
		"forward restores the resource and consumes forward history");
	inspector.GoBack();
	app.RunFrame();
	inspector.EditResource(canvasMaterial);
	app.RunFrame();
	Check(resourceForward->IsDisabled(), "opening a resource after back clears the forward branch");
	inspector.GoBack();
	app.RunFrame();

	InputEvent tooltipFocus;
	tooltipFocus.m_type = InputEventType::WindowFocus;
	tooltipFocus.m_focused = true;
	resourceViewport->ProcessInputEvent(tooltipFocus);
	resourceViewport->SetGuiHovered(saveResource);
	Wait(app, [editor]()
	{
		auto* tooltip = dynamic_cast<Control*>(FindNamed(editor->m_editorUI->GetRootControl(), "EditorTooltip"));
		return tooltip != nullptr && tooltip->IsVisible();
	}, "hovering a resource action displays its tooltip after the delay");
	CaptureMaterialFrame(*app.GetEngine(), (project.parent_path() / "resource-tooltip.ppm").string());
	resourceViewport->SetGuiHovered(nullptr);
	app.RunFrame();
	Check(!static_cast<Control*>(FindNamed(editor->m_editorUI->GetRootControl(), "EditorTooltip"))->IsVisible(),
		"leaving the resource action hides the tooltip");
	Control* depthField = dynamic_cast<Control*>(FindNamed(inspector.GetContentControl(), "depth_mode"));
	Check(depthField != nullptr && depthField->GetGlobalPosition().x + depthField->GetSize().x
		<= inspector.GetGlobalPosition().x + inspector.GetSize().x,
		"long material enum labels do not push inspector fields beyond the panel");
	CaptureMaterialFrame(*app.GetEngine(), (project.parent_path() / "material-preview.ppm").string());
	Node* nameControl = FindNamed(inspector.GetContentControl(), "name");
	Check(nameControl != nullptr, "resource properties appear in the real inspector");
	LineEdit* name = FindTextField(nameControl);
	Check(name != nullptr, "resource string editor exists");
	InputEvent focus;
	focus.m_type = InputEventType::WindowFocus;
	focus.m_focused = true;
	static_cast<Viewport*>(editor->GetSceneTree()->GetRoot())->ProcessInputEvent(focus);
	name->GrabFocus();
	name->SelectAll();
	for (char character : std::string("Edited Brick"))
	{
		InputEvent input;
		input.m_type = InputEventType::Text;
		input.m_codepoint = character;
		name->GuiInput(input);
	}
	name->EndEditing(LineEdit::EditEndReason::Submitted);
	Check(material->GetName() == String("Edited Brick") && editor->GetChecker()->IsResourceDirty(materialPath), "inspector marks edited resource dirty");
	Check(!editor->IsSceneDirty(), "resource edit does not dirty scene");
	Check(editor->GetUndoRedo().Undo(material->GetObjectID()), "resource has independent undo history");
	Check(material->GetName() == original->GetName(), "resource undo restores original value");
	Check(editor->GetUndoRedo().Redo(material->GetObjectID()), "resource redo succeeds");
	Check(inspector.SaveResource() && !editor->GetChecker()->IsResourceDirty(materialPath), "saving resource clears dirty after success");
	inspector.GoBack();
	app.RunFrame();

	material->SetTint(Color::Green);
	editor->MarkResourceDirty(material.Get());
	std::filesystem::remove(project / "TextResources" / "brick.tres");
	Check(!editor->GetChecker()->SaveDirtyResources() && editor->GetChecker()->IsResourceDirty(materialPath), "deleted resource is not recreated and stays dirty");
	if (auto* viewport = static_cast<Viewport*>(editor->GetSceneTree()->GetRoot()); viewport->GetGuiPopup() != nullptr)
	{
		Choose(viewport, "OK");
	}
	Check(ResourceSaver::Save(materialPath, material), "explicit resource write restores fixture");


	VirtualPath createdPath;
	editor->m_editorUI->CreateResource(VirtualPath("res://TextResources"), [&createdPath](VirtualPath const& path) { createdPath = path; });
	auto* viewport = static_cast<Viewport*>(editor->GetSceneTree()->GetRoot());
	Check(FindNamed(viewport->GetGuiPopup(), "ResourceTypes") != nullptr, "resource creation opens the type picker");
	app.RunFrame();
	CaptureMaterialFrame(*app.GetEngine(), (project.parent_path() / "resource-picker.ppm").string());
	Choose(viewport, "Create");
	app.RunFrame();
	auto* filename = dynamic_cast<LineEdit*>(FindNamed(viewport->GetGuiPopup(), "SceneFilename"));
	Check(filename != nullptr, "new material uses the existing file dialog");
	filename->SetText("Created.tres");
	Choose(viewport, "Save");
	Check(createdPath == VirtualPath("res://TextResources/Created.tres") && ResourceLoader::Load(createdPath).IsValid(), "material creation saves an independent file");
	Check(Ref<ShaderMaterial>(ResourceLoader::Load(createdPath)).IsValid(), "new resources use the ShaderMaterial concrete type");
	Check(editor->m_editorUI->GetPanel<FileSystemPanel>().GetSelectedVirtualPath() == createdPath,
		"new resource is selected in the filesystem");
	Wait(app, [editor]() { return !editor->IsCheckingProject(); }, "creation scan finishes before the next picker");
	for (uint64_t type : { 2u, 3u })
	{
		VirtualPath standardPath;
		editor->m_editorUI->CreateResource(VirtualPath("res://TextResources"), [&standardPath](VirtualPath const& path) { standardPath = path; });
		app.RunFrame();
		std::function<Tree*(Node*)> findTree = [&](Node* node) -> Tree*
		{
			if (auto* tree = dynamic_cast<Tree*>(node))
			{
				return tree;
			}
			for (Node* child : node->GetChildren())
			{
				if (Tree* tree = findTree(child))
				{
					return tree;
				}
			}
			return nullptr;
		};
		auto* picker = findTree(viewport->GetGuiPopup());
		Check(picker != nullptr, "standard material type picker exists");
		picker->Select(type);
		app.RunFrame();
		Choose(viewport, "Create");
		app.RunFrame();
		filename = dynamic_cast<LineEdit*>(FindNamed(viewport->GetGuiPopup(), "SceneFilename"));
		Check(filename != nullptr, "standard material opens save dialog");
		filename->SetText(type == 2 ? "Created2D.tres" : "Created3D.tres");
		Choose(viewport, "Save");
		Ref<Resource> standard = ResourceLoader::Load(standardPath);
		Check(standard.IsValid() && standard->GetClassName() == (type == 2 ? "StandardMaterial2D" : "StandardMaterial3D"),
			"resource picker creates the selected concrete standard material type");
	}
	VirtualPath copiedPath;
	editor->m_editorUI->SaveMaterialAs(material, [&copiedPath](VirtualPath const& path) { copiedPath = path; });
	app.RunFrame();
	filename = dynamic_cast<LineEdit*>(FindNamed(viewport->GetGuiPopup(), "SceneFilename"));
	Check(filename != nullptr, "material copy opens save as dialog");
	filename->SetText("Unique.tres");
	Choose(viewport, "Save");
	Ref<StandardMaterial3D> copy(ResourceLoader::Load(copiedPath));
	Check(copy.IsValid() && copy != material && copy->GetTint() == material->GetTint(), "save as creates an independent material with matching parameters");
	Check(copy->GetDiffuseTexture() == material->GetDiffuseTexture(), "independent material keeps shared textures");
	copy->SetTint(Color::Blue);
	Check(material->GetTint() == Color::Green, "editing copied material leaves original unchanged");

	VirtualPath shaderCopyPath;
	editor->m_editorUI->SaveMaterialAs(loadedShader, [&shaderCopyPath](VirtualPath const& path) { shaderCopyPath = path; });
	app.RunFrame();
	filename = dynamic_cast<LineEdit*>(FindNamed(viewport->GetGuiPopup(), "SceneFilename"));
	Check(filename != nullptr, "ShaderMaterial copy opens save as dialog");
	filename->SetText("ShaderCopy.tres");
	Choose(viewport, "Save");
	Ref<ShaderMaterial> shaderCopy(ResourceLoader::LoadUncached(shaderCopyPath));
	Check(shaderCopy.IsValid() && shaderCopy->GetShaderResource() == loadedShader->GetShaderResource()
		&& shaderCopy->GetBlendMode() == loadedShader->GetBlendMode(),
		"save as preserves ShaderMaterial type and state");
	inspector.EditResource(shaderCopy);
	app.RunFrame();
	app.RunFrame();
	auto const captureDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
	while (editor->IsCheckingProject() && std::chrono::steady_clock::now() < captureDeadline)
	{
		app.RunFrame();
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
	Check(!editor->IsCheckingProject(), "resource import finishes before capturing the material inspector");
	CaptureMaterialFrame(*app.GetEngine(), (project.parent_path() / "shader-material.ppm").string());

	auto* instance = MemNew<MeshInstance3D>();
	instance->SetName("MaterialUser");
	instance->SetMeshResource(mesh);
	editor->GetSceneTree()->GetScene()->AddNode(instance);
	editor->GetSelection().SetSelected(instance->GetObjectID());
	app.RunFrame();
	auto* materialField = dynamic_cast<Control*>(FindNamed(inspector.GetContentControl(), "material_override"));
	Check(materialField != nullptr, "node exposes material override in inspector");
	GuiDragData drag{ "file", ObjectID::Invalid, materialPath.GetString() };
	Check(materialField->CanDropData(Vector2::Zero, drag), "material field accepts compatible file drag");
	materialField->DropData(Vector2::Zero, drag);
	Check(instance->GetMaterialOverride() == material && editor->IsSceneDirty(), "dropping material assigns override and dirties scene");
	drag.m_value = "res://TextResources/white.tga";
	Check(!materialField->CanDropData(Vector2::Zero, drag), "material field rejects texture drop");
	Check(editor->SaveSceneAs("res://TextResources/assigned.tscn"), "assigned material scene saves");
	Check(editor->LoadScene("res://main.tscn"), "scene replacement succeeds after resource editing");
	app.RunFrame();
	Check(editor->GetUndoRedo().HasUndo(material->GetObjectID()), "scene replacement preserves resource undo history");

	Ref<StandardMaterial3D> disk = CreateRef<StandardMaterial3D>();
	disk->SetTint(Color::Blue);
	Check(ResourceFormatSaverText::Write(disk, text, error), "external edit fixture encodes");
	material->SetTint(Color::Red);
	editor->MarkResourceDirty(material.Get());
	Write(project / "TextResources" / "brick.tres", text);
	editor->CheckProject();
	Wait(app, [viewport]() { return FindButton(viewport->GetGuiPopup(), "Reload from disk") != nullptr; }, "dirty resource external edit prompts");
	Check(material->GetTint() == Color::Red && editor->GetChecker()->IsResourceDirty(materialPath), "external scan preserves dirty memory before resolution");
	Choose(viewport, "Cancel");
	Check(material->GetTint() == Color::Red && editor->GetChecker()->IsResourceDirty(materialPath), "cancel preserves pending resource changes");
	editor->CheckProject();
	Wait(app, [viewport]() { return FindButton(viewport->GetGuiPopup(), "Reload from disk") != nullptr; }, "unresolved external resource can be checked again");
	Choose(viewport, "Reload from disk");
	Check(material->GetTint() == Color::Blue && !editor->GetChecker()->IsResourceDirty(materialPath), "reload replaces resource in place and clears dirty");
	material->SetTint(Color::Green);
	editor->MarkResourceDirty(material.Get());
	disk->SetTint(Color::Red);
	Check(ResourceFormatSaverText::Write(disk, text, error), "second external edit encodes");
	Write(project / "TextResources" / "brick.tres", text);
	editor->CheckProject();
	Wait(app, [viewport]() { return FindButton(viewport->GetGuiPopup(), "Reload from disk") != nullptr; }, "second dirty resource conflict prompts");
	Choose(viewport, "Keep and Save");
	Ref<StandardMaterial3D> saved(ResourceLoader::LoadUncached(materialPath));
	Check(saved.IsValid() && saved->GetTint() == Color::Green && !editor->GetChecker()->IsResourceDirty(materialPath), "keep and save writes memory and clears dirty");
}


char const* const parameterShaderSource = R"(shader_type spatial;
uniform float scalar = 1.0;
uniform float2 pair = float2(2.0, 3.0);
uniform float3 triple = float3(4.0, 5.0, 6.0);
uniform float4 quad = float4(0.2, 0.4, 0.8, 1.0);
void fragment(inout FragmentContext f) { f.color = quad; }
)";

void VerifyShaderParameters(App& app, std::filesystem::path const& project, EditorNode* editor)
{
	auto* native = MemNew<Node>();
	native->SetName("Before");
	Check(native->SetProperty("name", String("")) && native->GetName() == String("Node"),
		"native property writes retain setter normalization and getter readback");
	MemDelete(native);
	Write(project / "parameters.mshader", parameterShaderSource);
	auto shader = Ref<ShaderResource>(ResourceLoader::Load("res://parameters.mshader"));
	Check(shader.IsValid(), "parameter fixture loads through the shader loader");
	auto material = CreateRef<ShaderMaterial>();
	material->SetShaderResource(shader);
	auto shared = CreateRef<ShaderMaterial>();
	shared->SetShaderResource(shader);
	Check(material->GetProperty("shader_parameter/scalar") == Variant(1.f)
		&& material->GetPropertyState("shader_parameter/scalar").IsEmpty(), "unmodified uniforms read shader defaults without overrides");
	Check(material->SetProperty("shader_parameter/scalar", 1.f)
		&& material->HasShaderParameterOverride("scalar"), "setting the default explicitly retains an override");
	auto uniforms = shader->GetUniforms();
	uniforms[0].m_defaultValue = 2.f;
	shader->SetUniforms(uniforms);
	Check(material->GetShaderParameter("scalar") == Variant(1.f)
		&& shared->GetShaderParameter("scalar") == Variant(2.f), "default revisions affect only materials without overrides");
	Check(material->SetProperty("shader_parameter/scalar", Variant())
		&& material->GetShaderParameter("scalar") == Variant(2.f), "clearing override restores the current shader default");
	Check(!material->SetProperty("shader_parameter/scalar", Vector2(1.f, 2.f)), "dynamic writes reject incompatible types");
	Check(material->SetProperty("shader_parameter/quad", Vector4(0.3f, 0.5f, 0.7f, 1.f)), "float4 writes through the object property interface");
	std::string text;
	std::string error;
	Check(ResourceFormatSaverText::Write(material, text, error)
		&& text.find("shader_parameter/scalar") == std::string::npos
		&& text.find("shader_parameter/quad") != std::string::npos, "serialization contains only explicit uniform overrides");
	VirtualPath path("res://parameters.tres");
	Check(ResourceSaver::Save(path, material), "shader parameter resource saves");
	auto loaded = Ref<ShaderMaterial>(ResourceLoader::LoadUncached(path));
	Check(loaded.IsValid() && loaded->GetShaderParameter("quad") == material->GetShaderParameter("quad")
		&& !loaded->HasShaderParameterOverride("scalar"), "resource roundtrip preserves values and override absence");
	size_t begin = text.find("shader_parameter/quad =");
	size_t end = text.find('\n', begin);
	std::string assignment = text.substr(begin, end - begin + 1);
	text.erase(begin, end - begin + 1);
	text.insert(text.find("[resource]\n") + 11, assignment);
	loaded = Ref<ShaderMaterial>(ResourceFormatLoaderText::Parse(text, path, error));
	Check(loaded.IsValid() && loaded->GetShaderParameter("quad") == material->GetShaderParameter("quad"),
		"uniform assignments can precede the shader assignment");
	text += "shader_parameter/removed = 3.0\n";
	text.replace(text.find(assignment), assignment.size(), "shader_parameter/quad = Vector3(1.0,2.0,3.0)\n");
	loaded = Ref<ShaderMaterial>(ResourceFormatLoaderText::Parse(text, path, error));
	Check(loaded.IsValid() && !loaded->HasShaderParameterOverride("quad"), "removed and incompatible saved uniforms are skipped");
	Check(!ResourceFormatLoaderText::Parse(text + "shader_parameter/broken = Vector4(nope)\n", path, error).IsValid(),
		"malformed saved uniform values still fail loading");

	auto& inspector = editor->m_editorUI->GetPanel<InspectorPanel>();
	inspector.EditResource(material);
	app.RunFrame();
	app.RunFrame();
	std::vector<EditorSpinSlider*> spins;
	std::function<void(Node*)> collect = [&](Node* node)
	{
		if (auto* spin = dynamic_cast<EditorSpinSlider*>(node))
		{
			spins.push_back(spin);
		}
		for (auto* child : node->GetChildren())
		{
			collect(child);
		}
	};
	for (auto const& entry : std::vector<std::pair<std::string, size_t>>{ { "scalar", 1 }, { "pair", 2 }, { "triple", 3 }, { "quad", 4 } })
	{
		auto* field = FindNamed(&inspector, "shader_parameter/" + entry.first);
		Check(field != nullptr, "custom uniform appears in the real Inspector");
		spins.clear();
		collect(field);
		Check(spins.size() == entry.second, "uniform field has the correct number of numeric components");
	}
	for (int component = 0; component < 4; ++component)
	{
		InputEvent click;
		click.m_type = InputEventType::MouseButton;
		click.m_mouseButton = GLFW_MOUSE_BUTTON_LEFT;
		click.m_pressed = true;
		spins[component]->GuiInput(click);
		click.m_pressed = false;
		spins[component]->GuiInput(click);
		auto* field = spins[component]->GetLineEdit();
		Check(field->IsEditing(), "clicking a numeric component starts text editing");
		field->SelectAll();
		for (char character : std::string("0.9"))
		{
			InputEvent textInput;
			textInput.m_type = InputEventType::Text;
			textInput.m_codepoint = character;
			field->GuiInput(textInput);
		}
		field->EndEditing(LineEdit::EditEndReason::Submitted);
		Vector4 expected(0.3f, 0.5f, 0.7f, 1.f);
		(component == 0 ? expected.x : (component == 1 ? expected.y : (component == 2 ? expected.z : expected.w))) = 0.9f;
		Check(material->GetShaderParameter("quad") == Variant(expected), "float4 text editing updates only the selected component");
		Check(editor->GetUndoRedo().Undo(material->GetObjectID())
			&& material->GetShaderParameter("quad") == Variant(Vector4(0.3f, 0.5f, 0.7f, 1.f)), "float4 component editing is undoable");
		app.RunFrame();
	}
	Node* fixedField = FindNamed(&inspector, "blend_mode");
	auto* scalarField = FindNamed(&inspector, "shader_parameter/scalar");
	spins.clear();
	collect(scalarField);
	auto* spin = spins.front();
	InputEvent input;
	input.m_type = InputEventType::MouseButton;
	input.m_mouseButton = GLFW_MOUSE_BUTTON_LEFT;
	input.m_pressed = true;
	spin->GuiInput(input);
	input.m_type = InputEventType::MouseMotion;
	input.m_relative = Vector2(20.f, 0.f);
	spin->GuiInput(input);
	Check(material->GetShaderParameter("scalar") != Variant(2.f), "drag previews the material before release");
	input.m_type = InputEventType::MouseButton;
	input.m_pressed = false;
	spin->GuiInput(input);
	auto& history = editor->GetUndoRedo();
	Check(history.Undo(material->GetObjectID()) && !material->HasShaderParameterOverride("scalar"),
		"undoing a drag restores absence of an override");
	Check(history.Redo(material->GetObjectID()) && material->HasShaderParameterOverride("scalar"), "redo restores the drag override");
	app.RunFrame();
	auto* revert = dynamic_cast<Button*>(FindNamed(scalarField, "RevertProperty"));
	Check(revert != nullptr && revert->IsVisible(), "overridden parameter offers restore default");
	input.m_pressed = true;
	revert->GuiInput(input);
	input.m_pressed = false;
	revert->GuiInput(input);
	Check(!material->HasShaderParameterOverride("scalar") && material->GetShaderParameter("scalar") == Variant(2.f),
		"restore default removes the override");
	Check(history.Undo(material->GetObjectID()) && material->HasShaderParameterOverride("scalar"), "restore default is undoable");
	Check(history.Redo(material->GetObjectID()) && !material->HasShaderParameterOverride("scalar"), "restore default is redoable");

	material->SetShaderParameter("scalar", 2.f);
	app.RunFrame();
	Check(revert->IsVisible(), "restore default remains available for an explicit value equal to the default");
	input.m_pressed = true;
	revert->GuiInput(input);
	input.m_pressed = false;
	revert->GuiInput(input);
	Check(!material->HasShaderParameterOverride("scalar"), "restore default clears an equal-valued override");
	app.RunFrame();
	input.m_pressed = true;
	spin->GuiInput(input);
	input.m_type = InputEventType::MouseMotion;
	input.m_relative = Vector2(20.f, 0.f);
	spin->GuiInput(input);
	spin->EndEditing(LineEdit::EditEndReason::Cancelled);
	Check(!material->HasShaderParameterOverride("scalar"), "cancelling a drag restores absence of an override");

	input.m_type = InputEventType::MouseButton;
	input.m_pressed = true;
	spin->GuiInput(input);
	input.m_type = InputEventType::MouseMotion;
	input.m_relative = Vector2(20.f, 0.f);
	spin->GuiInput(input);
	shader->SetUniforms(uniforms);
	app.RunFrame();
	Check(!material->HasShaderParameterOverride("scalar"), "shader revisions cancel transient drag overrides");

	Write(project / "parameters-second.mshader", R"(shader_type spatial;
uniform float scalar = 2.0;
uniform float2 pair = float2(2.0, 3.0);
uniform float3 triple = float3(4.0, 5.0, 6.0);
uniform float3 quad = float3(1.0, 2.0, 3.0);
void fragment(inout FragmentContext f) { f.color = float4(quad, 1.0); }
)");
	auto replacement = Ref<ShaderResource>(ResourceLoader::Load("res://parameters-second.mshader"));
	Check(replacement.IsValid(), "replacement shader has compiled render data");
	Check(history.CreateAction("Replace shader", material->GetObjectID()), "shader replacement action starts");
	Check(history.AddDoPropertyState(material.Get(), "shader", Variant(replacement), {})
		&& history.AddUndoPropertyState(material.Get(), "shader", Variant(shader),
			{ { "shader_parameter/quad", material->GetShaderParameter("quad") } })
		&& history.CommitAction(), "shader replacement records dependent parameter state");
	app.RunFrame();
	Check(FindNamed(&inspector, "blend_mode") == fixedField && FindNamed(&inspector, "shader_parameter/quad") != nullptr,
		"shader revision rebuilds only dynamic fields");
	Check(history.Undo(material->GetObjectID()) && material->GetShaderResource() == shader
		&& material->GetShaderParameter("quad") == Variant(Vector4(0.3f, 0.5f, 0.7f, 1.f)),
		"undo shader replacement restores removed overrides");
	app.RunFrame();
	app.RunFrame();
	CaptureMaterialFrame(*app.GetEngine(), (project.parent_path() / "shader-parameters.ppm").string());
	material->SetVirtualPath(path);
	Check(inspector.SaveResource(), "parameter edits can be saved from Inspector");
}


char const* const hintShaderSource = R"(shader_type spatial;
group_uniforms Appearance;
uniform float opacity : hint_range(0.f, 1.f, 0.01f) = 0.8;
uniform float4 tint : hint_color = float4(0.12345, 0.5, 0.8, 1);
group_uniforms Effect;
uniform int mode : hint_enum("Normal", "Glow", "Outline") = 0;
uniform int count : hint_range(0, 16, 1) = 4;
group_uniforms;
uniform float strength = 1;
void fragment(inout FragmentContext f) { f.color = tint * opacity * strength; }
)";

template <typename T>
T* FindControl(Node* node)
{
	if (auto* control = dynamic_cast<T*>(node))
	{
		return control;
	}
	for (auto* child : node->GetChildren())
	{
		if (auto* control = FindControl<T>(child))
		{
			return control;
		}
	}
	return nullptr;
}

void VerifyShaderHints(App& app, std::filesystem::path const& directory, EditorNode* editor, Ref<ShaderMaterial> material)
{
	auto& inspector = editor->m_editorUI->GetPanel<InspectorPanel>();
	auto evidence = directory.parent_path() / "ShaderParameterEvidence";
	std::filesystem::create_directories(evidence);
	auto* viewport = static_cast<Viewport*>(editor->GetSceneTree()->GetRoot());
	InputEvent focus;
	focus.m_type = InputEventType::WindowFocus;
	focus.m_focused = true;
	viewport->ProcessInputEvent(focus);
	editor->m_editorUI->ResetNativeLayout();
	app.RunFrame();
	app.RunFrame();
	Check(FindNamed(&inspector, "InspectorGroup_Appearance") != nullptr
		&& FindNamed(&inspector, "InspectorGroup_Effect") != nullptr
		&& FindNamed(&inspector, "InspectorGroup_Shader Parameters") != nullptr, "shader groups and ungrouped parameters appear");
	auto* color = FindControl<ColorPickerButton>(FindNamed(&inspector, "shader_parameter/tint"));
	auto* option = FindControl<OptionButton>(FindNamed(&inspector, "shader_parameter/mode"));
	auto* range = FindControl<EditorSpinSlider>(FindNamed(&inspector, "shader_parameter/opacity"));
	Check(color != nullptr && option != nullptr && range != nullptr && option->GetOptions().size() == 3
		&& range->GetError().empty(), "hints select configured color, enum and range editors");
	Check(!material->SetShaderParameter("mode", 1.f) && !material->SetShaderParameter("tint", Vector4(1.f, 0.f, 0.f, 1.f)),
		"hinted parameters retain strict int and Color API types");
	CaptureMaterialFrame(*app.GetEngine(), (evidence / "hints-inspector.ppm").string());
	Color const original = material->GetShaderParameter("tint").As<Color>();
	Check(original.r == 0.12345f && color->GetColor() == original, "Inspector preserves non-byte float colors");
	color->OpenPicker();
	app.RunFrame();
	CaptureMaterialFrame(*app.GetEngine(), (evidence / "hints-color.ppm").string());
	Check(color->IsPickerOpen() && color->GetPicker()->GetColor() == original, "opening float picker preserves precision");
	auto* hexInput = dynamic_cast<LineEdit*>(FindNamed(color->GetPicker(), "HexColor"));
	Check(hexInput != nullptr && hexInput->GetText() == "#1F80CCFF" && color->GetColor() == original,
		"Hex display rounds a copy without quantizing the original");
	auto enterHex = [&](char const* text)
	{
		hexInput->GrabFocus();
		hexInput->SelectAll();
		for (char const* c = text; *c != '\0'; ++c)
		{
			InputEvent input;
			input.m_type = InputEventType::Text;
			input.m_codepoint = static_cast<unsigned char>(*c);
			hexInput->GuiInput(input);
		}
		hexInput->EndEditing(LineEdit::EditEndReason::Submitted);
	};
	enterHex("#1F80CCFF");
	Check(color->GetColor() == original, "submitting unchanged Hex text preserves float precision");
	color->ClosePicker(true);
	app.RunFrame();
	Check(material->GetShaderParameter("tint") == Variant(original) && !material->HasShaderParameterOverride("tint"), "opening and closing picker does not create an override");
	color->OpenPicker();
	app.RunFrame();
	enterHex("#80402080");
	Check(material->GetShaderParameter("tint") == Variant(Color(128.f / 255.f, 64.f / 255.f, 32.f / 255.f, 128.f / 255.f)),
		"submitting changed Hex text converts bytes to floating point channels");
	color->ClosePicker(false);
	app.RunFrame();
	Check(material->GetShaderParameter("tint") == Variant(original) && !material->HasShaderParameterOverride("tint"),
		"cancelling Hex editing restores exact value and override absence");
	color->OpenPicker();
	app.RunFrame();
	auto* green = dynamic_cast<SpinBox*>(FindNamed(color->GetPicker(), "Channel1"));
	Check(green != nullptr, "float picker exposes RGBA channels");
	green->Step(1);
	Color changed = material->GetShaderParameter("tint").As<Color>();
	Check(changed.r == original.r && changed.b == original.b && changed.a == original.a
		&& changed.g > original.g && changed.g < original.g + 0.002f, "float channel edit preserves untouched channels");
	color->ClosePicker(false);
	app.RunFrame();
	Check(material->GetShaderParameter("tint") == Variant(original) && !material->HasShaderParameterOverride("tint"), "cancel restores exact color and override absence");
	color->SetScreenColorReader([](Vector2 const&, Color& sample)
	{
		sample = Color(17.f / 255.f, 128.f / 255.f, 240.f / 255.f, 1.f);
		return true;
	});
	color->OpenPicker();
	color->StartScreenPicking();
	InputEvent sample;
	sample.m_type = InputEventType::MouseMotion;
	sample.m_position = Vector2(100.f, 100.f);
	viewport->ProcessInputEvent(sample);
	Check(material->GetShaderParameter("tint") == Variant(Color(17.f / 255.f, 128.f / 255.f, 240.f / 255.f, 1.f)), "screen sample converts byte source to float RGBA");
	sample.m_type = InputEventType::Key;
	sample.m_pressed = true;
	sample.m_keyCode = GLFW_KEY_ESCAPE;
	viewport->ProcessInputEvent(sample);
	Check(!color->IsPickingScreen() && material->GetShaderParameter("tint") == Variant(original), "screen sampling cancellation restores float precision");
	color->ClosePicker(false);
	app.RunFrame();
	color->OpenPicker();
	app.RunFrame();
	green->Step(1);
	color->ClosePicker(true);
	app.RunFrame();
	changed = material->GetShaderParameter("tint").As<Color>();
	auto& history = editor->GetUndoRedo();
	Check(history.Undo(material->GetObjectID()) && material->GetShaderParameter("tint") == Variant(original), "float color editing is undoable");
	Check(history.Redo(material->GetObjectID()) && material->GetShaderParameter("tint") == Variant(changed), "float color editing is redoable");
	app.RunFrame();
	auto* revert = dynamic_cast<Button*>(FindNamed(FindNamed(&inspector, "shader_parameter/tint"), "RevertProperty"));
	InputEvent click;
	click.m_type = InputEventType::MouseButton;
	click.m_mouseButton = GLFW_MOUSE_BUTTON_LEFT;
	click.m_pressed = true;
	revert->GuiInput(click);
	click.m_pressed = false;
	revert->GuiInput(click);
	Check(material->GetShaderParameter("tint") == Variant(original) && !material->HasShaderParameterOverride("tint"), "color restore-default clears override");
	option->OpenOptions();
	app.RunFrame();
	CaptureMaterialFrame(*app.GetEngine(), (evidence / "hints-enum.ppm").string());
	Choose(viewport, "Glow");
	Check(material->GetShaderParameter("mode") == Variant(1), "enum selection commits an integer");
	Check(history.Undo(material->GetObjectID()) && material->GetShaderParameter("mode") == Variant(0), "shader enum selection is undoable");
	Check(history.Redo(material->GetObjectID()) && material->GetShaderParameter("mode") == Variant(1), "shader enum selection is redoable");
	material->SetShaderParameter("tint", original);
	VirtualPath path("res://hints-roundtrip.tres");
	Check(ResourceSaver::Save(path, material), "hint parameters save");
	Ref<ShaderMaterial> loaded(ResourceLoader::LoadUncached(path));
	Check(loaded.IsValid() && loaded->GetShaderParameter("tint") == Variant(original)
		&& loaded->GetShaderParameter("mode") == Variant(1), "float colors and integer enums survive resource roundtrip");
	auto uniforms = material->GetShaderResource()->GetUniforms();
	uniforms[1].m_defaultValue = Vector4(0.8f, 0.6f, 0.4f, 1.f);
	material->GetShaderResource()->SetUniforms(uniforms);
	app.RunFrame();
	app.RunFrame();
	Check(material->GetShaderParameter("tint") == Variant(original), "reload preserves matching Color overrides");
	color = FindControl<ColorPickerButton>(FindNamed(&inspector, "shader_parameter/tint"));
	Check(color != nullptr && color->GetColor() == original, "dynamic UI refresh retains hint and float value");
	uniforms[1].m_hint = PropertyInfo::Hint::None;
	material->GetShaderResource()->SetUniforms(uniforms);
	app.RunFrame();
	app.RunFrame();
	Check(FindControl<ColorPickerButton>(FindNamed(&inspector, "shader_parameter/tint")) == nullptr
		&& FindControl<EditorSpinSlider>(FindNamed(&inspector, "shader_parameter/tint")) != nullptr
		&& material->GetShaderParameter("tint") == Variant(Vector4(original.r, original.g, original.b, original.a)), "removing a color hint preserves components and changes the API type");
	uniforms[1].m_hint = PropertyInfo::Hint::Color;
	uniforms[2].m_defaultValue = 0.25f;
	uniforms[2].m_hint = PropertyInfo::Hint::None;
	material->GetShaderResource()->SetUniforms(uniforms);
	loaded = Ref<ShaderMaterial>(ResourceLoader::LoadUncached(path));
	Check(loaded.IsValid() && loaded->GetShaderParameter("mode") == Variant(0.25f), "saved int override is skipped after a shader type change");
	app.RunFrame();
	app.RunFrame();
	color = FindControl<ColorPickerButton>(FindNamed(&inspector, "shader_parameter/tint"));
	Color const extended(-0.25f, 1.5f, 0.12345f, 1.f);
	material->SetShaderParameter("tint", extended);
	app.RunFrame();
	color->OpenPicker();
	app.RunFrame();
	CaptureMaterialFrame(*app.GetEngine(), (evidence / "hints-color-extended.ppm").string());
	color->ClosePicker(true);
	Check(material->GetShaderParameter("tint") == Variant(extended), "opening picker does not clamp external values");
	material->SetShaderParameter("tint", original);
	app.RunFrame();
	auto* picker = MemNew<ColorPicker>();
	picker->SetColorNoSignal(original);
	auto* channel = dynamic_cast<SpinBox*>(FindNamed(picker, "Channel1"));
	channel->Step(1);
	Color channelEdited = picker->GetColor();
	Check(channelEdited.r == original.r && channelEdited.b == original.b && channelEdited.g > original.g,
		"ordinary Color editing preserves float precision");
	picker->SetColorNoSignal(original);
	picker->SetHSVMode(true);
	Check(picker->GetColor() == original, "switching RGB and HSV does not change float values");
	auto* alpha = dynamic_cast<SpinBox*>(FindNamed(picker, "Alpha"));
	alpha->Step(-1);
	Color alphaEdited = picker->GetColor();
	Check(alphaEdited.r == original.r && alphaEdited.g == original.g && alphaEdited.b == original.b
		&& alphaEdited.a < 1.f && alphaEdited.a > 0.998f, "alpha editing preserves RGB precision in HSV mode");
	channel->Step(1);
	Check(picker->GetColor() != alphaEdited && picker->GetColor().a == alphaEdited.a,
		"HSV editing preserves floating alpha");
	picker->SetColorNoSignal(extended);
	picker->SetHSVMode(true);
	Check(!picker->IsHSVMode() && picker->GetColor() == extended, "extended RGB keeps exact values and disables HSV editing");
	auto* hex = dynamic_cast<LineEdit*>(FindNamed(picker, "HexColor"));
	Check(hex != nullptr && !hex->IsEditable(), "extended RGB disables Hex editing");
	channel->Step(1);
	Check(picker->GetColor().r == extended.r && picker->GetColor().b == extended.b
		&& picker->GetColor().g > extended.g, "extended RGB channel editing does not clamp other channels");
	MemDelete(picker);
	uniforms[2].m_defaultValue = 0;
	uniforms[2].m_hint = PropertyInfo::Hint::Enum;
	material->GetShaderResource()->SetUniforms(uniforms);
	editor->m_editorUI->GetDockSplit(1)->SetSplitRatio(0.75f);
	app.RunFrame();
	app.RunFrame();
	CaptureMaterialFrame(*app.GetEngine(), (evidence / "hints-wide.ppm").string());
	std::printf("SHADER_HINTS: PASS checks=%d\n", checks);
}

void PreviewShaderParameters(bool verifyHints = false)
{
	std::filesystem::current_path(GetExecutableDirectory());
	auto directory = std::filesystem::absolute("../Temporary/ShaderParameterPreview");
	std::filesystem::create_directories(directory / "Settings");
	_wputenv_s(L"LOCALAPPDATA", (directory / "Settings").c_str());
	Write(directory / "project.ming", R"({"type":"ProjectSettings","version":1,"start_scene_path":"res://main.tscn"})");
	Write(directory / "main.tscn", "[ming_scene format=1]\n[node name=\"ShaderParameters\" type=\"Node3D\" parent=\"\"]\n");
	Write(directory / "parameters.mshader", hintShaderSource);
	MingRunConfig config;
	config.mode = MingRunMode::Editor;
	config.projectPath = directory;
	App app(config);
	app.Startup();
	auto* editor = EditorNode::Get();
	Wait(app, [editor]() { return editor->HasScene() && !editor->IsCheckingProject(); }, "preview project opens");
	auto shader = Ref<ShaderResource>(ResourceLoader::Load("res://parameters.mshader"));
	auto material = CreateRef<ShaderMaterial>();
	material->SetName("Shader Parameters Preview");
	material->SetShaderResource(shader);
	VirtualPath path("res://parameters.tres");
	Check(ResourceSaver::Save(path, material), "preview material saves");
	material->SetVirtualPath(path);
	editor->m_editorUI->GetPanel<InspectorPanel>().EditResource(material);
	if (verifyHints)
	{
		VerifyShaderHints(app, directory, editor, material);
	}
	else
	{
		app.RunMainLoop();
	}
	app.Shutdown();
}

void VerifyResourceQuit(App& app, EditorNode* editor)
{
	if (editor->IsSceneDirty())
	{
		Check(editor->SaveScene(), "quit fixture saves scene baseline");
	}

	VirtualPath const path("res://QuitVerification/material.tres");
	Ref<StandardMaterial3D> material = CreateRef<StandardMaterial3D>();
	Check(ResourceSaver::Save(path, material), "quit fixture creates independent material");
	editor->m_editorUI->GetPanel<InspectorPanel>().EditResource(material);
	app.RunFrame();
	material->SetTint(Color::Blue);
	editor->MarkResourceDirty(material.Get());
	Check(!editor->IsSceneDirty() && editor->GetChecker()->IsResourceDirty(path), "quit fixture has only resource changes");
	auto* viewport = static_cast<Viewport*>(editor->GetSceneTree()->GetRoot());
	app.Quit();
	Check(!app.IsQuitting(), "resource-only changes block immediate quit");
	Wait(app, [viewport]() { return FindButton(viewport->GetGuiPopup(), "Save & Quit") != nullptr; }, "resource-only quit prompts");
	Choose(viewport, "Cancel");
	Check(!app.IsQuitting() && editor->GetChecker()->IsResourceDirty(path), "cancel quit retains dirty resource");
	app.Quit();
	Wait(app, [viewport]() { return FindButton(viewport->GetGuiPopup(), "Save & Quit") != nullptr; }, "resource-only quit can be retried");
	Choose(viewport, "Save & Quit");
	Wait(app, [&app]() { return app.IsQuitting(); }, "save and quit completes");
	Ref<StandardMaterial3D> saved(ResourceLoader::LoadUncached(path));
	Check(saved.IsValid() && saved->GetTint() == Color::Blue && !editor->GetChecker()->IsResourceDirty(path), "quit saves material changes before exit");
}

void Verify(bool resourcesOnly = false)
{
	auto const directory = std::filesystem::absolute("../Temporary/EditorProjectVerification/Project" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
	std::filesystem::create_directories(directory / "Settings");
	_wputenv_s(L"LOCALAPPDATA", (directory / "Settings").c_str());
	std::string const scene = R"([ming_scene format=1]

[node name="Main" type="Node" parent=""]

[node name="A" type="Node" parent="."]

[node name="B" type="Node" parent="."]

[node name="C" type="Node" parent="."]
)";
	Write(directory / "project.ming", R"({"type":"ProjectSettings","version":1,"start_scene_path":"res://main.tscn"})");
	Write(directory / "main.tscn", scene);
	Image(directory / "board.tga", 12);
	MingRunConfig config;
	config.mode = MingRunMode::Editor;
	config.projectPath = directory;
	App app(config);
	app.Startup();
	Check(!app.IsQuitting(), "editor startup");
	glfwHideWindow(app.GetEngine()->m_windowSystem->GetGLFWWindow());
	auto* editor = EditorNode::Get();
	auto* viewport = static_cast<Viewport*>(app.GetEngine()->m_sceneSystem->GetSceneTree()->GetRoot());
	Wait(app, [editor]() { return editor->HasScene() && !editor->IsCheckingProject(); }, "initial imports finish before scene opens");
	Check(std::filesystem::exists(directory / "board.tga.import"), "initial source imported");
	VerifyTextResources(app, directory, editor);
	VerifyShaderParameters(app, directory, editor);
	VerifyCanvasSampling(app);
	VerifyOptionButton(app, editor, directory.parent_path());
	VerifyFileRemovals(app, editor, directory);
	if (resourcesOnly)
	{
		VerifyResourceQuit(app, editor);
		app.Shutdown();
		return;
	}
	auto* selected = &editor->GetSelection();
	Node* root = editor->GetSceneTree()->GetScene();
	Node* c = root->FindChildByName("C");
	Node* b = root->FindChildByName("B");
	app.RunFrame();
	Tree* tree = editor->m_editorUI->GetSceneTreeControl();
	GuiDragData drag{ "node", c->GetObjectID(), "C" };
	auto rect = tree->GetItemRect(Key(b));
	Vector2 above(rect.GetCenter().x, rect.m_mins.y + 1.f);
	Check(tree->GetDropSectionAtPosition(above) == -1, "tree detects insertion above");
	Check(tree->CanDropData(above, drag), "sibling reorder accepted");
	tree->DropData(above, drag);
	Check(root->GetChildren()[1] == c && c->GetParent() == root, "sibling reorder preserves hierarchy");
	editor->GetUndoRedo().Undo(root->GetObjectID());
	Check(root->GetChildren()[2] == c, "reorder undo restores position");
	editor->GetUndoRedo().Redo(root->GetObjectID());
	Check(root->GetChildren()[1] == c, "reorder redo restores position");
	Check(editor->SaveScene(), "save edited scene");
	editor->CheckProject();
	for (int i = 0; i < 40; ++i)
	{
		app.RunFrame();
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	Check(viewport->GetGuiPopup() == nullptr, "own save does not show external conflict");

	Ref<TextureResource> texture(ResourceLoader::Load("res://board.tga"));
	Check(texture.IsValid(), "load imported texture");
	auto* textureIdentity = texture.Get();
	auto oldTime = std::filesystem::last_write_time(directory / "board.tga");
	Image(directory / "board.tga", 220);
	std::filesystem::last_write_time(directory / "board.tga", oldTime - std::chrono::seconds(20));
	editor->CheckProject();
	Wait(app, [&texture]() { return texture->m_image->GetRawData()[0] == 220; }, "time rollback triggers texture reimport and reload");
	Check(texture.Get() == textureIdentity, "reload retains resource identity");
	VirtualPath outputPath;
	Check(ResourceImporter::TryGetImportFile("res://board.tga", outputPath), "resolve generated texture");
	std::filesystem::path outputPhysical;
	app.GetEngine()->m_fileSystem->TryGetPhysicalPath(outputPath, outputPhysical);
	std::filesystem::remove(outputPhysical);
	editor->CheckProject();
	Wait(app, [&outputPhysical]() { return std::filesystem::exists(outputPhysical); }, "missing imported product is regenerated");
	Wait(app, [editor]() { return !editor->IsCheckingProject(); }, "recovery completes");

	Write(directory / "main.tscn", R"([ming_scene format=1]

[node name="External" type="Node" parent=""]
)");
	editor->CheckProject();
	Wait(app, [viewport]() { return FindButton(viewport->GetGuiPopup(), "Reload from disk") != nullptr; }, "external scene change prompts");
	Choose(viewport, "Reload from disk");
	Wait(app, [editor]() { return editor->GetSceneTree()->GetScene()->GetName() == "External"; }, "reload chooses disk scene");
	Check(&editor->GetSelection() == selected, "selection survives scene reload");
	Write(directory / "main.tscn", scene);
	editor->CheckProject();
	Wait(app, [viewport]() { return FindButton(viewport->GetGuiPopup(), "Reload from disk") != nullptr; }, "second external scene change prompts");
	Choose(viewport, "Keep and Save");
	std::ifstream saved(directory / "main.tscn");
	std::string savedText((std::istreambuf_iterator<char>(saved)), {});
	saved.close();
	Check(savedText.find("External") != std::string::npos, "ignore overwrites disk with editor scene");
	Node* retained = editor->GetSceneTree()->GetScene();
	Write(directory / "main.tscn", "{");
	editor->CheckProject();
	Wait(app, [viewport]() { return FindButton(viewport->GetGuiPopup(), "Reload from disk") != nullptr; }, "invalid external scene still prompts");
	Choose(viewport, "Reload from disk");
	Wait(app, [viewport]() { return FindButton(viewport->GetGuiPopup(), "OK") != nullptr; }, "reload failure is reported");
	Check(editor->GetSceneTree()->GetScene() == retained, "failed reload retains edited scene");
	Choose(viewport, "OK");
	Check(editor->SaveScene(), "save retained scene after failed reload");


	Ref<ProjectSettings> settings = ProjectSettings::Get();
	Write(directory / "project.ming", R"({"type":"ProjectSettings","version":1,"start_scene_path":""})");
	editor->CheckProject();
	Wait(app, [viewport]() { return FindButton(viewport->GetGuiPopup(), "Reload from disk") != nullptr; }, "project settings change prompts");
	Choose(viewport, "Reload from disk");
	Check(ProjectSettings::Get().Get() == settings.Get() && !settings->m_startScenePath.IsValid(), "configuration reload retains identity");

	std::filesystem::create_directories(directory.parent_path() / "External");
	auto const external = directory.parent_path() / "External" / "drop.tga";
	Image(external, 44);
	Tree* files = editor->m_editorUI->GetFileTreeControl();
	Vector2 const drop = files->GetGlobalPosition() + Vector2(files->GetSize().x * 0.5f, files->GetSize().y - 8.f);
	auto importFile = [&]()
	{
		std::string utf8 = external.u8string();
		// 1) Exercise the same editor callback used by GLFW, with an explicit drop position.
		editor->ImportExternalFiles({ utf8 }, drop);
	};
	importFile();
	Wait(app, [&directory]() { return std::filesystem::exists(directory / "drop.tga.import"); }, "external file copied and imported");
	Wait(app, [editor]() { return !editor->IsCheckingProject(); }, "external import completes");
	Image(external, 88);
	importFile();
	Wait(app, [viewport]() { return FindButton(viewport->GetGuiPopup(), "Skip") != nullptr; }, "copy collision prompts");
	Choose(viewport, "Skip");
	Wait(app, [editor]() { return !editor->IsCheckingProject(); }, "skipped copy completes");
	Ref<TextureResource> dropped(ResourceLoader::Load("res://drop.tga"));
	Check(dropped->m_image->GetRawData()[0] == 44, "skip preserves existing file");
	importFile();
	Wait(app, [viewport]() { return FindButton(viewport->GetGuiPopup(), "Skip") != nullptr; }, "second collision prompts");
	Choose(viewport, "Replace");
	Wait(app, [&dropped]() { return dropped->m_image->GetRawData()[0] == 88; }, "replace copies and updates loaded resource");
	Wait(app, [editor]() { return !editor->IsCheckingProject(); }, "replacement finishes");

	Write(directory / "Probe.cs", "using Ming; public partial class Probe : Node { }\n");
	Image(external, 120);
	importFile();
	Wait(app, [viewport]() { return FindButton(viewport->GetGuiPopup(), "Cancel") != nullptr; }, "cancel copy collision prompts");
	Choose(viewport, "Cancel");
	Check(dropped->m_image->GetRawData()[0] == 88, "cancel preserves destination");
	auto const folder = external.parent_path() / "Folder";
	std::filesystem::create_directories(folder);
	auto const unicodeName = std::filesystem::u8path(u8"\u4e2d\u6587.tga");
	Image(folder / unicodeName, 33);
	editor->ImportExternalFiles({ folder.u8string() }, drop);
	Wait(app, [&directory, &unicodeName]() { return std::filesystem::exists(directory / "Folder" / (unicodeName.wstring() + L".import")); }, "recursive copy imports Unicode filename");
	Wait(app, [editor]() { return !editor->IsCheckingProject(); }, "directory import completes");
	Check(editor->GenerateCSharpProject(), "generate C# project");
	Check(editor->BuildCSharpProject(), "start editor build");
	Wait(app, [editor]() { return !editor->IsBuildingCSharpProject(); }, "editor build completes");
	Check(editor->GetCSharpBuildExitCode() == 0 && viewport->GetGuiPopup() == nullptr, "C# build succeeds with editor pool");
	Check(editor->ReloadCSharpAssembly(), "native editor pool allows assembly reload");
	Check(editor->SetMainScene("res://main.tscn"), "set main scene for Play");
	editor->RequestPlay();
	Wait(app, [&app]() { return app.IsGameRunning(); }, "Play saves builds validates and starts game");
	editor->RequestStop();
	Wait(app, [&app]() { return !app.IsGameRunning(); }, "Stop reaps the game process");
	auto* pool = app.GetEngine()->m_workerThreadPool;
	std::atomic<int> result{0};
	auto id = pool->AddNativeTask([](void* data) { static_cast<std::atomic<int>*>(data)->store(1); }, &result);
	Check(id != WorkerThreadPool::INVALID_TASK_ID && !pool->HasManagedTasks(), "native pool task accepted without managed callbacks");
	Check(pool->WaitForTaskCompletion(id) && result == 1, "native task completes and is reclaimed");
	Check(!pool->WaitForTaskCompletion(id), "task cannot be reclaimed twice");
	editor->CheckProject();
	app.RunFrame();
	dropped = {};
	texture = {};
	settings = {};
	VerifyResourceQuit(app, editor);
	app.Shutdown();
	Check(EditorNode::Get() == nullptr, "shutdown drains checker before destroying editor");
}
}

int wmain(int argc, wchar_t** argv)
{
	if ((argc == 1 && std::filesystem::path(argv[0]).filename() == L"ShaderParameterPreview.exe")
		|| (argc == 2 && (std::wstring(argv[1]) == L"--preview-shader-parameters" || std::wstring(argv[1]) == L"--verify-shader-hints")))
	{
		try
		{
			PreviewShaderParameters(argc == 2 && std::wstring(argv[1]) == L"--verify-shader-hints");
			return 0;
		}
		catch (std::exception const& exception)
		{
			std::fprintf(stderr, "SHADER_HINTS: FAIL %s\n", exception.what());
			return 1;
		}
	}
	bool const resourcesOnly = argc == 2 && std::wstring(argv[1]) == L"--verify-resources";
	if (argc > 1 && !resourcesOnly)
	{
		MingRunConfig config;
		if (!TryParseRunConfig(config, argc, argv))
		{
			return 2;
		}
		App child(config);
		child.Startup();
		glfwHideWindow(child.GetEngine()->m_windowSystem->GetGLFWWindow());
		child.RunMainLoop();
		child.Shutdown();
		return child.GetExitCode();
	}

	try
	{
		Verify(resourcesOnly);
		std::printf("EDITOR_PROJECT: PASS checks=%d\n", checks);
		return 0;
	}
	catch (std::exception const& exception)
	{
		std::fprintf(stderr, "EDITOR_PROJECT: FAIL %s\n", exception.what());
		return 1;
	}
}
