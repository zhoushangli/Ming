#define NOMINMAX
#include "MingEngine/Engine/Application/App.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Application/ProjectSettings.hpp"
#include "MingEngine/Engine/Scene/SceneSystem.hpp"
#include "MingEngine/Engine/Script/ScriptSystem.hpp"
#include "MingEngine/Engine/Window/WindowSystem.hpp"
#include "MingEngine/Editor/EditorNode.hpp"
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
	Color result(pixel[0], pixel[1], pixel[2], pixel[3]);
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
	editor->GetSelection().SetSelected(canvas->GetObjectID());
	app.RunFrame();
	app.RunFrame();
	auto& inspector = editor->m_editorUI->GetPanel<InspectorPanel>();
	for (auto const& name : { "texture_filter", "texture_repeat" })
	{
		Node* field = FindNamed(inspector.GetContentControl(), name);
		Check(field != nullptr, "canvas texture properties retain their serialized names");
		Node* labelArea = FindNamed(field, "PropertyLabelArea");
		auto* title = dynamic_cast<Label*>(labelArea->GetChildren().front());
		Check(title != nullptr && title->GetText() == String(std::string(name) == "texture_filter" ? "Filter" : "Repeat"),
			"texture group displays short property labels");
	}
	Node* filterField = FindNamed(inspector.GetContentControl(), "texture_filter");
	auto* filter = dynamic_cast<OptionButton*>(FindButton(filterField, "Inherit"));
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
	auto texture = CreateRef<TextureResource>();
	texture->m_image = image;
	Check(texture->InitGPUResources(), "sampling fixture texture upload");
	auto* viewport = MemNew<SubViewport>();
	viewport->UpdateResolution(IntVec2(64, 16));
	app.GetEngine()->m_sceneSystem->GetSceneTree()->GetRoot()->AddNode(viewport);
	auto* first = MemNew<Control>();
	first->SetTextureFilter(TextureFilter::Nearest);
	viewport->AddNode(first);
	auto* second = MemNew<Control>();
	second->SetPosition(Vector2(32.f, 0.f));
	viewport->AddNode(second);
	auto* nearest = MemNew<TextureRect>();
	nearest->SetTexture(texture);
	nearest->SetExpandMode(TextureRect::ExpandMode::IgnoreSize);
	nearest->UpdateSize(Vector2(32.f, 16.f));
	first->AddNode(nearest);
	auto* linear = MemNew<TextureRect>();
	linear->SetTexture(texture);
	linear->SetExpandMode(TextureRect::ExpandMode::IgnoreSize);
	linear->UpdateSize(Vector2(32.f, 16.f));
	second->AddNode(linear);
	Color pixel = ReadCanvasPixel(app, viewport, 12, 8);
	Check(pixel.r > 240 && pixel.b < 10, "nearest sampling follows the parent CanvasItem");
	pixel = ReadCanvasPixel(app, viewport, 44, 8);
	Check(pixel.r > 80 && pixel.b > 20, "the same texture uses root-default linear sampling on another CanvasItem");
	first->SetTextureFilter(TextureFilter::Linear);
	pixel = ReadCanvasPixel(app, viewport, 12, 8);
	Check(pixel.r > 80 && pixel.b > 20, "parent sampling changes affect existing draw commands");
	nearest->SetTextureFilter(TextureFilter::Nearest);
	nearest->SetStretchMode(TextureRect::StretchMode::Tile);
	first->SetTextureRepeat(TextureRepeat::Enabled);
	pixel = ReadCanvasPixel(app, viewport, 4, 8);
	Check(pixel.r > 240 && pixel.b < 10, "repeat inherits independently of the child's filter override");
	nearest->SetTextureRepeat(TextureRepeat::Disabled);
	pixel = ReadCanvasPixel(app, viewport, 4, 8);
	Check(pixel.b > 240 && pixel.r < 10, "disabled repeat clamps even when tiled UVs exceed one");
	nearest->SetTextureRepeat(TextureRepeat::Inherit);
	first->SetTextureRepeat(TextureRepeat::Disabled);
	pixel = ReadCanvasPixel(app, viewport, 4, 8);
	Check(pixel.b > 240 && pixel.r < 10, "parent repeat changes affect inherited addressing");
	linear->QueueFree();
	app.RunFrame();
	nearest->Reparent(second, false);
	nearest->SetStretchMode(TextureRect::StretchMode::Scale);
	nearest->SetTextureFilter(TextureFilter::Inherit);
	pixel = ReadCanvasPixel(app, viewport, 44, 8);
	Check(pixel.r > 80 && pixel.b > 20, "reparenting resolves sampling from the new CanvasItem ancestry");
	nearest->SetVisible(false);
	auto* embedded = MemNew<SubViewportContainer>();
	embedded->UpdateSize(Vector2(32.f, 16.f));
	second->AddNode(embedded);
	auto* source = MemNew<TextureRect>();
	source->SetTexture(texture);
	source->SetTextureFilter(TextureFilter::Nearest);
	source->UpdateSize(Vector2(2.f, 1.f));
	embedded->GetSubViewport()->AddNode(source);
	app.RunFrame();
	embedded->GetSubViewport()->UpdateResolution(IntVec2(2, 1));
	embedded->SetTextureFilter(TextureFilter::Nearest);
	pixel = ReadCanvasPixel(app, viewport, 44, 8);
	Check(pixel.r > 240 && pixel.b < 10, "viewport textures obey CanvasItem nearest filtering");
	embedded->SetTextureFilter(TextureFilter::Linear);
	pixel = ReadCanvasPixel(app, viewport, 44, 8);
	Check(pixel.r > 80 && pixel.b > 20, "viewport textures obey CanvasItem linear filtering");
	viewport->QueueFree();
	app.RunFrame();
}

void VerifyTextResources(App& app, std::filesystem::path const& project, EditorNode* editor)
{
	std::filesystem::create_directories(project / "TextResources");
	VirtualPath const materialPath("res://TextResources/brick.tres");
	Ref<MaterialResource> original = CreateRef<MaterialResource>();
	original->SetName("Brick\n\"quoted\"\\path");
	original->SetTint(Color(123, 45, 67, 255));
	Image(project / "TextResources" / "white.tga", 255, true);
	Check(ResourceImporter::EnsureImported("res://TextResources/white.tga"), "resource fixture imports its own image");
	Ref<TextureResource> texture(ResourceLoader::Load("res://TextResources/white.tga"));
	Check(texture.IsValid(), "text fixture loads its own texture dependency");
	original->SetDiffuseTexture(texture);
	original->SetNormalTexture(texture);
	Check(ResourceSaver::Save(materialPath, original), "generic saver writes material");
	Ref<MaterialResource> material(ResourceLoader::Load(materialPath));
	Check(material.IsValid() && material->GetName() == original->GetName(), "name and escapes roundtrip");
	Check(material->GetTint() == original->GetTint(), "byte RGBA roundtrip");
	Check(material->GetDiffuseTexture() == texture && material->GetNormalTexture() == texture, "external references share cached identity");
	Check(!material->GetShaderResource().IsValid(), "null resource roundtrip");
	Check(material->GetVirtualPath() == materialPath, "loader supplies resource path");

	Ref<ShaderMaterial> shaderMaterial = CreateRef<ShaderMaterial>();
	shaderMaterial->SetTint(Color::Blue);
	shaderMaterial->SetDiffuseTexture(texture);
	shaderMaterial->SetBlendMode(BlendMode::ALPHA);
	Check(ResourceSaver::Save("res://TextResources/shader.tres", shaderMaterial), "ShaderMaterial saves through the text resource writer");
	Ref<ShaderMaterial> loadedShader(ResourceLoader::LoadUncached("res://TextResources/shader.tres"));
	Check(loadedShader.IsValid() && loadedShader->GetTint() == Color::Blue
		&& loadedShader->GetDiffuseTexture() == texture && loadedShader->GetBlendMode() == BlendMode::ALPHA,
		"ShaderMaterial preserves its concrete type and inherited properties");

	std::string text;
	std::string error;
	Check(ResourceFormatSaverText::Write(material, text, error), "material encodes without IO");
	auto rejects = [&](std::string const& contents)
	{
		error.clear();
		return !ResourceFormatLoaderText::Parse(contents, materialPath, error).IsValid() && error.find("brick.tres:") != std::string::npos;
	};
	Check(rejects("[ming_resource type=\"MissingClass\" format=1]\n[resource]\n"), "unknown resource class reports location");
	Check(rejects("[ming_resource type=\"MaterialResource\" format=2]\n[resource]\n"), "future format is rejected");
	Check(rejects("[ming_resource type=\"MaterialResource\" format=1]\n[resource]\ntint = [1,2,3,4]\n"), "plain arrays are not typed colors");
	Check(rejects("[ming_resource type=\"MaterialResource\" format=1]\n[resource]\ntint = Color(256,0,0,255)\n"), "out of range color is rejected");
	Check(rejects("[ming_resource type=\"MaterialResource\" format=1]\n[resource]\nmissing = 1\n"), "unknown property is rejected");
	Check(rejects("[ming_resource type=\"MaterialResource\" format=1]\n[resource]\nblend_mode = 999\n"), "setter rejected values fail loading");
	Check(rejects("[ming_resource type=\"MaterialResource\" format=1]\n[resource]\nblend_mode = 4294967296\n"), "int overflow is rejected");

	Ref<MaterialResource> anonymous = CreateRef<MaterialResource>();
	anonymous->SetShaderResource(app.GetEngine()->m_renderServer->GetDefaultLitShaderResource());
	Check(!ResourceFormatSaverText::Write(anonymous, text, error), "unsaved resource dependencies are rejected");
	Write(project / "TextResources" / "cycle.tres", "[ming_resource type=\"MaterialResource\" format=1]\n[ext_resource type=\"MaterialResource\" path=\"res://TextResources/cycle.tres\" id=\"1\"]\n[resource]\n");
	Check(!ResourceLoader::Load("res://TextResources/cycle.tres").IsValid(), "recursive dependencies fail without recursion overflow");

	Ref<SurfaceTool> surface = CreateRef<SurfaceTool>();
	surface->AddVertex(Vector3(0.f, 0.f, 0.f));
	surface->AddVertex(Vector3(0.f, 1.f, 0.f));
	surface->AddVertex(Vector3(0.f, 0.f, 1.f));
	surface->Index();
	Ref<MeshResource> mesh = surface->CreateMeshResource();
	Check(ResourceSaver::Save("res://TextResources/triangle.mesh", mesh), "specialized mesh saver remains available");
	Check(!ResourceFormatSaverText::Write(mesh, text, error), "text writer refuses incomplete mesh serialization");
	Ref<MaterialResource> other = CreateRef<MaterialResource>();
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
	canvas->SetTextureFilter(TextureFilter::Nearest);
	canvas->SetTextureRepeat(TextureRepeat::Enabled);
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
	Check(canvas != nullptr && canvas->GetTextureFilter() == TextureFilter::Nearest
		&& canvas->GetTextureRepeat() == TextureRepeat::Enabled, "CanvasItem sampling survives scene serialization");
	MemDelete(root);

	auto& inspector = editor->m_editorUI->GetPanel<InspectorPanel>();
	inspector.EditResource(material);
	app.RunFrame();
	app.RunFrame();
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
	VirtualPath copiedPath;
	editor->m_editorUI->SaveMaterialAs(material, [&copiedPath](VirtualPath const& path) { copiedPath = path; });
	app.RunFrame();
	filename = dynamic_cast<LineEdit*>(FindNamed(viewport->GetGuiPopup(), "SceneFilename"));
	Check(filename != nullptr, "material copy opens save as dialog");
	filename->SetText("Unique.tres");
	Choose(viewport, "Save");
	Ref<MaterialResource> copy(ResourceLoader::Load(copiedPath));
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
	Check(shaderCopy.IsValid() && shaderCopy->GetTint() == loadedShader->GetTint()
		&& shaderCopy->GetDiffuseTexture() == loadedShader->GetDiffuseTexture(),
		"save as preserves ShaderMaterial type and inherited values");
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

	Ref<MaterialResource> disk = CreateRef<MaterialResource>();
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
	Ref<MaterialResource> saved(ResourceLoader::LoadUncached(materialPath));
	Check(saved.IsValid() && saved->GetTint() == Color::Green && !editor->GetChecker()->IsResourceDirty(materialPath), "keep and save writes memory and clears dirty");
}


void VerifyResourceQuit(App& app, EditorNode* editor)
{
	if (editor->IsSceneDirty())
	{
		Check(editor->SaveScene(), "quit fixture saves scene baseline");
	}

	VirtualPath const path("res://QuitVerification/material.tres");
	Ref<MaterialResource> material = CreateRef<MaterialResource>();
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
	Ref<MaterialResource> saved(ResourceLoader::LoadUncached(path));
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
