#pragma once

#include "MingEngine/Core/IO/ResourceLoader.hpp"
#include "MingEngine/Core/StringUtils.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"
#include "MingEngine/Scene/GUI/ColorRect.hpp"
#include "MingEngine/Scene/GUI/Label.hpp"
#include "MingEngine/Scene/GUI/Button.hpp"
#include "MingEngine/Scene/GUI/LineEdit.hpp"
#include "MingEngine/Scene/GUI/MarginContainer.hpp"
#include "MingEngine/Scene/GUI/HBoxContainer.hpp"
#include "MingEngine/Scene/GUI/VBoxContainer.hpp"
#include <memory>

inline Label* BuildUILayoutPreview(SceneTree* tree)
{
	auto   regenerateCount = std::make_shared<int>(0);
	Label* sizeLabel       = nullptr;
	// 1) Load the font and build the visual test helpers
	Ref<FontResource> font = ResourceLoader::Load("res://fusion-pixel-12px-proportional-zh_hans.ttf");
	if (!font.IsValid())
	{
		font = FontResource::CreateDefault();
		WARN_PRINT("UI layout test is using the embedded fallback font.");
	}

	auto addRect = [](Node* parent, char const* name, Color const& color, Vector2 const& minimum)
	{
		ColorRect* rect = MemNew<ColorRect>();
		rect->SetName(name);
		rect->SetColor(color);
		rect->SetCustomMinimumSize(minimum);
		parent->AddNode(rect);
		return rect;
	};

	auto addLabel = [&font](Node* parent, char const* name, char const* text, int fontSize)
	{
		Label* label = MemNew<Label>();
		label->SetName(name);
		label->SetFont(font);
		label->SetFontSize(fontSize);
		label->SetText(text);
		parent->AddNode(label);
		return label;
	};

	// 2) Build three nested margins that follow the root Viewport in both axes
	MarginContainer* example = MemNew<MarginContainer>();
	example->SetName("UIResizeTest");
	example->SetMargins(24.0f, 24.0f, 24.0f, 24.0f);
	addRect(example, "OuterBackground", Color(40, 80, 125), Vector2::Zero);

	MarginContainer* nested = MemNew<MarginContainer>();
	nested->SetName("MiddleMargin");
	nested->SetMargins(16.0f, 16.0f, 16.0f, 16.0f);
	example->AddNode(nested);
	addRect(nested, "MiddleBackground", Color(35, 125, 110), Vector2::Zero);

	MarginContainer* content = MemNew<MarginContainer>();
	content->SetName("InnerMargin");
	content->SetMargins(16.0f, 16.0f, 16.0f, 16.0f);
	nested->AddNode(content);
	addRect(content, "InnerBackground", Color(25, 30, 40), Vector2::Zero);

	VBoxContainer* column = MemNew<VBoxContainer>();
	column->SetName("Content");
	column->SetSeparation(12.0f);
	content->AddNode(column);

	// 3) Build the layout comparison header
	Label* title = addLabel(column, "Title", "Ming UI Size Flags Test", 28);
	title->SetHorizontalAlignment(HorizontalAlignment::Center);
	Label* hint = addLabel(column, "Hint", "Resize window. Dark areas show allocated rectangles.", 20);
	hint->SetHorizontalAlignment(HorizontalAlignment::Center);
	sizeLabel = addLabel(column, "ViewportSize", "Viewport", 20);
	sizeLabel->SetHorizontalAlignment(HorizontalAlignment::Center);

	// 4) Verify button activation, disabling, and independent toggles
	HBoxContainer* buttons = MemNew<HBoxContainer>();
	buttons->SetName("ButtonInteractionTest");
	buttons->SetSeparation(12.f);
	column->AddNode(buttons);

	auto addButton = [&font, buttons](char const* name, bool toggle)
	{
		Button* button = MemNew<Button>();
		button->SetName(name);
		button->SetFont(font);
		button->SetFontSize(20);
		button->SetText(name);
		button->SetToggleMode(toggle);
		buttons->AddNode(button);
		return button;
	};

	Button*        regenerate   = addButton("Regenerate", false);
	Button*        disable      = addButton("Disable Regenerate", true);
	Button*        modeA        = addButton("Mode A", true);
	Button*        modeB        = addButton("Mode B", true);
	Label*         status       = addLabel(column, "ButtonStatus", "Regenerate: 0 | Mode A: off | Mode B: off", 18);
	ObjectID const statusID     = status->GetObjectID();
	ObjectID const regenerateID = regenerate->GetObjectID();
	ObjectID const modeAID      = modeA->GetObjectID();
	ObjectID const modeBID      = modeB->GetObjectID();
	auto           updateStatus = [regenerateCount, statusID, modeAID, modeBID]()
	{
		Label*  label = ObjectDatabase::GetInstance<Label>(statusID);
		Button* a     = ObjectDatabase::GetInstance<Button>(modeAID);
		Button* b     = ObjectDatabase::GetInstance<Button>(modeBID);
		if (label != nullptr && a != nullptr && b != nullptr)
		{
			label->SetText(Stringf(
							   "Regenerate: %d | Mode A: %s | Mode B: %s",
							   *regenerateCount,
							   a->IsPressed() ? "on" : "off",
							   b->IsPressed() ? "on" : "off")
							   .c_str());
		}
	};

	regenerate->SetPressedCallback(
		[regenerateCount, updateStatus]()
		{
			++*regenerateCount;
			updateStatus();
		});
	disable->SetToggledCallback(
		[regenerateID](bool selected)
		{
			if (Button* button = ObjectDatabase::GetInstance<Button>(regenerateID))
			{
				button->SetDisabled(selected);
			}
		});
	modeA->SetToggledCallback(
		[updateStatus](bool)
		{
			updateStatus();
		});
	modeB->SetToggledCallback(
		[updateStatus](bool)
		{
			updateStatus();
		});

	// 5) Verify single-line editing, selection, scrolling, and state changes
	addLabel(column, "LineEditHint", "LineEdit: Enter applies, Esc restores, Tab/click applies nonempty text", 16);
	HBoxContainer* editRow = MemNew<HBoxContainer>();
	editRow->SetName("LineEditInteractionTest");
	editRow->SetSeparation(12.f);
	column->AddNode(editRow);

	LineEdit* firstEdit = MemNew<LineEdit>();
	firstEdit->SetName("NameInput");
	firstEdit->SetFont(font);
	firstEdit->SetFontSize(20);
	firstEdit->SetPlaceholder(U"Enter a name / \u8f93\u5165\u540d\u79f0");
	firstEdit->SetText("Player");
	firstEdit->SetHorizontalSizeFlags(Control::Size_ExpandFill);
	editRow->AddNode(firstEdit);

	String const longName   = U"Long name / \u957f\u540d\u79f0: ABCDEFGHIJKLMNOPQRSTUVWXYZ 0123456789 -- "
							  U"ABCDEFGHIJKLMNOPQRSTUVWXYZ 0123456789 -- ABCDEFGHIJKLMNOPQRSTUVWXYZ 0123456789 -- "
							  U"ABCDEFGHIJKLMNOPQRSTUVWXYZ 0123456789 -- scroll to the end";
	LineEdit*    secondEdit = MemNew<LineEdit>();
	secondEdit->SetName("LongNameInput");
	secondEdit->SetFont(font);
	secondEdit->SetFontSize(20);
	secondEdit->SetText(longName);
	secondEdit->SetHorizontalSizeFlags(Control::Size_ExpandFill);
	editRow->AddNode(secondEdit);

	// 1) Keep each field's snapshot and committed values outside LineEdit
	struct EditExample
	{
		ObjectID    m_editID;
		String      m_values[2];
		String      m_original;
		int         m_target      = 0;
		int         m_starts      = 0;
		int         m_ends        = 0;
		int         m_writes      = 0;
		char const* m_result      = "Ready";
		bool        m_focusOnEnd  = false;
		bool        m_removeOnEnd = false;
	};

	auto bindEdit = [&addLabel, column](LineEdit* edit, LineEdit* other, char const* name)
	{
		auto state              = std::make_shared<EditExample>();
		state->m_editID         = edit->GetObjectID();
		state->m_values[0]      = edit->GetText();
		state->m_values[1]      = "Enemy";
		state->m_original       = edit->GetText();
		ObjectID const statusID = addLabel(column, name, "", 14)->GetObjectID();
		ObjectID const otherID  = other->GetObjectID();
		auto           update   = [state, statusID, name]()
		{
			if (Label* label = ObjectDatabase::GetInstance<Label>(statusID))
			{
				LineEdit*    field = ObjectDatabase::GetInstance<LineEdit>(state->m_editID);
				String const draft = field != nullptr ? field->GetText() : String("<removed>");
				label->SetText(Stringf(
								   "%s[%d] before:%s draft:%s saved:%s | %s | start:%d end:%d writes:%d",
								   name,
								   state->m_target,
								   state->m_original.Substr(0, 12).ToUtf8().c_str(),
								   draft.Substr(0, 12).ToUtf8().c_str(),
								   state->m_values[state->m_target].Substr(0, 12).ToUtf8().c_str(),
								   state->m_result,
								   state->m_starts,
								   state->m_ends,
								   state->m_writes)
								   .c_str());
			}
		};
		edit->SetEditingStartedCallback(
			[state, update](String const& text)
			{
				state->m_original = text;
				++state->m_starts;
				state->m_result = "Editing";
				update();
			});
		edit->SetTextChangedCallback(
			[update](String const&)
			{
				update();
			});
		edit->SetEditingEndedCallback(
			[state, update, otherID](String const& text, LineEdit::EditEndReason reason)
			{
				++state->m_ends;
				bool const apply =
					reason == LineEdit::EditEndReason::Submitted || reason == LineEdit::EditEndReason::FocusLost;
				if (apply && !text.IsEmpty())
				{
					if (state->m_values[state->m_target] != text)
					{
						state->m_values[state->m_target] = text;
						++state->m_writes;
					}

					state->m_result = reason == LineEdit::EditEndReason::Submitted ? "Submitted" : "FocusLost";
				}
				else
				{
					state->m_result = apply                                          ? "Invalid:empty"
									  : reason == LineEdit::EditEndReason::Cancelled ? "Cancelled"
																					 : "Interrupted";
					if (LineEdit* field = ObjectDatabase::GetInstance<LineEdit>(state->m_editID))
					{
						field->SetText(state->m_original);
					}
				}

				// 2) Exercise callbacks that change focus or remove their own field
				if (state->m_focusOnEnd)
				{
					if (LineEdit* next = ObjectDatabase::GetInstance<LineEdit>(otherID))
					{
						next->GrabFocus();
					}
				}

				if (state->m_removeOnEnd)
				{
					if (LineEdit* field = ObjectDatabase::GetInstance<LineEdit>(state->m_editID))
					{
						field->QueueFree();
					}

					state->m_editID = ObjectID::Invalid;
				}

				update();
			});
		update();
		return std::make_pair(state, update);
	};

	auto           firstExample  = bindEdit(firstEdit, secondEdit, "First");
	auto           secondExample = bindEdit(secondEdit, firstEdit, "Second");
	ObjectID const editRowID     = editRow->GetObjectID();

	// 3) Interrupt fields directly without moving focus to a test button first
	char const* const editButtonNames[] = { "Read only",     "Disabled",     "Hide fields",  "Reset text",
											"Switch object", "Remove first", "Focus on end", "Remove on end" };
	HBoxContainer*    editButtons       = nullptr;
	for (int index = 0; index < 8; ++index)
	{
		if (index % 4 == 0)
		{
			editButtons = MemNew<HBoxContainer>();
			editButtons->SetName(index == 0 ? "LineEditStateTest" : "LineEditEndTest");
			editButtons->SetSeparation(12.f);
			column->AddNode(editButtons);
		}

		Button* button = MemNew<Button>();
		button->SetFont(font);
		button->SetFontSize(16);
		button->SetFocusMode(Control::FocusMode::None);
		button->SetText(editButtonNames[index]);
		button->SetToggleMode(index < 3 || index >= 6);
		editButtons->AddNode(button);
		button->SetPressedCallback(
			[firstExample, secondExample, editRowID, longName, index, buttonID = button->GetObjectID()]()
			{
				auto       state    = firstExample.first;
				LineEdit*  field    = ObjectDatabase::GetInstance<LineEdit>(state->m_editID);
				Button*    source   = ObjectDatabase::GetInstance<Button>(buttonID);
				bool const selected = source != nullptr && source->IsPressed();
				if (index == 0 && field != nullptr)
				{
					field->SetEditable(!selected);
				}
				else if (index == 1 && field != nullptr)
				{
					field->SetDisabled(selected);
				}
				else if (index == 2)
				{
					if (Control* row = ObjectDatabase::GetInstance<Control>(editRowID))
					{
						row->SetVisible(!selected);
					}
				}
				else if (index == 3 || index == 4)
				{
					if (field != nullptr)
					{
						field->EndEditing();
						field = ObjectDatabase::GetInstance<LineEdit>(state->m_editID);
						if (field != nullptr && !field->IsQueuedForDeletion())
						{
							if (index == 4)
							{
								state->m_target = 1 - state->m_target;
							}
							else
							{
								state->m_values[state->m_target] = "Player";
							}

							state->m_original = state->m_values[state->m_target];
							field->SetText(state->m_original);
						}
					}

					if (index == 3)
					{
						auto second = secondExample.first;
						if (LineEdit* other = ObjectDatabase::GetInstance<LineEdit>(second->m_editID))
						{
							other->EndEditing();
							second->m_values[second->m_target] = longName;
							second->m_original                 = longName;
							other = ObjectDatabase::GetInstance<LineEdit>(second->m_editID);
							if (other != nullptr)
							{
								other->SetText(longName);
							}
						}
					}
				}
				else if (index == 5 && field != nullptr)
				{
					field->EndEditing();
					if (LineEdit* remaining = ObjectDatabase::GetInstance<LineEdit>(state->m_editID))
					{
						remaining->QueueFree();
					}

					state->m_editID = ObjectID::Invalid;
				}
				else if (index == 6)
				{
					state->m_focusOnEnd = selected;
				}
				else if (index == 7)
				{
					state->m_removeOnEnd = selected;
				}

				firstExample.second();
				secondExample.second();
			});
	}

	// 6) Compare fill and shrink alignment inside visible allocated rectangles
	auto addAlignmentRow = [&](char const* name, bool vertical)
	{
		HBoxContainer* row = MemNew<HBoxContainer>();
		row->SetName(name);
		row->SetSeparation(12.0f);
		if (vertical)
		{
			row->SetVerticalSizeFlags(Control::Size_ExpandFill);
		}

		column->AddNode(row);

		char const* const names[] = { "Fill", "Shrink Begin", "Shrink Center", "Shrink End" };
		int const         flags[] = { Control::Size_Fill,
									  Control::Size_ShrinkBegin,
									  Control::Size_ShrinkCenter,
									  Control::Size_ShrinkEnd };
		Color const colors[] = { Color(70, 145, 235), Color(220, 80, 80), Color(80, 190, 120), Color(220, 180, 70) };
		for (int index = 0; index < 4; ++index)
		{
			VBoxContainer* group = MemNew<VBoxContainer>();
			group->SetName(names[index]);
			group->SetSeparation(4.0f);
			group->SetHorizontalSizeFlags(Control::Size_ExpandFill);
			row->AddNode(group);
			addLabel(group, "Caption", names[index], 16);

			MarginContainer* slot = MemNew<MarginContainer>();
			slot->SetName("AllocatedArea");
			slot->SetCustomMinimumSize(Vector2(96.0f, vertical ? 80.0f : 36.0f));
			slot->SetVerticalSizeFlags(Control::Size_ExpandFill);
			group->AddNode(slot);
			addRect(slot, "Background", Color(45, 55, 70), Vector2::Zero);

			ColorRect* sample = addRect(slot, "Sample", colors[index], Vector2(48.0f, 24.0f));
			sample->SetHorizontalSizeFlags(vertical ? Control::Size_ShrinkCenter : flags[index]);
			sample->SetVerticalSizeFlags(vertical ? flags[index] : Control::Size_ShrinkCenter);
		}
	};

	addLabel(column, "HorizontalHint", "Horizontal alignment", 18);
	addAlignmentRow("HorizontalAlignment", false);
	addLabel(column, "VerticalHint", "Vertical alignment (this row expands vertically)", 18);
	addAlignmentRow("VerticalAlignment", true);

	// 7) Compare fixed width, expand with fill, and expand with shrink center
	addLabel(column, "ExpandHint", "HBox: fixed red | Expand + Fill green | Expand + Center yellow", 18);
	MarginContainer* expandArea = MemNew<MarginContainer>();
	expandArea->SetName("ExpandArea");
	expandArea->SetMargins(8.0f, 8.0f, 8.0f, 8.0f);
	column->AddNode(expandArea);
	addRect(expandArea, "Background", Color(45, 55, 70), Vector2::Zero);

	HBoxContainer* expandRow = MemNew<HBoxContainer>();
	expandRow->SetName("ExpandComparison");
	expandRow->SetSeparation(12.0f);
	expandArea->AddNode(expandRow);
	addRect(expandRow, "Fixed", Color(220, 80, 80), Vector2(80.0f, 28.0f));
	ColorRect* expandFill = addRect(expandRow, "ExpandFill", Color(80, 190, 120), Vector2(80.0f, 28.0f));
	expandFill->SetHorizontalSizeFlags(Control::Size_ExpandFill);
	ColorRect* expandCenter = addRect(expandRow, "ExpandCenter", Color(220, 180, 70), Vector2(80.0f, 28.0f));
	expandCenter->SetHorizontalSizeFlags(Control::Size_Expand | Control::Size_ShrinkCenter);

	Label* corner = addLabel(content, "BottomRight", "BOTTOM RIGHT | fixed margins", 20);
	corner->SetHorizontalAlignment(HorizontalAlignment::Right);
	corner->SetVerticalAlignment(VerticalAlignment::Bottom);

	tree->GetRoot()->AddNode(example);
	return sizeLabel;
}
