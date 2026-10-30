#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Application/ProjectSettings.hpp"
#include "MingEngine/Editor/EditorNode.hpp"
#include "MingEngine/Editor/UI/InspectorPanel.hpp"
#include "MingEngine/Editor/UI/InspectorProperty.hpp"
#include "MingEngine/Editor/UI/InspectorPropertyEnum.hpp"
#include "MingEngine/Editor/UI/InspectorPropertyInt.hpp"
#include "MingEngine/Editor/UI/Popup/ProjectSettingsPopup.hpp"
#include "MingEngine/Scene/RegisterAllTypes.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"
#include "MingEngine/Scene/Core/Viewport.hpp"
#include "MingEngine/Scene/Core/Window.hpp"
#include "MingEngine/Scene/Core/PackedScene.hpp"
#include "MingEngine/Scene/GUI/SpinBox.hpp"
#include "MingEngine/Scene/GUI/OptionButton.hpp"
#include "MingEngine/Scene/GUI/ScrollContainer.hpp"
#include "MingEngine/Scene/GUI/VBoxContainer.hpp"
#include "MingEngine/Scene/GUI/ColorRect.hpp"
#include "MingEngine/Scene/GUI/TextureRect.hpp"
#include "MingEngine/Engine/Render/GPUTexture.hpp"
#include "MingEngine/Core/Object/ResourceLoader.hpp"
#include "MingEngine/Core/Object/ResourceSaver.hpp"
#include "MingEngine/Core/Object/VariantJson.hpp"
#include "ThirdParty/imgui/imgui.h"
#include "MingEngine/Editor/UI/EditorUI.hpp"
#include "MingEngine/Editor/UI/JetBrainsMonoFont.hpp"
#include "MingEngine/Scene/GUI/Tree.hpp"
#include "MingEngine/Scene/GUI/TabContainer.hpp"
#include "MingEngine/Scene/GUI/SplitContainer.hpp"
#include "MingEngine/Scene/GUI/SubViewportContainer.hpp"
#include "MingEngine/Scene/GUI/PopupPanel.hpp"
#include "MingEngine/Scene/3D/MeshInstance3D.hpp"
#include "MingEngine/Editor/EditorCamera.hpp"
#include "MingEngine/Scene/3D/Light3D.hpp"
#include "MingEngine/Core/Render/VertexUtils.hpp"
#include "MingEngine/Core/Render/SurfaceTool.hpp"

#include "RenderReadback.hpp"
#include <d3d11.h>
#include <wrl/client.h>

#ifdef RegisterClass
#undef RegisterClass
#endif

#ifdef GetClassName
#undef GetClassName
#endif

#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
#include <memory>
#include <random>
#include <stdexcept>

class FieldProbe : public Node3D
{
	MCLASS(FieldProbe, Node3D);

public:
	int         m_mode   = 0;
	float       m_number = 0.f;
	void        SetMode(int value) { m_mode = value; }
	int         GetMode() const { return m_mode; }
	void        SetNumber(float value) { m_number = std::clamp(value, -10.f, 10.f); }
	float       GetNumber() const { return m_number; }
	static void BindMethods()
	{
		ClassDatabase::BindMethod("SetMode", &FieldProbe::SetMode, { "value" });
		ClassDatabase::BindMethod("GetMode", &FieldProbe::GetMode, {});
		ClassDatabase::BindMethod("SetNumber", &FieldProbe::SetNumber, { "value" });
		ClassDatabase::BindMethod("GetNumber", &FieldProbe::GetNumber, {});
		ADD_PROPERTY(
			PropertyInfo(
				Variant::Type::Int,
				"mode",
				PropertyInfo::Hint::Enum,
				"Idle:0,Walk:10,Run:20",
				PropertyInfo::UsageFlags::Default),
			"SetMode",
			"GetMode");
		ADD_PROPERTY(
			PropertyInfo(
				Variant::Type::Float,
				"number",
				PropertyInfo::Hint::None,
				"",
				PropertyInfo::UsageFlags::Default),
			"SetNumber",
			"GetNumber");
	}
};

template <typename T>
T* FindRecursive(Node* root, int& index)
{
	if (auto* target = dynamic_cast<T*>(root); target != nullptr && index-- == 0)
	{
		return target;
	}

	for (Node* child : root->GetChildren())
	{
		if (T* target = FindRecursive<T>(child, index))
		{
			return target;
		}
	}

	return nullptr;
}

template <typename T>
T* Find(Node* root, int index = 0)
{
	return FindRecursive<T>(root, index);
}

void Key(Viewport* viewport, int key, int modifiers = 0)
{
	InputEvent event;
	event.m_type      = InputEventType::Key;
	event.m_keyCode   = key;
	event.m_pressed   = true;
	event.m_modifiers = modifiers;
	viewport->ProcessGuiInputEvent(event);
}

void Draft(Viewport* viewport, LineEdit* edit, char const* text)
{
	viewport->SetGuiFocus(nullptr);
	edit->GrabFocus();
	edit->SelectAll();
	Key(viewport, GLFW_KEY_BACKSPACE);
	for (char const* cursor = text; *cursor != '\0'; ++cursor)
	{
		InputEvent event;
		event.m_type      = InputEventType::Text;
		event.m_codepoint = static_cast<uint32_t>(*cursor);
		viewport->ProcessGuiInputEvent(event);
	}
}

int main(int argc, char** argv)
{
	int  checks   = 0;
	int  failures = 0;
	auto check    = [&](bool result, char const* name)
	{
		++checks;
		if (!result)
		{
			++failures;
			std::printf("FAIL: %s\n", name);
		}

		std::fflush(stdout);
	};
	if (argc != 2)
	{
		return 2;
	}

	std::filesystem::path const projectPath = std::filesystem::absolute(argv[1]);
	std::filesystem::create_directories(projectPath);
	EngineConfig config;
	config.m_imguiConfig.m_isEnable          = false;
	config.m_fileSystemConfig.m_resourceRoot = projectPath;
	config.m_scriptSystemConfig.m_isEnabled  = false;
	config.m_audioConfig.m_isEnable          = false;
	config.m_networkConfig.m_isEnable        = false;
	config.m_windowConfig.m_appName          = "Native Fields Automated Verification";
	Engine engine(config);
	ClassDatabase::Startup();
	RegisterAllTypes();
	ClassDatabase::RegisterClass<FieldProbe>();
	engine.Startup();
	{
		SceneTree tree;
		auto* viewport = static_cast<Viewport*>(tree.GetRoot());
		auto* canvas = MemNew<Control>();
		viewport->AddNode(canvas);
		auto* source = MemNew<Button>();
		canvas->AddNode(source);
		source->SetPosition(Vector2(20.f, 20.f));
		source->UpdateSize(Vector2(120.f, 50.f));
		auto* target = MemNew<ColorRect>();
		target->SetMouseFilter(Control::MouseFilter::Stop);
		canvas->AddNode(target);
		target->SetPosition(Vector2(250.f, 20.f));
		target->UpdateSize(Vector2(160.f, 100.f));
		auto* child = MemNew<Control>();
		target->AddNode(child);
		child->SetMouseFilter(Control::MouseFilter::Pass);
		child->UpdateSize(Vector2(100.f, 60.f));
		int drops = 0;
		int clicks = 0;
		bool allowSource = true;
		source->SetPressedCallback([&]() { ++clicks; });
		source->SetDragCallbacks(
			[&](Vector2 const&, GuiDragData& data)
			{
				data.m_type = "test";
				data.m_value = "payload";
				return allowSource;
			}, {}, {});
		target->SetDragCallbacks({},
			[](Vector2 const& position, GuiDragData const& data)
			{
				return data.m_type == String("test") && position.x >= 0.f;
			},
			[&](Vector2 const&, GuiDragData const& data)
			{
				check(!viewport->IsGuiDragging() && !viewport->HasGuiMouseCapture(), "drop ends capture before commit");
				check(data.m_value == String("payload"), "drop preserves payload after session cleanup");
				++drops;
			});
		auto pointer = [&](InputEventType type, Vector2 position, bool pressed = false)
		{
			InputEvent event;
			event.m_type = type;
			event.m_position = position;
			event.m_mouseButton = GLFW_MOUSE_BUTTON_LEFT;
			event.m_pressed = pressed;
			viewport->ProcessInputEvent(event);
		};
		auto begin = [&]()
		{
			pointer(InputEventType::MouseButton, Vector2(40.f, 40.f), true);
			pointer(InputEventType::MouseMotion, Vector2(49.f, 40.f));
		};
		pointer(InputEventType::MouseButton, Vector2(40.f, 40.f), true);
		pointer(InputEventType::MouseMotion, Vector2(48.f, 40.f));
		check(!viewport->IsGuiDragging(), "drag threshold preserves an eight-unit click");
		pointer(InputEventType::MouseMotion, Vector2(49.f, 40.f));
		check(viewport->IsGuiDragging() && !viewport->HasGuiMouseCapture(), "drag starts beyond threshold and releases ordinary capture");
		pointer(InputEventType::MouseMotion, Vector2(280.f, 40.f));
		check(viewport->GetGuiDragTarget() == target, "drop checks parent of pass-through child");
		auto* preview = dynamic_cast<Control*>(viewport->GetChildren().back());
		check(preview != nullptr && preview->GetMouseFilter() == Control::MouseFilter::Ignore && !preview->GetSerializable(),
			"drag preview ignores pointer and scene serialization");
		pointer(InputEventType::MouseButton, Vector2(280.f, 40.f));
		pointer(InputEventType::MouseButton, Vector2(280.f, 40.f));
		check(drops == 1 && clicks == 0, "drag commits once without activating source button");
		begin();
		Key(viewport, GLFW_KEY_ESCAPE);
		pointer(InputEventType::MouseButton, Vector2(280.f, 40.f));
		check(!viewport->IsGuiDragging() && drops == 1, "escape cancels drag without commit");
		begin();
		InputEvent focus;
		focus.m_type = InputEventType::WindowFocus;
		focus.m_focused = false;
		viewport->ProcessInputEvent(focus);
		check(!viewport->IsGuiDragging(), "window focus loss cancels drag");
		focus.m_focused = true;
		viewport->ProcessInputEvent(focus);
		begin();
		pointer(InputEventType::MouseButton, Vector2(700.f, 600.f));
		check(!viewport->IsGuiDragging() && drops == 1, "invalid target release ends drag");
		begin();
		child->SetMouseFilter(Control::MouseFilter::Stop);
		pointer(InputEventType::MouseMotion, Vector2(280.f, 40.f));
		check(viewport->GetGuiDragTarget() == nullptr, "stop filter prevents parent drop");
		viewport->CancelGuiDrag();
		child->SetMouseFilter(Control::MouseFilter::Pass);
		allowSource = false;
		begin();
		pointer(InputEventType::MouseButton, Vector2(49.f, 40.f));
		check(!viewport->IsGuiDragging() && clicks == 1, "source without drag data retains ordinary button click");
		allowSource = true;
		begin();
		auto* popup = MemNew<PopupPanel>();
		canvas->AddNode(popup);
		popup->Open(source);
		check(!viewport->IsGuiDragging(), "opening a modal cancels active drag");
		popup->Close();
		target->SetClipContents(true);
		child->SetPosition(Vector2(145.f, 0.f));
		begin();
		pointer(InputEventType::MouseMotion, Vector2(430.f, 40.f));
		check(viewport->GetGuiDragTarget() == nullptr, "clipped child outside ancestor cannot receive drop");
		viewport->CancelGuiDrag();
		child->SetPosition(Vector2::Zero);
		begin();
		source->QueueFree();
		viewport->UpdateGuiDrag(Vector2(280.f, 40.f));
		check(!viewport->IsGuiDragging(), "queued source deletion cancels drag safely");
		tree.FlushPendingNode();
	}

	{
		SceneTree  tree;
		EditorNode editor;
		Viewport*  root = static_cast<Viewport*>(tree.GetRoot());
		Variant    parsed;
		for (char const* text : { "", "-", "+", "1.5", "1e2", "2147483648", "-2147483649", "12abc", "1+2" })
		{
			check(!SpinBox::ParseValue(text, true, parsed), "invalid integer rejected");
		}

		check(
			SpinBox::ParseValue(" -2147483648 ", true, parsed) && parsed.As<int>() == std::numeric_limits<int>::min(),
			"integer minimum");
		check(
			SpinBox::ParseValue("+2147483647", true, parsed) && parsed.As<int>() == std::numeric_limits<int>::max(),
			"integer maximum");
		for (char const* text : { "", "-", ".", "1e", "NaN", "Infinity", "1e100", "1e-100", "2x", "0x1p2", "--1" })
		{
			check(!SpinBox::ParseValue(text, false, parsed), "invalid float rejected");
		}

		check(SpinBox::ParseValue(" -0.125 ", false, parsed) && parsed.As<float>() == -0.125f, "negative decimal");
		check(SpinBox::ParseValue("+1.25e-2", false, parsed) && parsed.As<float>() == 0.0125f, "scientific decimal");
		check(
			SpinBox::ParseValue("1.40129846e-45", false, parsed)
				&& parsed.As<float>() == std::numeric_limits<float>::denorm_min(),
			"representable subnormal");
		std::mt19937 random(7331);
		for (int i = 0; i < 1000; ++i)
		{
			uint32_t const bits     = random();
			float          original = 0.f;
			std::memcpy(&original, &bits, sizeof(original));
			if (std::isfinite(original))
			{
				check(
					SpinBox::ParseValue(SpinBox::FormatValue(Variant(original)), false, parsed)
						&& std::memcmp(&parsed.As<float>(), &original, sizeof(original)) == 0,
					"float display bitwise round trip");
			}
		}

		auto* spin = MemNew<SpinBox>();
		root->AddNode(spin);
		spin->Configure(true, "-5,5,2");
		spin->SetValueNoSignal(Variant(3));
		int changes = 0;
		spin->SetValueChangedCallback(
			[&](Variant const&)
			{
				++changes;
			});
		Draft(root, spin->GetLineEdit(), "-");
		check(spin->GetValue().As<int>() == 3 && changes == 0, "incomplete draft does not write zero");
		Key(root, GLFW_KEY_ENTER);
		check(
			spin->GetValue().As<int>() == 3 && !spin->GetError().empty()
				&& spin->GetLineEdit()->GetText() == String("3"),
			"invalid submission restores value with error");
		Draft(root, spin->GetLineEdit(), "2.5");
		root->SetGuiFocus(nullptr);
		check(spin->GetValue().As<int>() == 3 && changes == 0, "integer rejects decimal on blur");
		Draft(root, spin->GetLineEdit(), "99");
		Key(root, GLFW_KEY_ENTER);
		check(spin->GetValue().As<int>() == 3 && changes == 0, "range rejects submission");
		Draft(root, spin->GetLineEdit(), "-4");
		root->SetGuiFocus(nullptr);
		check(spin->GetValue().As<int>() == -4 && changes == 1, "valid blur commits");
		Draft(root, spin->GetLineEdit(), "1");
		Key(root, GLFW_KEY_ESCAPE);
		check(spin->GetValue().As<int>() == -4 && changes == 1, "escape cancels");
		Draft(root, spin->GetLineEdit(), "4");
		spin->Step(1);
		check(spin->GetValue().As<int>() == 5 && changes == 2, "step uses draft and clamps");
		spin->Step(1);
		check(spin->GetValue().As<int>() == 5 && changes == 2, "step stops at boundary");
		Draft(root, spin->GetLineEdit(), "garbage");
		spin->Step(-1);
		check(spin->GetValue().As<int>() == 5 && changes == 2, "invalid step does not change value");
		Draft(root, spin->GetLineEdit(), "0");
		InputEvent lost;
		lost.m_type    = InputEventType::WindowFocus;
		lost.m_focused = false;
		root->ProcessInputEvent(lost);
		check(spin->GetValue().As<int>() == 5 && changes == 2, "window loss interrupts draft");
		lost.m_focused = true;
		root->ProcessInputEvent(lost);
		check(!spin->Configure(true, "5,-5,1"), "reversed range hint rejected");
		check(!spin->Configure(false, "0,1,0"), "zero step rejected");
		spin->Configure(true, "2147483646,2147483647,1");
		spin->SetValueNoSignal(Variant(2147483646));
		spin->Step(1);
		spin->Step(1);
		check(spin->GetValue().As<int>() == 2147483647, "integer stepping preserves maximum precision");
		spin->Configure(false);
		spin->SetValueNoSignal(Variant(1.23456789f));
		float const precise = spin->GetValue().As<float>();
		root->SetGuiFocus(nullptr);
		spin->GetLineEdit()->GrabFocus();
		spin->GetLineEdit()->SetCursorColumn(3);
		spin->SetValueNoSignal(Variant(9.5f));
		check(
			spin->GetLineEdit()->GetCursorColumn() == 3
				&& spin->GetLineEdit()->GetText() == String(SpinBox::FormatValue(Variant(precise))),
			"refresh preserves draft and cursor");
		Key(root, GLFW_KEY_ESCAPE);
		check(
			spin->GetValue().As<float>() == 9.5f && spin->GetLineEdit()->GetText() == String("9.5"),
			"cancel restores latest external value");
		Draft(root, spin->GetLineEdit(), "-1.25");
		Key(root, GLFW_KEY_ENTER);
		check(spin->GetValue().As<float>() == -1.25f, "float text commits");
		Draft(root, spin->GetLineEdit(), "-2.75");
		Key(root, GLFW_KEY_Z, GLFW_MOD_CONTROL);
		check(
			spin->GetLineEdit()->GetText() == String("-2.7") && spin->GetValue().As<float>() == -1.25f,
			"numeric draft undo uses text history without changing value");
		Key(root, GLFW_KEY_Y, GLFW_MOD_CONTROL);
		check(spin->GetLineEdit()->GetText() == String("-2.75"), "numeric draft redo restores text");
		Key(root, GLFW_KEY_ESCAPE);
		root->SetGuiFocus(nullptr);
		spin->GetLineEdit()->GrabFocus();
		spin->SetValueNoSignal(Variant(8.25f));
		root->SetGuiFocus(nullptr);
		check(
			spin->GetValue().As<float>() == 8.25f && spin->GetLineEdit()->GetText() == String("8.25"),
			"unchanged edit does not overwrite external update on blur");

		auto* option = MemNew<OptionButton>();
		root->AddNode(option);
		check(option->SetOptions("Idle:0,Walk:10,Run:20"), "enum hint parsed");
		option->SetValueNoSignal(99);
		check(option->GetText() == String("99") && option->GetValue() == 99, "unknown enum preserved");
		option->SetValueNoSignal(0);
		option->OpenOptions();
		Key(root, GLFW_KEY_DOWN);
		Key(root, GLFW_KEY_ENTER);
		check(
			option->GetValue() == 10 && root->GetGuiPopup() == nullptr && option->HasFocus(),
			"enum keyboard selects actual value and restores focus");
		option->OpenOptions();
		Key(root, GLFW_KEY_DOWN);
		Key(root, GLFW_KEY_ESCAPE);
		check(
			option->GetValue() == 10 && root->GetGuiPopup() == nullptr && option->HasFocus(),
			"enum escape cancels highlight");
		option->OpenOptions();
		Button*    lastOption = Find<Button>(root->GetGuiPopup(), 2);
		InputEvent optionMouse;
		optionMouse.m_type        = InputEventType::MouseButton;
		optionMouse.m_mouseButton = 0;
		optionMouse.m_position    = lastOption->GetGlobalPosition() + Vector2(4.f, 4.f);
		optionMouse.m_pressed     = true;
		root->ProcessGuiInputEvent(optionMouse);
		optionMouse.m_pressed = false;
		root->ProcessGuiInputEvent(optionMouse);
		check(
			option->GetValue() == 20 && root->GetGuiPopup() == nullptr,
			"enum mouse selects saved value rather than index");
		check(!option->SetOptions("Broken:1.5"), "invalid enum hint disables selection");
		option->SetOptions("Idle:0,Walk:10,Run:20");
		option->OpenOptions();
		option->QueueFree();
		tree.FlushPendingNode();
		check(root->GetGuiPopup() == nullptr, "destroying enum owner closes popup safely");
		{
			PropertyInfo
				integer(Variant::Type::Int, "mode", PropertyInfo::Hint::None, "", PropertyInfo::UsageFlags::Default);
			std::unique_ptr<InspectorProperty> ordinary(InspectorProperty::Create(integer, "ordinary", {}));
			integer.m_hint     = PropertyInfo::Hint::Enum;
			integer.m_hintData = "Idle:0,Run:20";
			std::unique_ptr<InspectorProperty> enumeration(InspectorProperty::Create(integer, "enumeration", {}));
			check(
				dynamic_cast<InspectorPropertyInt*>(ordinary.get()) != nullptr
					&& dynamic_cast<InspectorPropertyEnum*>(enumeration.get()) != nullptr,
				"factory distinguishes plain and enum integers");
		}

		Variant vectorValue(Vector3(1.f, 2.f, 3.f));
		auto    makeField = [&](Variant::Type      type,
								char const*        name,
								PropertyInfo::Hint hint = PropertyInfo::Hint::None,
								char const*        data = "")
		{
			auto field = std::unique_ptr<InspectorProperty>(InspectorProperty::Create(
				PropertyInfo(type, name, hint, data, PropertyInfo::UsageFlags::Default),
				name,
				[&](Variant const& value)
				{
					vectorValue = value;
				}));
			field->SetValueReader(
				[&]()
				{
					return vectorValue;
				});
			root->AddNode(field->GetControl());
			field->UpdateValue(vectorValue);
			return field;
		};
		{
			auto  field = makeField(Variant::Type::Vector3, "position");
			auto* x     = Find<SpinBox>(field->GetControl(), 0);
			Draft(root, x->GetLineEdit(), "5.25");
			vectorValue = Variant(Vector3(1.f, 7.f, 8.f));
			Key(root, GLFW_KEY_ENTER);
			check(
				vectorValue.As<Vector3>() == Vector3(5.25f, 7.f, 8.f),
				"component commit preserves newest other components");
			Draft(root, x->GetLineEdit(), "6");
			field->EndEditing();
			check(vectorValue.As<Vector3>() == Vector3(5.25f, 7.f, 8.f), "component interruption cancels");
		}

		{
			vectorValue = Variant(Vector2(1.f, 2.f));
			auto field  = makeField(Variant::Type::Vector2, "size");
			Draft(root, Find<SpinBox>(field->GetControl(), 1)->GetLineEdit(), "-4.5");
			Key(root, GLFW_KEY_ENTER);
			check(vectorValue.As<Vector2>() == Vector2(1.f, -4.5f), "Vector2 edits preserve other component");
		}

		{
			vectorValue = Variant(Vector3(1.f, 2.f, 3.f));
			auto field  = makeField(Variant::Type::Vector3, "scale");
			Draft(root, Find<SpinBox>(field->GetControl())->GetLineEdit(), "0");
			Key(root, GLFW_KEY_ENTER);
			check(vectorValue.As<Vector3>() == Vector3(1e-5f, 2.f, 3.f), "scale retains near-zero protection");
		}

		{
			vectorValue = Variant(EulerAngles(30.f, 20.f, 10.f));
			auto field  = makeField(Variant::Type::EulerAngles, "rotation");
			Draft(root, Find<SpinBox>(field->GetControl())->GetLineEdit(), "-90");
			Key(root, GLFW_KEY_ENTER);
			EulerAngles const v = vectorValue.As<EulerAngles>();
			check(
				v.m_rollDegrees == -90.f && v.m_yawDegrees == 30.f && v.m_pitchDegrees == 20.f,
				"Euler degree order preserved");
		}

		{
			vectorValue = Variant(Color(10, 20, 30, 40));
			auto field  = makeField(Variant::Type::Color, "color");
			Draft(root, Find<SpinBox>(field->GetControl(), 3)->GetLineEdit(), "255");
			Key(root, GLFW_KEY_ENTER);
			check(vectorValue.As<Color>() == Color(10, 20, 30, 255), "RGBA edits one byte");
			Draft(root, Find<SpinBox>(field->GetControl())->GetLineEdit(), "200");
			Key(root, GLFW_KEY_ESCAPE);
			check(vectorValue.As<Color>() == Color(10, 20, 30, 255), "RGBA cancellation preserves all bytes");
		}

		{
			vectorValue    = Variant(false);
			auto    field  = makeField(Variant::Type::Bool, "enabled");
			Button* button = Find<Button>(field->GetControl());
			button->GrabFocus();
			Key(root, GLFW_KEY_SPACE);
			InputEvent release;
			release.m_type    = InputEventType::Key;
			release.m_keyCode = GLFW_KEY_SPACE;
			root->ProcessGuiInputEvent(release);
			check(vectorValue.As<bool>(), "boolean uses native button state");
			field->UpdateValue(Variant(false));
			check(vectorValue.As<bool>() && !button->IsPressed(), "boolean refresh does not emit changes");
		}

		{
			vectorValue     = Variant("res://old.tscn");
			auto      field = makeField(Variant::Type::String, "path", PropertyInfo::Hint::Path);
			LineEdit* edit  = Find<LineEdit>(field->GetControl());
			Draft(root, edit, "bad-path");
			Key(root, GLFW_KEY_ENTER);
			check(
				vectorValue.As<String>() == String("res://old.tscn") && edit->GetText() == String("res://old.tscn"),
				"invalid path restores actual value");
			Draft(root, edit, "res://new.tscn");
			Key(root, GLFW_KEY_ENTER);
			check(vectorValue.As<String>() == String("res://new.tscn"), "path commits through existing parser");
		}

		for (int byte = 0; byte <= 255; ++byte)
		{
			check(
				DenormalizeByte(NormalizeByte(static_cast<unsigned char>(byte))) == byte,
				"RGBA normalized byte round trip");
		}

		nlohmann::ordered_json json;
		Variant                decoded;
		check(
			VariantJson::TrySerialize(Variant(Vector2(1.25f, -2.5f)), json)
				&& VariantJson::TryDeserialize(json, Variant::Type::Vector2, decoded)
				&& decoded.As<Vector2>() == Vector2(1.25f, -2.5f),
			"Vector2 JSON round trip");

		{
			auto* clipped = MemNew<Control>();
			clipped->SetPosition(Vector2(400.f, 50.f));
			clipped->UpdateSize(Vector2(80.f, 30.f));
			clipped->SetClipContents(true);
			root->AddNode(clipped);
			clipped->UpdateSize(Vector2(80.f, 30.f));
			auto* button = MemNew<Button>();
			button->SetPosition(Vector2(0.f, 45.f));
			button->UpdateSize(Vector2(80.f, 30.f));
			clipped->AddNode(button);
			int clicks = 0;
			button->SetPressedCallback(
				[&]()
				{
					++clicks;
				});
			InputEvent mouse;
			mouse.m_type        = InputEventType::MouseButton;
			mouse.m_mouseButton = 0;
			mouse.m_position    = Vector2(410.f, 105.f);
			mouse.m_pressed     = true;
			root->ProcessGuiInputEvent(mouse);
			mouse.m_pressed = false;
			root->ProcessGuiInputEvent(mouse);
			check(clicks == 0, "clipped descendants cannot receive mouse input");
			clipped->QueueFree();
		}

		{
			auto* viewport = MemNew<SubViewport>();
			root->AddNode(viewport);
			viewport->UpdateResolution(IntVec2(64, 64));
			auto* background = MemNew<ColorRect>();
			background->SetColor(Color::Blue);
			viewport->AddNode(background);
			auto* clip = MemNew<Control>();
			clip->SetPosition(Vector2(10.f, 10.f));
			clip->SetClipContents(true);
			viewport->AddNode(clip);
			clip->UpdateSize(Vector2(20.f, 20.f));
			Ref<TextureResource> texture = CreateRef<TextureResource>();
			texture->m_image             = CreateRef<Image>();
			texture->m_image->LoadFromRGBA8(IntVec2(2, 1), { 255, 0, 0, 255, 0, 255, 0, 255 });
			texture->InitGPUResources();
			auto* picture = MemNew<TextureRect>();
			picture->SetTexture(texture);
			picture->SetExpandMode(TextureRect::ExpandMode::IgnoreSize);
			clip->AddNode(picture);
			picture->UpdateSize(Vector2(40.f, 20.f));
			engine.BeginFrame();
			engine.m_renderServer->Render();
			GPUTexture* output = engine.m_renderServer->ViewportGetTexture(viewport->GetViewportRID());
			check(
				ReadPixel(output, 25, 15).r == 255 && ReadPixel(output, 35, 15).b == 255,
				"GPU clipping preserves right-edge texture UV and outside pixels");
			engine.EndFrame();
			picture->SetPosition(Vector2(-20.f, 0.f));
			engine.BeginFrame();
			engine.m_renderServer->Render();
			check(
				ReadPixel(output, 15, 15).g == 255 && ReadPixel(output, 5, 15).b == 255,
				"GPU clipping preserves left-edge texture UV");
			engine.EndFrame();
			picture->SetHorizontalFlip(true);
			engine.BeginFrame();
			engine.m_renderServer->Render();
			check(ReadPixel(output, 15, 15).r == 255, "GPU clipping preserves flipped texture UV");
			engine.EndFrame();
			viewport->QueueFree();
		}

		{
			auto* probe = MemNew<FieldProbe>();
			probe->SetName("FieldProbe");
			root->AddNode(probe);
			EditorSelection selection;
			selection.SetSelected(probe->GetObjectID());
			EditorUIContext context;
			context.m_sceneTree = &tree;
			context.m_selection = &selection;
			InspectorPanel panel;
			auto           frame = [&]()
			{
				engine.BeginFrame();
				panel.GetHost().BeginFrame();
				panel.RenderNative(context, Vector2::Zero, Vector2(320.f, 640.f));
				panel.GetHost().EndFrame();
				panel.FinishInput(context);
				engine.m_renderServer->Render();
				engine.EndFrame();
				tree.FlushPendingNode();
			};
			frame();
			frame();
			auto* nativeRoot = panel.GetHost().GetViewport();
			auto* number     = Find<SpinBox>(panel.GetHost().GetContent());
			Draft(nativeRoot, number->GetLineEdit(), "50");
			Key(nativeRoot, GLFW_KEY_ENTER);
			check(
				probe->m_number == 10.f && number->GetValue().As<float>() == 10.f,
				"Inspector setter clamp reads actual value back");
			Draft(nativeRoot, number->GetLineEdit(), "-2");
			InputEvent outside;
			outside.m_type        = InputEventType::MouseButton;
			outside.m_pressed     = true;
			outside.m_mouseButton = 0;
			outside.m_position    = Vector2(-20.f, -20.f);
			panel.GetHost().HandleInput(outside);
			selection.SetSelected(ObjectID::Invalid);
			panel.FinishInput(context);
			check(probe->m_number == 10.f, "selection switch discards pending outside blur");
			selection.SetSelected(probe->GetObjectID());
			frame();
			probe->SetLocalPosition(Vector3(4.f, 5.f, 6.f));
			Ref<PackedScene> packed = CreateRef<PackedScene>();
			check(
				packed->Pack(probe) && ResourceSaver::Save("res://roundtrip.tscn", packed),
				"edited object scene saves");
			Ref<PackedScene> loaded   = ResourceLoader::Load("res://roundtrip.tscn");
			Node*            instance = loaded.IsValid() ? loaded->Instantiate() : nullptr;
			auto*            restored = dynamic_cast<FieldProbe*>(instance);
			check(
				restored != nullptr && restored->GetLocalPosition() == Vector3(4.f, 5.f, 6.f)
					&& restored->m_number == 10.f,
				"scene reopens with actual typed values");
			MemDelete(instance);
			{
				vectorValue = Variant(static_cast<Object*>(nullptr));
				auto resource =
					makeField(Variant::Type::ObjectPtr, "resource", PropertyInfo::Hint::ResourceType, "FontResource");
				check(
					!resource->CanAssignResource("res://roundtrip.tscn")
						&& !resource->AssignResource("res://roundtrip.tscn") && vectorValue.As<Object*>() == nullptr,
					"wrong resource type rejected");
				GuiDragData const data { "file", ObjectID::Invalid, "res://roundtrip.tscn" };
				check(!Find<LineEdit>(resource->GetControl())->CanDropData(Vector2::Zero, data),
					"native resource drop rejects incompatible class");
			}

			{
				auto resource =
					makeField(Variant::Type::ObjectPtr, "resource", PropertyInfo::Hint::ResourceType, "PackedScene");
				check(
					resource->AssignResource("res://roundtrip.tscn")
						&& dynamic_cast<PackedScene*>(vectorValue.As<Object*>()) != nullptr,
					"matching resource assigns actual resource");
				GuiDragData const data { "file", ObjectID::Invalid, "res://roundtrip.tscn" };
				LineEdit* field = Find<LineEdit>(resource->GetControl());
				vectorValue = Variant(static_cast<Object*>(nullptr));
				check(field->CanDropData(Vector2::Zero, data), "native resource field accepts matching class");
				field->DropData(Vector2::Zero, data);
				check(dynamic_cast<PackedScene*>(vectorValue.As<Object*>()) != nullptr,
					"native resource drop follows property value callback");
			}

			probe->QueueFree();
			panel.FinishInput(context);
			frame();
			check(
				panel.GetHost().GetViewport()->GetGuiFocus() == nullptr,
				"deleted selection loses native focus safely");

			ProjectSettingsPopup popup;
			Ref<ProjectSettings> settings = ProjectSettings::Get();
			settings->m_startScenePath    = "res://old.tscn";
			auto popupFrame               = [&]()
			{
				engine.BeginFrame();
				popup.PrepareFields(context);
				engine.m_renderServer->Render();
				engine.EndFrame();
			};
			popup.Open();
			popupFrame();
			auto* pathEdit = Find<LineEdit>(popup.GetHost().GetContent());
			pathEdit->SetText("res://new.tscn");
			pathEdit->GrabFocus();
			popup.GetHost().GetViewport()->SetGuiFocus(nullptr);
			check(
				settings->m_startScenePath == VirtualPath("res://old.tscn"),
				"Start Scene blur does not save settings");
			popup.Cancel();
			check(settings->m_startScenePath == VirtualPath("res://old.tscn"), "Start Scene cancel discards draft");
			popup.Open();
			popupFrame();
			pathEdit->SetText("not-a-resource-path");
			check(
				!popup.Confirm() && !popup.GetError().empty()
					&& settings->m_startScenePath == VirtualPath("res://old.tscn"),
				"invalid Start Scene keeps old settings");
			pathEdit->SetText("res://new.tscn");
			std::filesystem::path const settingsFile = projectPath / "project.ming";
			std::filesystem::path const backup       = projectPath / "project.ming.backup";
			std::filesystem::rename(settingsFile, backup);
			std::filesystem::create_directory(settingsFile);
			check(
				!popup.Confirm() && !popup.GetError().empty() && popup.IsOpen()
					&& settings->m_startScenePath == VirtualPath("res://old.tscn")
					&& pathEdit->GetText() == String("res://new.tscn"),
				"failed save restores memory and preserves error and draft");
			std::filesystem::remove(settingsFile);
			std::filesystem::rename(backup, settingsFile);
			check(
				popup.Confirm() && settings->m_startScenePath == VirtualPath("res://new.tscn"),
				"Start Scene save retries successfully");
		}
	}

	// 1) Exercise the production Window -> EditorUI -> Inspector input and destruction path.
	{
		check(
			ImGui::GetCurrentContext() == nullptr && engine.m_imguiSystem == nullptr,
			"native editor has no ImGui context or system");
		SceneTree tree;
		auto*     editor = MemNew<EditorNode>();
		tree.GetRoot()->AddNode(editor);
		check(editor->LoadScene("res://roundtrip.tscn"), "production editor loads scene");
		tree.FlushPendingNode();
		auto* probe = dynamic_cast<FieldProbe*>(tree.GetScene());
		editor->GetSelection().SetSelected(probe->GetObjectID());
		auto frame = [&]()
		{
			engine.BeginFrame();
			tree.UpdateScene(0.016f);
			engine.m_renderServer->Render();
			engine.EndFrame();
			tree.FlushPendingNode();
		};
		frame();
		frame();
		auto* number = Find<SpinBox>(editor);
		check(number != nullptr, "production Inspector owns persistent native controls");
		if (number != nullptr)
		{
			Vector2 const  origin  = Vector2::Zero;
			LineEdit*      edit    = number->GetLineEdit();
			ObjectID const fieldID = edit->GetObjectID();
			InputEvent     mouse;
			mouse.m_type        = InputEventType::MouseButton;
			mouse.m_mouseButton = 0;
			mouse.m_position    = origin + edit->GetGlobalPosition() + Vector2(5.f, 5.f);
			mouse.m_pressed     = true;
			auto* window        = static_cast<Window*>(tree.GetRoot());
			window->OnInputEvent(mouse);
			mouse.m_pressed = false;
			window->OnInputEvent(mouse);
			check(edit->HasFocus(), "Window forwards pointer into native Inspector");
			InputEvent key;
			key.m_type      = InputEventType::Key;
			key.m_keyCode   = GLFW_KEY_A;
			key.m_pressed   = true;
			key.m_modifiers = GLFW_MOD_CONTROL;
			window->OnInputEvent(key);
			for (char character : std::string("-5.5"))
			{
				InputEvent text;
				text.m_type      = InputEventType::Text;
				text.m_codepoint = character;
				window->OnInputEvent(text);
			}

			frame();
			check(
				edit->GetText() == String("-5.5") && edit->GetObjectID() == fieldID && probe->m_number == 10.f,
				"production refresh preserves uncommitted draft");
			key.m_modifiers = 0;
			key.m_keyCode   = GLFW_KEY_ENTER;
			window->OnInputEvent(key);
			check(
				probe->m_number == -5.5f && editor->IsSceneDirty(),
				"production native input changes object and scene dirty state");
			check(editor->SaveScene() && !editor->IsSceneDirty(), "production scene saves after native edit");
			Ref<PackedScene> saved    = ResourceLoader::LoadUncached("res://roundtrip.tscn");
			Node*            restored = saved->Instantiate();
			check(dynamic_cast<FieldProbe*>(restored)->m_number == -5.5f, "fresh disk load preserves native edit");
			MemDelete(restored);
		}
	}

	// 1) Verify native editor drag business rules and persistent docking.
	{
		std::filesystem::path const fixture = projectPath / "DragDockFixture";
		std::filesystem::create_directories(fixture / "source" / "child");
		std::filesystem::create_directories(fixture / "target");
		std::filesystem::remove(fixture / "target" / "move.txt");
		std::filesystem::remove(fixture / "target" / "move.txt.import");
		std::ofstream(fixture / "source" / "move.txt") << "drag fixture";
		std::ofstream(fixture / "source" / "move.txt.import") << "import fixture";
		std::ofstream(fixture / "source" / "conflict.txt") << "source";
		std::ofstream(fixture / "target" / "conflict.txt") << "target";
		SceneTree tree;
		auto* editor = MemNew<EditorNode>();
		tree.GetRoot()->AddNode(editor);
		auto* root = static_cast<Viewport*>(tree.GetRoot());
		auto* window = static_cast<Window*>(root);
		root->UpdateResolution(IntVec2(1200, 760));
		auto* scene = MemNew<Node3D>();
		auto* source = MemNew<Node3D>();
		auto* child = MemNew<Node3D>();
		auto* target = MemNew<Node3D>();
		scene->SetName("DragRoot");
		source->SetName("DragSource");
		child->SetName("DragChild");
		target->SetName("DragTarget");
		scene->AddNode(source);
		source->AddNode(child);
		scene->AddNode(target);
		source->SetLocalPosition(Vector3(2.f, 3.f, 4.f));
		target->SetLocalPosition(Vector3(10.f, 20.f, 30.f));
		tree.ChangeScene(scene);
		tree.FlushPendingNode();
		auto frame = [&]()
		{
			engine.BeginFrame();
			tree.UpdateScene(0.016f);
			engine.m_renderServer->Render();
			engine.EndFrame();
			tree.FlushPendingNode();
		};
		EditorUI* ui = editor->m_editorUI;
		ui->ResetNativeLayout();
		frame();
		check(!ui->GetDockSlot(0)->GetPage(0)->GetSerializable()
			&& !ui->GetDockSlot(0)->GetPage(1)->GetSerializable() && !ui->GetDockSlot(2)->GetPage(0)->GetSerializable(),
			"dock panel roots remain explicitly nonserializable");
		auto pointer = [&](InputEventType type, Vector2 position, bool pressed = false)
		{
			InputEvent event;
			event.m_type = type;
			event.m_position = position;
			event.m_mouseButton = GLFW_MOUSE_BUTTON_LEFT;
			event.m_pressed = pressed;
			window->OnInputEvent(event);
		};
		auto drag = [&](Vector2 from, Vector2 to)
		{
			pointer(InputEventType::MouseButton, from, true);
			pointer(InputEventType::MouseMotion, from + Vector2(12.f, 0.f));
			pointer(InputEventType::MouseMotion, to);
			pointer(InputEventType::MouseButton, to);
			frame();
		};
		Tree* nodes = ui->GetSceneTreeControl();
		GuiDragData nodeData { "node", source->GetObjectID(), "DragSource" };
		check(!nodes->CanDropData(Vector2(60.f, 42.f), nodeData), "node cannot drop onto itself");
		check(!nodes->CanDropData(Vector2(60.f, 70.f), nodeData), "node cannot drop into descendant");
		GuiDragData rootData { "node", scene->GetObjectID(), "DragRoot" };
		check(!nodes->CanDropData(Vector2(60.f, 98.f), rootData), "scene root cannot be reparented");
		check(nodes->CanDropData(Vector2(60.f, 98.f), nodeData), "native node target accepts valid reparent");
		Vector3 const world = source->GetWorldPosition();
		drag(nodes->GetGlobalPosition() + Vector2(80.f, 42.f), nodes->GetGlobalPosition() + Vector2(80.f, 98.f));
		check(source->GetParent() == target && source->GetWorldPosition() == world,
			"real pointer drag reparents node and preserves world transform");
		check(editor->IsSceneDirty() && editor->GetSelection().GetSelected() == source->GetObjectID(),
			"node drop preserves selection and marks edited scene dirty");
		ui->GetTabs()->SetCurrentTab(1);
		frame();
		Tree* files = ui->GetFileTreeControl();
		auto filePosition = [&](char const* path)
		{
			for (int row = 0; row < static_cast<int>(files->GetInternalMinimumSize().y / 28.f); ++row)
			{
				Vector2 const position(110.f, row * 28.f + 14.f);
				GuiDragData data;
				if (files->GetDragData(position, data) && data.m_value == String(path))
				{
					return position;
				}
			}

			return Vector2(-1.f, -1.f);
		};
		Vector2 const targetPosition = filePosition("res://DragDockFixture/target");
		check(targetPosition.y >= 0.f, "native file rows expose stable paths");
		GuiDragData fileData { "file", ObjectID::Invalid, "res://DragDockFixture/source/move.txt" };
		GuiDragData folderData { "file", ObjectID::Invalid, "res://DragDockFixture/source" };
		check(files->CanDropData(targetPosition, fileData), "native file target accepts valid move");
		check(!files->CanDropData(filePosition("res://DragDockFixture/source/child"), folderData),
			"native folder cannot move into descendant");
		check(!files->CanDropData(filePosition("res://DragDockFixture/source"), folderData), "native folder cannot move into itself");
		GuiDragData conflict { "file", ObjectID::Invalid, "res://DragDockFixture/source/conflict.txt" };
		check(!files->CanDropData(targetPosition, conflict), "native file drop rejects name collision");
		check(!files->CanDropData(filePosition("res://DragDockFixture/source"), fileData), "native file drop rejects existing parent");
		files->DropData(targetPosition, fileData);
		check(std::filesystem::exists(fixture / "target" / "move.txt")
			&& std::filesystem::exists(fixture / "target" / "move.txt.import")
			&& !std::filesystem::exists(fixture / "source" / "move.txt"), "native file drop moves content and import metadata");
		Draft(root, Find<LineEdit>(ui->GetTabs()->GetPage(1)), "target");
		Key(root, GLFW_KEY_ENTER);
		frame();
		Vector2 const fromFile = files->GetGlobalPosition() + filePosition("res://DragDockFixture/target");
		pointer(InputEventType::MouseButton, fromFile, true);
		pointer(InputEventType::MouseMotion, fromFile + Vector2(12.f, 0.f));
		Vector2 const sceneTabPosition = ui->GetTabs()->GetTabButton(0)->GetGlobalPosition() + Vector2(20.f, 14.f);
		pointer(InputEventType::MouseMotion, sceneTabPosition);
		glfwSetTime(glfwGetTime() + 0.6);
		frame();
		check(root->IsGuiDragging() && ui->GetTabs()->GetCurrentTab() == 0 && !files->IsVisibleInHierarchy(),
			"hover switches tab after half a second while preserving drag from hidden page");
		Key(root, GLFW_KEY_ESCAPE);
		Draft(root, Find<LineEdit>(ui->GetTabs()->GetPage(1)), "");
		Key(root, GLFW_KEY_ENTER);
		ui->ResetNativeLayout();
		frame();
		ObjectID const pageID = ui->GetDockSlot(0)->GetPage(1)->GetObjectID();
		Button* tab = ui->GetDockSlot(0)->GetTabButton(1);
		Vector2 const dockTarget = ui->GetDockSlot(2)->GetGlobalPosition() + Vector2(70.f, 180.f);
		drag(tab->GetGlobalPosition() + Vector2(20.f, 14.f), dockTarget);
		check(ui->GetDockSlot(2)->GetPageCount() == 2 && ui->GetDockSlot(2)->GetCurrentPage()->GetObjectID() == pageID,
			"pointer dock transfer preserves panel instance and selects it");
		ui->MoveNativePanel("FileSystem", 3);
		frame();
		check(ui->GetDockSlot(3)->IsVisible() && ui->GetDockSlot(3)->GetPage(0)->GetObjectID() == pageID,
			"preset lower slot expands without recreating panel");
		SplitContainer* split = ui->GetDockSplit(3);
		Vector2 const divider = split->GetGlobalPosition() + Vector2(30.f, split->GetSplitOffset() + 3.f);
		pointer(InputEventType::MouseButton, divider, true);
		pointer(InputEventType::MouseMotion, divider + Vector2(0.f, 10000.f));
		pointer(InputEventType::MouseButton, divider + Vector2(0.f, 10000.f));
		frame();
		check(!root->HasGuiMouseCapture() && ui->GetDockSlot(3)->GetSize().y >= 80.f,
			"split drag clamps panel minimum and releases outside window");
		ui->GetDockSplit(0)->SetSplitRatio(0.3f);
		check(ui->SaveNativeLayout(), "native dock layout saves");
		std::string savedLayout;
		engine.m_fileSystem->ReadText("res://.ming/Editor/NativeEditorLayout.json", savedLayout);
		ui->MoveNativePanel("Scene", 2);
		engine.m_fileSystem->WriteText("res://.ming/Editor/NativeEditorLayout.json", savedLayout);
		ui->LoadNativeLayout();
		frame();
		check(ui->GetDockSlot(3)->GetPageCount() == 1 && ui->GetDockSlot(0)->GetPageTitle(0) == String("Scene"),
			"layout reload restores panel locations");
		check(std::abs(ui->GetDockSplit(0)->GetSplitRatio() - 0.3f) < 0.001f, "layout reload restores split ratio");
		engine.m_fileSystem->WriteText("res://.ming/Editor/NativeEditorLayout.json",
			R"({"version":1,"slots":[{"panels":["FileSystem","FileSystem","Unknown"],"current":"FileSystem"}],"ratios":[-10,20,"bad"]})");
		ui->LoadNativeLayout();
		frame();
		check(ui->GetDockSlot(0)->GetPageCount() == 2 && ui->GetDockSlot(2)->GetPageCount() == 1,
			"layout ignores duplicate and unknown panels and restores missing panels");
		check(ui->GetDockSplit(0)->GetSplitRatio() == 0.f && ui->GetDockSplit(1)->GetSplitRatio() == 1.f,
			"layout clamps out-of-range ratios");
		engine.m_fileSystem->WriteText("res://.ming/Editor/NativeEditorLayout.json", "{invalid json");
		ui->LoadNativeLayout();
		check(ui->GetDockSlot(0)->GetPageTitle(0) == String("Scene") && !ui->GetDockSlot(3)->IsVisible(),
			"invalid layout restores default without losing panels");
		ui->MoveNativePanel("FileSystem", 3);
		std::filesystem::path const layoutFile = projectPath / ".ming" / "Editor" / "NativeEditorLayout.json";
		std::filesystem::remove(layoutFile);
		std::filesystem::create_directory(layoutFile);
		check(!ui->SaveNativeLayout() && root->GetGuiPopup() != nullptr && ui->GetDockSlot(3)->GetPageCount() == 1,
			"layout write failure reports error and preserves live layout");
		Key(root, GLFW_KEY_ESCAPE);
		std::filesystem::remove(layoutFile);
		ui->ResetNativeLayout();
		frame();
		check(ui->GetDockSlot(0)->GetPageCount() == 2 && ui->GetDockSlot(2)->GetPageCount() == 1
			&& !ui->GetDockSlot(1)->IsVisible() && !ui->GetDockSlot(3)->IsVisible(), "reset restores default layout and collapses empty slots");
		for (int i = 0; i < 3; ++i)
		{
			ui->GetTabs()->MovePage(0, 1);
			tree.FlushContainerUpdate();
			Button* button = ui->GetTabs()->GetTabButton(0);
			Vector2 const position = button->GetGlobalPosition() + Vector2(15.f, 15.f);
			pointer(InputEventType::MouseButton, position, true);
			pointer(InputEventType::MouseButton, position);
			check(ui->GetTabs()->GetCurrentPage() == ui->GetTabs()->GetPage(0), "reordered tab button resolves current page identity");
		}

		ui->ResetNativeLayout();
		ui->MoveNativePanel("FileSystem", 3);
		ui->GetDockSplit(0)->SetSplitRatio(0.31f);
		check(ui->SaveNativeLayout(), "restart fixture layout saves");
	}

	{
		SceneTree tree;
		auto* editor = MemNew<EditorNode>();
		tree.GetRoot()->AddNode(editor);
		EditorUI* ui = editor->m_editorUI;
		check(ui->GetDockSlot(3)->GetPageTitle(0) == String("FileSystem")
			&& ui->GetDockSlot(0)->GetPageCount() == 1 && std::abs(ui->GetDockSplit(0)->GetSplitRatio() - 0.31f) < 0.001f,
			"new editor instance restores panel identities and split ratio from disk");
		ui->ResetNativeLayout();
	}

	// 1) Verify native trees, tabs, scrolling, viewport sampling, and a real mesh editing flow.
	{
		SceneTree tree;
		auto*     editor = MemNew<EditorNode>();
		tree.GetRoot()->AddNode(editor);
		auto* root = static_cast<Viewport*>(tree.GetRoot());
		root->UpdateResolution(IntVec2(1200, 760));
		auto* window = static_cast<Window*>(root);
		auto  frame  = [&]()
		{
			engine.BeginFrame();
			tree.UpdateScene(0.016f);
			engine.m_renderServer->Render();
			engine.EndFrame();
			tree.FlushPendingNode();
		};
		auto click = [&](Vector2 position)
		{
			InputEvent event;
			event.m_type        = InputEventType::MouseButton;
			event.m_mouseButton = 0;
			event.m_position    = position;
			event.m_pressed     = true;
			window->OnInputEvent(event);
			event.m_pressed = false;
			window->OnInputEvent(event);
		};
		auto text = [&](char const* value)
		{
			Key(root, GLFW_KEY_A, GLFW_MOD_CONTROL);
			Key(root, GLFW_KEY_BACKSPACE);
			for (char const* c = value; *c != '\0'; ++c)
			{
				InputEvent event;
				event.m_type      = InputEventType::Text;
				event.m_codepoint = static_cast<unsigned char>(*c);
				window->OnInputEvent(event);
			}
		};
		editor->GetEditorCamera()->SetLocalPosition(Vector3(5.f, 5.f, 7.071068f));
		auto* scene = MemNew<Node3D>();
		scene->SetName("NativeEditorDemo");
		std::vector<Vertex> vertices;
		AddVertsForAABB3D(
			vertices,
			AABB3(Vector3(-0.7f, -0.7f, -0.7f), Vector3(0.7f, 0.7f, 0.7f)),
			Color(90, 170, 235));
		SurfaceTool surface;
		for (Vertex const& vertex : vertices)
		{
			surface.SetColor(vertex.m_color);
			surface.SetUV(vertex.m_uv);
			surface.AddVertex(vertex.m_position);
		}

		surface.GenerateNormals();
		surface.Index();
		auto mesh = surface.CreateMeshResource();
		mesh->SetVirtualPath("res://demo.mesh");
		check(ResourceSaver::Save("res://demo.mesh", mesh), "native demo mesh saves");
		auto* cube = MemNew<MeshInstance3D>();
		cube->SetName("BlueCube");
		cube->SetMeshResource(mesh);
		scene->AddNode(cube);
		auto* light = MemNew<DirectionalLight3D>();
		light->SetName("Sun");
		light->SetLocalOrientation(EulerAngles(45.f, 45.f, 0.f));
		light->SetIntensity(2.f);
		scene->AddNode(light);
		auto* group = MemNew<Node3D>();
		group->SetName("Group");
		scene->AddNode(group);
		auto* leaf = MemNew<Node3D>();
		leaf->SetName("Needle_very_long_node_name_for_horizontal_scrolling_0123456789");
		group->AddNode(leaf);
		for (int i = 0; i < 40; ++i)
		{
			auto* node = MemNew<Node3D>();
			node->SetName("Item_" + std::to_string(i));
			scene->AddNode(node);
		}

		tree.ChangeScene(scene);
		tree.FlushPendingNode();
		auto packed = CreateRef<PackedScene>();
		check(packed->Pack(scene) && ResourceSaver::Save("res://native-demo.tscn", packed), "native demo scene saves");
		check(editor->LoadScene("res://native-demo.tscn"), "native demo reloads");
		tree.FlushPendingNode();
		scene = dynamic_cast<Node3D*>(tree.GetScene());
		cube  = dynamic_cast<MeshInstance3D*>(scene->FindChildByName("BlueCube"));
		group = dynamic_cast<Node3D*>(scene->FindChildByName("Group"));
		leaf  = dynamic_cast<Node3D*>(group->GetChildren()[0]);
		check(
			scene->GetParent() == tree.GetSceneViewport() && tree.GetSceneViewport() != root,
			"edited scene mounts in independent SubViewport");
		frame();
		frame();
		EditorUI* ui       = editor->m_editorUI;
		ObjectID const editorPageID = ui->GetTabs()->GetPage(0)->GetObjectID();
		Tree*     nodes    = ui->GetSceneTreeControl();
		Button*   sceneTab = Find<Button>(ui->GetTabs());
		click(sceneTab->GetGlobalPosition() + Vector2(12.f, 12.f));
		check(
			sceneTab->IsPressed() && ui->GetTabs()->GetCurrentTab() == 0,
			"clicking active tab preserves selection highlight");

		auto key = [](Node* node)
		{
			ObjectID const id = node->GetObjectID();
			return (static_cast<uint64_t>(id.GetUID()) << 32 | id.GetIndex()) + 1;
		};
		float const expandedHeight = nodes->GetInternalMinimumSize().y;
		nodes->SetCollapsed(key(group), true);
		frame();
		check(
			nodes->IsCollapsed(key(group)) && nodes->GetInternalMinimumSize().y < expandedHeight,
			"Tree collapse hides descendants");
		auto* filter = Find<LineEdit>(ui->GetTabs()->GetCurrentPage());
		filter->SetText("Needle");
		frame();
		check(nodes->GetInternalMinimumSize().y == 84.f, "filter retains ancestor path and expands matching branch");
		filter->SetText("");
		frame();
		check(
			nodes->IsCollapsed(key(group)) && nodes->GetInternalMinimumSize().y < expandedHeight,
			"clearing filter restores collapse state");
		editor->GetSelection().SetSelected(leaf->GetObjectID());
		frame();
		check(
			nodes->GetSelected() == key(leaf) && !nodes->IsCollapsed(key(group)),
			"external selection expands ancestors and synchronizes Tree");
		nodes->GrabFocus();
		Key(root, GLFW_KEY_UP);
		check(
			editor->GetSelection().GetSelected() == group->GetObjectID(),
			"Tree keyboard selection changes real editor selection");
		Key(root, GLFW_KEY_F2);
		auto* rename = Find<LineEdit>(nodes);
		check(rename->HasFocus(), "F2 focuses inline LineEdit");
		text("RenamedGroup");
		Key(root, GLFW_KEY_ENTER);
		frame();
		check(
			group->GetName() == String("RenamedGroup") && editor->IsSceneDirty(),
			"inline rename commits actual node name and dirty state");
		nodes->GrabFocus();
		Key(root, GLFW_KEY_F2);
		text("CancelledName");
		Key(root, GLFW_KEY_ESCAPE);
		frame();
		check(group->GetName() == String("RenamedGroup"), "inline rename Escape cancels");
		auto* scroll = dynamic_cast<ScrollContainer*>(nodes->GetParent());
		tree.FlushContainerUpdate();
		scroll->SetHorizontalScroll(500.f);
		scroll->SetScroll(500.f);
		check(scroll->GetHorizontalScroll() > 0.f && scroll->GetScroll() > 0.f, "Tree content scrolls in both axes");
		scroll->SetScroll(0.f);
		scroll->SetHorizontalScroll(0.f);
		nodes->GrabFocus();
		ui->GetTabs()->SetCurrentTab(1);
		frame();
		check(!nodes->IsVisibleInHierarchy() && !nodes->HasFocus(), "Tab switch hides old page and releases its focus");
		check(ui->GetFileTreeControl()->IsVisibleInHierarchy(), "FileSystem tab displays actual resource Tree");
		click(nodes->GetGlobalPosition() + Vector2(60.f, 12.f));
		check(!nodes->HasFocus(), "hidden Tree cannot receive pointer focus");
		ui->GetTabs()->SetCurrentTab(0);
		frame();
		editor->GetSelection().Clear();
		Vector2 const center = ui->GetViewportOrigin() + ui->GetViewportDimensions() * 0.5f;
		click(center);
		check(editor->GetSelection().GetSelected() == cube->GetObjectID(), "native viewport center picks real mesh");
		frame();
		std::filesystem::path const captures = projectPath.parent_path() / "Captures";
		std::filesystem::create_directories(captures);
		auto capture = [&](char const* name)
		{
			frame();
			SaveRenderedFrame(engine.m_renderServer->ViewportGetTexture(root->GetViewportRID()), captures / name);
		};
		capture("01-editor.ppm");
		GPUTexture* sceneTexture = engine.m_renderServer->ViewportGetTexture(tree.GetSceneViewport()->GetViewportRID());
		IntVec2 const dimensions = sceneTexture->GetDimensions();
		Color const   source     = ReadPixel(sceneTexture, dimensions.x / 2, dimensions.y / 2);
		Color const   composed   = ReadPixel(
			engine.m_renderServer->ViewportGetTexture(root->GetViewportRID()),
			static_cast<unsigned>(center.x),
			static_cast<unsigned>(center.y));
		check(
			source == composed && source != Color(47, 54, 65, 255),
			"root UI samples current rendered mesh viewport texture");
		auto* spin = Find<SpinBox>(editor);
		if (spin != nullptr)
		{
			click(spin->GetLineEdit()->GetGlobalPosition() + Vector2(8.f, 8.f));
			text("1.5");
			Key(root, GLFW_KEY_ENTER);
			frame();
			capture("02-property-edit.ppm");
		}

		Vector2 const cubePosition = cube->GetLocalPosition();
		check(editor->SaveScene() && !editor->IsSceneDirty(), "native demo saves after attribute edit");
		check(editor->LoadScene("res://native-demo.tscn"), "native demo requests reopen");
		tree.FlushPendingNode();
		scene = dynamic_cast<Node3D*>(tree.GetScene());
		cube  = dynamic_cast<MeshInstance3D*>(scene->FindChildByName("BlueCube"));
		check(
			cube != nullptr && cube->GetLocalPosition() == cubePosition
				&& scene->FindChildByName("RenamedGroup") != nullptr,
			"save and reopen preserve rename and attributes");
		check(
			scene->GetChildren().size() == 43 && scene->FindChildByName("EditorUI") == nullptr,
			"scene serialization excludes editor controls and camera");
		check(ui->GetTabs()->GetPage(0)->GetObjectID() == editorPageID,
			"scene save and reopen preserve editor dock panel instance");
		editor->GetSelection().SetSelected(cube->GetObjectID());
		capture("03-reopened.ppm");
		root->UpdateResolution(IntVec2(1100, 700));
		frame();
		check(
			tree.GetSceneViewport()->GetOutputResolution()
				== IntVec2(
					static_cast<int>(ui->GetViewportDimensions().x),
					static_cast<int>(ui->GetViewportDimensions().y)),
			"layout resize drives scene SubViewport resolution");
		Vector2 const mapped = ui->ToViewportPos(ui->GetViewportOrigin() + ui->GetViewportDimensions() * 0.5f);
		check(
			std::abs(mapped.x - tree.GetSceneViewport()->GetOutputResolution().x * 0.5f) < 1.f,
			"resized scene coordinate mapping stays aligned");
		capture("04-resized.ppm");
		ui->GetTabs()->SetCurrentTab(1);
		capture("05-filesystem.ppm");
		ui->GetTabs()->SetCurrentTab(0);
		editor->MarkSceneDirty();
		editor->RequestLoadScene("res://roundtrip.tscn");
		frame();
		check(root->GetGuiPopup() != nullptr, "unsaved scene confirmation uses native modal popup");
		ObjectID const sceneID = tree.GetScene()->GetObjectID();
		click(ui->GetViewportOrigin() + Vector2(10.f, 10.f));
		check(
			root->GetGuiPopup() != nullptr && tree.GetScene()->GetObjectID() == sceneID,
			"modal backdrop blocks scene interaction");
		capture("06-confirmation.ppm");
		Key(root, GLFW_KEY_ESCAPE);
		frame();
		check(
			tree.GetScene()->GetObjectID() == sceneID && editor->IsSceneDirty(),
			"cancel scene switch preserves dirty scene");
		InputEvent right;
		right.m_type        = InputEventType::MouseButton;
		right.m_mouseButton = GLFW_MOUSE_BUTTON_RIGHT;
		right.m_position    = ui->GetViewportOrigin() + Vector2(40.f, 40.f);
		right.m_pressed     = true;
		window->OnInputEvent(right);
		check(editor->IsSceneNavigationActive(), "camera navigation starts inside scene region");
		right.m_position = Vector2(5.f, 5.f);
		right.m_pressed  = false;
		window->OnInputEvent(right);
		check(
			!editor->IsSceneNavigationActive() && !root->HasGuiMouseCapture(),
			"camera release outside scene ends capture");
		right.m_pressed  = true;
		right.m_position = Vector2(15.f, 115.f);
		window->OnInputEvent(right);
		check(!editor->IsSceneNavigationActive(), "right click outside scene does not start camera");
		right.m_pressed = false;
		window->OnInputEvent(right);
		right.m_pressed  = true;
		right.m_position = ui->GetViewportOrigin() + Vector2(40.f, 40.f);
		window->OnInputEvent(right);
		InputEvent lost;
		lost.m_type    = InputEventType::WindowFocus;
		lost.m_focused = false;
		window->OnInputEvent(lost);
		check(
			!editor->IsSceneNavigationActive() && !root->HasGuiMouseCapture(),
			"window focus loss cancels scene navigation");
		lost.m_focused = true;
		window->OnInputEvent(lost);
		root->UpdateResolution(IntVec2(1100, 600));
		frame();
		auto* inspectorScroll =
			dynamic_cast<ScrollContainer*>(ui->GetPanel<InspectorPanel>().GetHost().GetRootControl());
		auto*      number = Find<SpinBox>(editor);
		InputEvent wheel;
		wheel.m_type     = InputEventType::MouseWheel;
		wheel.m_scroll   = Vector2(0.f, -3.f);
		wheel.m_position = number->GetLineEdit()->GetGlobalPosition() + Vector2(8.f, 8.f);
		window->OnInputEvent(wheel);
		check(inspectorScroll->GetScroll() > 0.f, "wheel over property field reaches enclosing ScrollContainer");
		editor->GetSelection().Clear();
		auto* leafTree   = ui->GetSceneTreeControl();
		auto* scrollTree = static_cast<ScrollContainer*>(leafTree->GetParent());
		scrollTree->SetScroll(0.f);
		frame();
		InputEvent bar;
		bar.m_type        = InputEventType::MouseButton;
		bar.m_mouseButton = 0;
		bar.m_pressed     = true;
		bar.m_position    = scrollTree->GetGlobalPosition() + Vector2(scrollTree->GetSize().x - 5.f, 15.f);
		window->OnInputEvent(bar);
		InputEvent motion;
		motion.m_type     = InputEventType::MouseMotion;
		motion.m_position = Vector2(800.f, 1200.f);
		window->OnInputEvent(motion);
		bar.m_pressed  = false;
		bar.m_position = motion.m_position;
		window->OnInputEvent(bar);
		check(
			scrollTree->GetScroll() > 0.f && !root->HasGuiMouseCapture(),
			"scrollbar drag releases beyond control boundary");
		click(scrollTree->GetGlobalPosition() + Vector2(60.f, scrollTree->GetSize().y + 5.f));
		check(
			!editor->GetSelection().GetSelected().IsValid(),
			"clipped Tree row outside scroll area cannot select scene nodes");
		check(editor->SaveScene(), "save resolves dirty scene before resource browsing");
		ui->GetTabs()->SetCurrentTab(1);
		auto* filesFilter = Find<LineEdit>(ui->GetTabs()->GetCurrentPage());
		filesFilter->GrabFocus();
		text("roundtrip");
		frame();
		Tree* files = ui->GetFileTreeControl();
		files->GrabFocus();
		Key(root, GLFW_KEY_DOWN);
		Key(root, GLFW_KEY_DOWN);
		ObjectID const editorRootID = editor->GetObjectID();
		Key(root, GLFW_KEY_ENTER);
		tree.FlushPendingNode();
		frame();
		check(
			dynamic_cast<FieldProbe*>(tree.GetScene()) != nullptr,
			"FileSystem keyboard activation opens selected real scene");
		check(
			editor->GetObjectID() == editorRootID && ui->GetSceneView() != nullptr,
			"scene replacement preserves editor UI lifecycle");
		tree.ClearScene();
		editor->GetSelection().Clear();
		tree.FlushPendingNode();
		frame();
		ui->Warning("Verification", "No scene is open");
		frame();
		check(root->GetGuiPopup() != nullptr, "warning modal remains available without a selected or open scene");
		Key(root, GLFW_KEY_ESCAPE);
		check(ImGui::GetCurrentContext() == nullptr, "entire native editing flow leaves ImGui uninitialized");
	}

	ResourceLoader::Shutdown();
	engine.Shutdown();
	ClassDatabase::Shutdown();
	std::printf("NATIVE_FIELDS checks=%d failures=%d\n", checks, failures);
	return failures == 0 ? 0 : 1;
}
