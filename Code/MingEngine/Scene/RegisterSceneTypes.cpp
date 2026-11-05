#include "MingEngine/Scene/RegisterSceneTypes.hpp"

#include "MingEngine/Core/Object/ClassDatabase.hpp"
#include "MingEngine/Core/Render/SurfaceTool.hpp"
#include "MingEngine/Scene/Resource/MaterialResource.hpp"
#include "MingEngine/Core/Object/ResourceImporter.hpp"
#include "MingEngine/Core/Object/ResourceLoader.hpp"
#include "MingEngine/Core/Object/ResourceSaver.hpp"
#include "MingEngine/Editor/EditorCamera.hpp"
#include "MingEngine/Editor/EditorNode.hpp"
#include "MingEngine/Editor/Gizmos/EditorGizmos.hpp"
#include "MingEngine/Engine/Script/CSharpScriptFormat.hpp"
#include "MingEngine/Scene/3D/Camera3D.hpp"
#include "MingEngine/Scene/3D/Light3D.hpp"
#include "MingEngine/Scene/3D/MeshInstance3D.hpp"
#include "MingEngine/Scene/3D/Node3D.hpp"
#include "MingEngine/Scene/3D/VisualInstance3D.hpp"
#include "MingEngine/Scene/Core/CanvasItem.hpp"
#include "MingEngine/Scene/Core/Node.hpp"
#include "MingEngine/Scene/Core/PackedScene.hpp"
#include "MingEngine/Scene/Core/PackedSceneFormat.hpp"
#include "MingEngine/Scene/Core/RaycastSpace3D.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"
#include "MingEngine/Scene/Core/Viewport.hpp"
#include "MingEngine/Scene/Core/Window.hpp"
#include "MingEngine/Scene/GUI/BoxContainer.hpp"
#include "MingEngine/Scene/GUI/ColorRect.hpp"
#include "MingEngine/Scene/GUI/ColorPicker.hpp"
#include "MingEngine/Scene/GUI/ColorPickerButton.hpp"
#include "MingEngine/Scene/GUI/Container.hpp"
#include "MingEngine/Scene/GUI/Control.hpp"
#include "MingEngine/Scene/GUI/BaseButton.hpp"
#include "MingEngine/Scene/GUI/Button.hpp"
#include "MingEngine/Scene/GUI/PanelContainer.hpp"
#include "MingEngine/Scene/GUI/HBoxContainer.hpp"
#include "MingEngine/Scene/GUI/MenuBar.hpp"
#include "MingEngine/Scene/GUI/Label.hpp"
#include "MingEngine/Scene/GUI/LineEdit.hpp"
#include "MingEngine/Scene/GUI/SpinBox.hpp"
#include "MingEngine/Scene/GUI/OptionButton.hpp"
#include "MingEngine/Scene/GUI/PopupPanel.hpp"
#include "MingEngine/Scene/GUI/Tree.hpp"
#include "MingEngine/Scene/GUI/TabContainer.hpp"
#include "MingEngine/Scene/GUI/SplitContainer.hpp"
#include "MingEngine/Scene/GUI/SubViewportContainer.hpp"
#include "MingEngine/Scene/GUI/ScrollContainer.hpp"
#include "MingEngine/Scene/GUI/MarginContainer.hpp"
#include "MingEngine/Scene/GUI/VBoxContainer.hpp"
#include "MingEngine/Scene/Import/GLTFImporter.hpp"
#include "MingEngine/Scene/Import/ImageImporter.hpp"
#include "MingEngine/Scene/Import/OBJImporter.hpp"
#include "MingEngine/Scene/Physics/AABBCollider3D.hpp"
#include "MingEngine/Scene/Physics/CapsuleCollider3D.hpp"
#include "MingEngine/Scene/Physics/CylinderZCollider3D.hpp"
#include "MingEngine/Scene/Physics/TriangleMeshCollider3D.hpp"
#include "MingEngine/Scene/Resource/FontResource.hpp"
#include "MingEngine/Scene/Resource/FontResourceFormat.hpp"
#include "MingEngine/Scene/Resource/MeshResourceFormat.hpp"
#include "MingEngine/Scene/Resource/ShaderResource.hpp"
#include "MingEngine/Scene/Resource/ShaderResourceFormat.hpp"
#include "MingEngine/Scene/Resource/TextureResource.hpp"
#include "MingEngine/Scene/Resource/TextureResourceFormat.hpp"

#include "MingEngine/Scene/Resource/Theme.hpp"

#pragma region Scene

void RegisterSceneTypes()
{
	ClassDatabase::SetApiType(ApiType::Runtime);


	BIND_ENUM(SamplerMode, POINT_CLAMP);
	BIND_ENUM(SamplerMode, BILINEAR_CLAMP);
	BIND_ENUM(SamplerMode, POINT_WRAP);
	BIND_ENUM(SamplerMode, BILINEAR_WRAP);
	BIND_ENUM(SamplerMode, COUNT);
	BIND_ENUM(BlendMode, ALPHA);
	BIND_ENUM(BlendMode, ADDITIVE);
	BIND_ENUM(BlendMode, OPAQUE);
	BIND_ENUM(BlendMode, COUNT);
	BIND_ENUM(DepthMode, DISABLED);
	BIND_ENUM(DepthMode, READ_ONLY_ALWAYS);
	BIND_ENUM(DepthMode, READ_ONLY_LESS_EQUAL);
	BIND_ENUM(DepthMode, READ_WRITE_LESS_EQUAL);
	BIND_ENUM(DepthMode, COUNT);
	BIND_ENUM(RasterizerMode, SOLID_CULL_NONE);
	BIND_ENUM(RasterizerMode, SOLID_CULL_BACK);
	BIND_ENUM(RasterizerMode, WIREFRAME_CULL_NONE);
	BIND_ENUM(RasterizerMode, WIREFRAME_CULL_BACK);
	BIND_ENUM(RasterizerMode, COUNT);

	ClassDatabase::RegisterClass<MaterialResource>();
	ClassDatabase::RegisterClass<SurfaceTool>();

	// Resource types
	ClassDatabase::RegisterClass<ThemeStyleBox>();
	ClassDatabase::RegisterClass<Theme>();
	ClassDatabase::RegisterClass<MeshResource>();
	ClassDatabase::RegisterClass<FontResource>();
	ClassDatabase::RegisterClass<TextureResource>();
	ClassDatabase::RegisterClass<PackedScene>();
	ClassDatabase::RegisterClass<ShaderResource>();
	ClassDatabase::RegisterClass<CSharpScript>();

	// Resource format types
	ClassDatabase::RegisterClass<PackedSceneLoader>();
	ClassDatabase::RegisterClass<MeshResourceLoader>();
	ClassDatabase::RegisterClass<FontResourceLoader>();
	ClassDatabase::RegisterClass<TextureResourceLoader>();
	ClassDatabase::RegisterClass<ShaderResourceLoader>();
	ClassDatabase::RegisterClass<CSharpScriptLoader>();

	ClassDatabase::RegisterClass<PackedSceneSaver>();
	ClassDatabase::RegisterClass<MeshResourceSaver>();
	ClassDatabase::RegisterClass<TextureResourceSaver>();
	// ClassDatabase::RegisterClass<ShaderResourceSaver>();
	ClassDatabase::RegisterClass<CSharpScriptSaver>();

	ClassDatabase::RegisterClass<OBJImporter>();
	ClassDatabase::RegisterClass<GLTFImporter>();
	ClassDatabase::RegisterClass<ImageImporter>();

	// Resource format registration
	ResourceLoader::AddLoader(CreateRef<PackedSceneLoader>());
	ResourceLoader::AddLoader(CreateRef<MeshResourceLoader>());
	ResourceLoader::AddLoader(CreateRef<FontResourceLoader>());
	ResourceLoader::AddLoader(CreateRef<TextureResourceLoader>());
	ResourceLoader::AddLoader(CreateRef<ShaderResourceLoader>());
	ResourceLoader::AddLoader(CreateRef<CSharpScriptLoader>());

	ResourceSaver::AddSaver(CreateRef<PackedSceneSaver>());
	ResourceSaver::AddSaver(CreateRef<MeshResourceSaver>());
	ResourceSaver::AddSaver(CreateRef<TextureResourceSaver>());
	// ResourceSaver::AddSaver(CreateRef<ShaderResourceSaver>());
	ResourceSaver::AddSaver(CreateRef<CSharpScriptSaver>());

	ResourceImporter::AddImporter(CreateRef<OBJImporter>());
	ResourceImporter::AddImporter(CreateRef<GLTFImporter>());
	ResourceImporter::AddImporter(CreateRef<ImageImporter>());

	// Scene types
	ClassDatabase::RegisterClass<Node>();
	ClassDatabase::RegisterClass<Viewport>();
	ClassDatabase::RegisterAbstractClass<Window>();
	ClassDatabase::RegisterClass<SceneTree>();
	ClassDatabase::RegisterClass<RaycastSpace3D>();
	ClassDatabase::RegisterClass<Node3D>();
	ClassDatabase::RegisterClass<Camera3D>();
	ClassDatabase::RegisterAbstractClass<Light3D>();
	ClassDatabase::RegisterClass<DirectionalLight3D>();
	ClassDatabase::RegisterClass<OmniLight3D>();
	ClassDatabase::RegisterClass<SpotLight3D>();
	ClassDatabase::RegisterClass<Collider3D>(true);
	ClassDatabase::RegisterClass<AABBCollider3D>();
	ClassDatabase::RegisterClass<CapsuleCollider3D>();
	ClassDatabase::RegisterClass<CylinderZCollider3D>();
	ClassDatabase::RegisterClass<TriangleMeshCollider3D>();
	ClassDatabase::RegisterAbstractClass<VisualInstance3D>();
	ClassDatabase::RegisterClass<MeshInstance3D>();
	ClassDatabase::RegisterClass<CanvasItem>();
	ClassDatabase::RegisterClass<Control>();
	ClassDatabase::RegisterAbstractClass<BaseButton>();
	ClassDatabase::RegisterClass<Button>();
	ClassDatabase::RegisterClass<PanelContainer>();
	ClassDatabase::RegisterClass<ColorRect>();
	ClassDatabase::RegisterClass<ColorPicker>();
	ClassDatabase::RegisterClass<ColorPickerButton>();
	ClassDatabase::RegisterClass<Label>();
	ClassDatabase::RegisterClass<LineEdit>();
	ClassDatabase::RegisterClass<SpinBox>();
	ClassDatabase::RegisterClass<OptionButton>();
	ClassDatabase::RegisterClass<PopupPanel>();
	ClassDatabase::RegisterClass<Tree>();
	ClassDatabase::RegisterClass<TabContainer>();
	ClassDatabase::RegisterClass<SplitContainer>();
	ClassDatabase::RegisterClass<SubViewportContainer>();
	ClassDatabase::RegisterClass<ScrollContainer>();
	ClassDatabase::RegisterClass<Container>();
	ClassDatabase::RegisterClass<MarginContainer>();
	ClassDatabase::RegisterClass<BoxContainer>();
	ClassDatabase::RegisterClass<HBoxContainer>();
	ClassDatabase::RegisterClass<MenuBar>();
	ClassDatabase::RegisterClass<VBoxContainer>();

	// Editor types
	ClassDatabase::SetApiType(ApiType::Editor);

	ClassDatabase::RegisterClass<EditorNode>();
	ClassDatabase::RegisterClass<EditorGizmos>();
	ClassDatabase::RegisterClass<EditorCamera>();
}

#pragma endregion
