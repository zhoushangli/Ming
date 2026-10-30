#include "MingEngine/Editor/UI/EditorUI.hpp"

#include "MingEngine/Editor/EditorNode.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/File/FileSystem.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"
#include "MingEngine/Scene/Core/Viewport.hpp"
#include "MingEngine/Scene/GUI/ColorRect.hpp"
#include "MingEngine/Scene/GUI/SplitContainer.hpp"
#include "MingEngine/Scene/GUI/SubViewportContainer.hpp"
#include "MingEngine/Scene/GUI/TabContainer.hpp"
#include "ThirdParty/nlohmann/json.hpp"

#include <algorithm>
#include <cmath>

namespace
{
constexpr char const* LayoutPath = "res://.ming/Editor/NativeEditorLayout.json";
}

EditorUI::~EditorUI()
{
	if (m_layoutReady)
	{
		SaveNativeLayout();
	}
}

TabContainer* EditorUI::GetDockSlot(int index) const
{
	return index >= 0 && index < 4 ? m_dockSlots[index] : nullptr;
}

SplitContainer* EditorUI::GetDockSplit(int index) const
{
	return index >= 0 && index < 4 ? m_dockSplits[index] : nullptr;
}

int EditorUI::FindNativePanel(String const& id) const
{
	for (int i = 0; i < static_cast<int>(m_dockPanels.size()); ++i)
	{
		if (m_dockPanels[i].m_id == id)
		{
			return i;
		}
	}

	return -1;
}

void EditorUI::InitializeNativeDocking()
{
	m_inspectorTitle->Reparent(m_inspectorBackground, false);
	m_emptyInspector->Reparent(m_inspectorBackground, false);
	m_dockPanels = { DockPanel{ "Scene", m_scenePage },
					 DockPanel{ "FileSystem", m_filePage },
					 DockPanel{ "Inspector", m_inspectorBackground } };
	for (DockPanel const& panel : m_dockPanels)
	{
		panel.m_root->SetSerializable(false);
	}

	m_sceneView->SetCustomMinimumSize(Vector2(320.f, 200.f));
	for (int i = 0; i < 4; ++i)
	{
		m_dockSplits[i] = MemNew<SplitContainer>();
		m_dockSplits[i]->SetSerializable(false);
		m_dockSplits[i]->SetVertical(i >= 2);
		m_dockSplits[i]->SetDragFinishedCallback(
			[this]()
			{
				SaveNativeLayout();
			});
		m_dockSlots[i] = i == 0 ? m_tabs : MemNew<TabContainer>();
		m_dockSlots[i]->SetSerializable(false);
		m_dockSlots[i]->SetTabsRearrangeGroup(1);
		m_dockSlots[i]->SetTabChangedCallback(
			[this](int)
			{
				if (m_layoutReady && !m_loadingLayout)
				{
					SaveNativeLayout();
				}
			});
		m_dockSlots[i]->SetTabDragCallback(
			[this, i](Control* page, GuiDragData& data)
			{
				for (DockPanel const& panel : m_dockPanels)
				{
					if (panel.m_root == page)
					{
						data = { "dock", ObjectID::Invalid, panel.m_id };
						SetNativeDragPreview(m_dockSlots[i], panel.m_id);
						return true;
					}
				}

				return false;
			});
	}

	m_nativeRoot->AddNode(m_dockSplits[0]);
	m_dockSplits[0]->AddNode(m_dockSplits[2]);
	m_dockSplits[0]->AddNode(m_dockSplits[1]);
	m_sceneView->Reparent(m_dockSplits[1], false);
	m_dockSplits[1]->AddNode(m_dockSplits[3]);
	m_dockSlots[0]->Reparent(m_dockSplits[2], false);
	m_dockSplits[2]->AddNode(m_dockSlots[1]);
	m_dockSplits[3]->AddNode(m_dockSlots[2]);
	m_dockSplits[3]->AddNode(m_dockSlots[3]);
	m_dockSlots[2]->AddPage("Inspector", m_inspectorBackground);
	for (int i = 0; i < 4; ++i)
	{
		auto* hint = MemNew<ColorRect>();
		hint->SetSerializable(false);
		hint->SetMouseFilter(Control::MouseFilter::Stop);
		hint->SetVisible(false);
		hint->SetColor(Color(60, 112, 155, 75));
		m_nativeRoot->AddNode(hint);
		m_dockHints[i] = hint;
		hint->SetDragCallbacks(
			{},
			[this](Vector2 const&, GuiDragData const& data)
			{
				return data.m_type == String("dock") && FindNativePanel(data.m_value) >= 0;
			},
			[this, i, hint](Vector2 const& position, GuiDragData const& data)
			{
				int       index  = -1;
				int const target = GetNativeDockDestination(i, position + hint->GetGlobalPosition(), index);
				MoveNativePanel(data.m_value, target, index);
			});
		hint->SetDragHoverCallback(
			[this, i, hint](Vector2 const& position, GuiDragData const& data, bool accepted)
			{
				hint->SetColor(accepted ? Color(76, 155, 215, 115) : Color(60, 112, 155, 55));
				for (TabContainer* slot : m_dockSlots)
				{
					slot->SetDropInsertIndex(-1);
				}

				m_dockInsertMark->SetVisible(false);
				if (!accepted || data.m_type != String("dock"))
				{
					return;
				}

				Vector2 const global = position + hint->GetGlobalPosition();
				int           index  = -1;
				int const     target = GetNativeDockDestination(i, global, index);
				TabContainer* slot   = m_dockSlots[target];
				if (index >= 0)
				{
					slot->SetDropInsertIndex(index);
					float x = slot->GetSize().x - 3.f;
					if (Button* button = slot->GetTabButton(index))
					{
						x = button->GetLocalPosition().x;
					}
					else if (Button* last = slot->GetTabButton(slot->GetPageCount() - 1))
					{
						x = last->GetLocalPosition().x + last->GetSize().x;
					}

					m_dockInsertMark->SetPosition(
						slot->GetGlobalPosition() - m_nativeRoot->GetGlobalPosition() + Vector2(x, 0.f));
					m_dockInsertMark->UpdateSize(Vector2(3.f, 36.f));
					m_dockInsertMark->SetVisible(true);
				}
			});
	}

	m_dockInsertMark = MemNew<ColorRect>();
	m_dockInsertMark->SetSerializable(false);
	m_dockInsertMark->SetMouseFilter(Control::MouseFilter::Ignore);
	m_dockInsertMark->SetColor(Color(135, 215, 255));
	m_dockInsertMark->SetVisible(false);
	m_nativeRoot->AddNode(m_dockInsertMark);
	bool const loading = m_loadingLayout;
	m_loadingLayout    = true;
	ResetNativeLayout();
	m_loadingLayout = loading;
	m_layoutReady   = true;
	LoadNativeLayout();
	UpdateNativeDocking();
	auto* reset = MemNew<Button>();
	reset->SetText("Reset Layout");
	reset->SetPressedCallback(
		[this]()
		{
			ResetNativeLayout();
		});
	m_toolbar->AddNode(reset);
}

bool EditorUI::MoveNativePanel(String const& id, int slot, int index)
{
	int const panelIndex = FindNativePanel(id);
	if (panelIndex < 0 || GetDockSlot(slot) == nullptr)
	{
		return false;
	}

	Control*      page = m_dockPanels[panelIndex].m_root;
	auto*         from = dynamic_cast<TabContainer*>(page->GetParent());
	TabContainer* to   = m_dockSlots[slot];
	if (from == nullptr)
	{
		return false;
	}

	bool const loading = m_loadingLayout;
	m_loadingLayout    = true;
	int const old      = from->GetPageIndex(page);
	if (index < 0)
	{
		index = to->GetPageCount();
	}

	if (from == to)
	{
		to->MovePage(old, index > old ? index - 1 : index);
	}
	else
	{
		to->InsertPage(id, page, index);
	}

	to->SetCurrentTab(to->GetPageIndex(page));
	m_loadingLayout = loading;
	UpdateNativeDocking();
	if (m_layoutReady && !m_loadingLayout)
	{
		SaveNativeLayout();
	}

	return true;
}

int EditorUI::GetNativeDockDestination(int slot, Vector2 const& globalPosition, int& index) const
{
	index              = -1;
	TabContainer* tabs = m_dockSlots[slot];
	if (!tabs->IsVisible())
	{
		return slot;
	}

	Vector2 const local = globalPosition - tabs->GetGlobalPosition();
	if (local.y < 40.f)
	{
		index = tabs->GetTabInsertIndex(local);
		return slot;
	}

	if (local.y < tabs->GetSize().y * 0.2f)
	{
		return slot & ~1;
	}

	if (local.y > tabs->GetSize().y * 0.8f)
	{
		return slot | 1;
	}

	return slot;
}

void EditorUI::UpdateNativeDocking()
{
	if (m_dockSplits[0] == nullptr)
	{
		return;
	}

	for (TabContainer* slot : m_dockSlots)
	{
		slot->SetVisible(slot->GetPageCount() != 0);
	}

	m_dockSplits[2]->SetVisible(m_dockSlots[0]->IsVisible() || m_dockSlots[1]->IsVisible());
	m_dockSplits[3]->SetVisible(m_dockSlots[2]->IsVisible() || m_dockSlots[3]->IsVisible());
	bool const  docking    = m_nativeRoot->GetViewport() != nullptr && m_nativeRoot->GetViewport()->IsGuiDragging()
							 && m_nativeRoot->GetViewport()->GetGuiDragData().m_type == String("dock");
	float const bodyHeight = std::max(1.f, m_nativeRoot->GetSize().y - 64.f);
	for (int i = 0; i < 4; ++i)
	{
		ColorRect* hint = m_dockHints[i];
		if (hint == nullptr)
		{
			continue;
		}

		hint->SetVisible(docking);
		if (!docking)
		{
			m_dockSlots[i]->SetDropInsertIndex(-1);
			continue;
		}

		TabContainer* slot = m_dockSlots[i];
		Vector2       position;
		Vector2       size;
		if (slot->IsVisible())
		{
			position = slot->GetGlobalPosition() - m_nativeRoot->GetGlobalPosition();
			size     = slot->GetSize();
			if (!m_dockSlots[i ^ 1]->IsVisible())
			{
				float const margin = std::min(100.f, bodyHeight * 0.2f);
				size.y             = std::max(1.f, size.y - margin);
				if ((i & 1) != 0)
				{
					position.y += margin;
				}
			}
		}
		else
		{
			SplitContainer* side   = m_dockSplits[2 + i / 2];
			float const     height = std::min(100.f, bodyHeight * 0.2f);
			float const     width =
				side->IsVisible() ? side->GetSize().x : std::min(180.f, m_nativeRoot->GetSize().x * 0.2f);
			float const x = side->IsVisible() ? side->GetGlobalPosition().x - m_nativeRoot->GetGlobalPosition().x
											  : (i < 2 ? 10.f : m_nativeRoot->GetSize().x - width - 10.f);
			position      = Vector2(x, (i & 1) != 0 ? 58.f + bodyHeight - height : 58.f);
			size          = Vector2(width, height);
		}

		hint->SetPosition(position);
		hint->UpdateSize(size);
	}

	if (!docking && m_dockInsertMark != nullptr)
	{
		m_dockInsertMark->SetVisible(false);
	}
}

void EditorUI::ResetNativeLayout()
{
	if (m_dockSlots[0] == nullptr)
	{
		return;
	}

	if (Viewport* viewport = m_nativeRoot->GetViewport())
	{
		viewport->CancelGuiDrag();
	}

	bool const loading = m_loadingLayout;
	m_loadingLayout    = true;
	MoveNativePanel("Scene", 0);
	MoveNativePanel("FileSystem", 0);
	MoveNativePanel("Inspector", 2);
	m_dockSlots[0]->SetCurrentTab(0);
	m_dockSlots[2]->SetCurrentTab(0);
	m_dockSplits[0]->SetSplitRatio(0.22f);
	m_dockSplits[1]->SetSplitRatio(0.62f);
	m_dockSplits[2]->SetSplitRatio(0.5f);
	m_dockSplits[3]->SetSplitRatio(0.5f);
	m_loadingLayout = loading;
	UpdateNativeDocking();
	if (m_layoutReady && !m_loadingLayout)
	{
		SaveNativeLayout();
	}
}

bool EditorUI::SaveNativeLayout()
{
	if (!m_layoutReady || m_loadingLayout || g_engine == nullptr || g_engine->m_fileSystem == nullptr)
	{
		return false;
	}

	nlohmann::json layout;
	layout["version"] = 1;
	layout["slots"]   = nlohmann::json::array();
	layout["ratios"]  = nlohmann::json::array();
	for (int i = 0; i < 4; ++i)
	{
		nlohmann::json slot;
		slot["panels"] = nlohmann::json::array();
		for (int j = 0; j < m_dockSlots[i]->GetPageCount(); ++j)
		{
			for (DockPanel const& panel : m_dockPanels)
			{
				if (panel.m_root == m_dockSlots[i]->GetPage(j))
				{
					slot["panels"].push_back(panel.m_id.ToUtf8());
				}
			}
		}

		slot["current"] = "";
		for (DockPanel const& panel : m_dockPanels)
		{
			if (panel.m_root == m_dockSlots[i]->GetCurrentPage())
			{
				slot["current"] = panel.m_id.ToUtf8();
			}
		}
		layout["slots"].push_back(slot);
		layout["ratios"].push_back(m_dockSplits[i]->GetSplitRatio());
	}

	bool const saved = g_engine->m_fileSystem->WriteText(LayoutPath, layout.dump(2));
	if (!saved && m_nativeRoot->GetSceneTree() != nullptr)
	{
		Warning("Layout save failed", "Could not save res://.ming/Editor/NativeEditorLayout.json");
	}

	return saved;
}

void EditorUI::LoadNativeLayout()
{
	m_loadingLayout = true;
	ResetNativeLayout();
	m_loadingLayout = false;
	std::string text;
	if (!g_engine->m_fileSystem->ReadText(LayoutPath, text))
	{
		return;
	}

	try
	{
		nlohmann::json const layout = nlohmann::json::parse(text);
		if (!layout.is_object() || layout.value("version", 0) != 1 || !layout.contains("slots")
			|| !layout["slots"].is_array())
		{
			return;
		}

		std::array<std::vector<String>, 4> groups;
		std::array<String, 4>              current;
		std::array<float, 4>               ratios = { 0.22f, 0.62f, 0.5f, 0.5f };
		std::array<bool, 3>                found  = {};
		for (int i = 0; i < 4 && i < static_cast<int>(layout["slots"].size()); ++i)
		{
			auto const& slot = layout["slots"][i];
			if (!slot.is_object())
			{
				continue;
			}

			if (slot.contains("current") && slot["current"].is_string())
			{
				current[i] = slot["current"].get<std::string>();
			}

			if (!slot.contains("panels") || !slot["panels"].is_array())
			{
				continue;
			}

			for (auto const& value : slot["panels"])
			{
				if (!value.is_string())
				{
					continue;
				}

				String const id(value.get<std::string>());
				int const    panel = FindNativePanel(id);
				if (panel >= 0 && !found[panel])
				{
					groups[i].push_back(id);
					found[panel] = true;
				}
			}
		}

		for (int i = 0; i < 3; ++i)
		{
			if (!found[i])
			{
				groups[i == 2 ? 2 : 0].push_back(m_dockPanels[i].m_id);
			}
		}

		if (layout.contains("ratios") && layout["ratios"].is_array())
		{
			for (int i = 0; i < 4 && i < static_cast<int>(layout["ratios"].size()); ++i)
			{
				if (layout["ratios"][i].is_number())
				{
					float const value = layout["ratios"][i].get<float>();
					if (std::isfinite(value))
					{
						ratios[i] = value;
					}
				}
			}
		}

		m_loadingLayout = true;
		for (int i = 0; i < 4; ++i)
		{
			for (String const& id : groups[i])
			{
				MoveNativePanel(id, i);
			}

			for (int j = 0; j < m_dockSlots[i]->GetPageCount(); ++j)
			{
				if (m_dockSlots[i]->GetPageTitle(j) == current[i])
				{
					m_dockSlots[i]->SetCurrentTab(j);
					break;
				}
			}

			m_dockSplits[i]->SetSplitRatio(ratios[i]);
		}

		m_loadingLayout = false;
		UpdateNativeDocking();
	}
	catch (std::exception const&)
	{
		m_loadingLayout = false;
	}
}
