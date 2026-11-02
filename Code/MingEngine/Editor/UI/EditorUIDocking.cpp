#include "MingEngine/Scene/GUI/Label.hpp"
#include "MingEngine/Editor/UI/EditorUI.hpp"
#include "MingEngine/Editor/UI/EditorIcons.hpp"
#include "MingEngine/Editor/UI/Popup/EditorPopupUtils.hpp"

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
	m_dockPanels = { DockPanel{ "Scene", m_scenePage },
					 DockPanel{ "FileSystem", m_filePage },
					 DockPanel{ "Inspector", m_inspectorBackground },
		DockPanel{ "Import", m_importPanel }, DockPanel{ "Output", m_outputPanel } };
	for (DockPanel const& panel : m_dockPanels)
	{
		panel.m_root->SetSerializable(false);
	}

	m_viewportPanel->SetCustomMinimumSize(Vector2(static_cast<float>(m_nativeRoot->GetThemeConstant("viewport_min_width", "Editor")), static_cast<float>(m_nativeRoot->GetThemeConstant("viewport_min_height", "Editor"))));
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
		m_dockSlots[i]->SetThemeTypeVariation("EditorDockTabs");
		auto* actions = m_dockSlots[i]->GetTitleActions();
		Button* collapse = EditorPopupUtils::AddButton(actions, "", [this]() { ToggleOutputCollapsed(); });
		collapse->SetName("Collapse");
		collapse->SetThemeTypeVariation("EditorPanelAction");
		collapse->SetIcon(EditorIcons::GetIconTexture("ChevronDown"));
		collapse->SetVisible(false);
		Button* more = EditorPopupUtils::AddButton(actions, "", [this, i, actions]()
		{
			std::vector<MenuItem> items;
			if (m_dockSlots[i]->GetCurrentPage() == m_fileSystemPanel)
			{
				items.push_back({ "Refresh", [this]() { g_engine->m_fileSystem->ScanResourceTree(); } });
				items.push_back({ "", {}, "", true });
			}

			for (DockPanel const& dock : m_dockPanels)
			{
				auto* panel = static_cast<EditorPanel*>(dock.m_root);
				items.push_back({ dock.m_id, [this, panel]() { TogglePanel(panel); }, "", false, true, panel->IsOpen() });
			}

			items.push_back({ "", {}, "", true });
			items.push_back({ "Restore Default Layout", [this]() { ResetNativeLayout(); } });
			ShowMenu(actions, actions->GetGlobalPosition() + Vector2(0.f, actions->GetSize().y), items);
		});
		more->SetThemeTypeVariation("EditorPanelAction");
		more->SetIcon(EditorIcons::GetIconTexture("More"));
		Button* close = EditorPopupUtils::AddButton(actions, "", [this, i]()
		{
			if (auto* panel = dynamic_cast<EditorPanel*>(m_dockSlots[i]->GetCurrentPage()))
			{
				TogglePanel(panel);
			}
		});
		close->SetThemeTypeVariation("EditorPanelAction");
		close->SetIcon(EditorIcons::GetIconTexture("Close"));
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
	m_dockSplits[1]->AddNode(m_dockSplits[3]);
	m_dockSlots[2]->Reparent(m_dockSplits[1], false);
	m_viewportPanel->Reparent(m_dockSplits[3], false);
	m_dockSlots[0]->Reparent(m_dockSplits[2], false);
	m_dockSplits[2]->AddNode(m_dockSlots[1]);
	m_dockSplits[3]->AddNode(m_dockSlots[3]);
	m_dockSlots[2]->AddPage("Inspector", m_inspectorBackground);
	m_dockSlots[2]->AddPage("Import", m_importPanel);
	m_dockSlots[3]->AddPage("Output", m_outputPanel);
	for (int i = 0; i < 4; ++i)
	{
		auto* hint = MemNew<ColorRect>();
		hint->SetSerializable(false);
		hint->SetMouseFilter(Control::MouseFilter::Stop);
		hint->SetVisible(false);
		hint->SetColor(Color(211, 173, 114, 55));
		m_nativeRoot->AddNode(hint);
		m_dockHints[i] = hint;
		hint->SetDragCallbacks(
			{},
			[this, i, hint](Vector2 const& position, GuiDragData const& data)
			{
				int index = -1;
				return data.m_type == String("dock") && FindNativePanel(data.m_value) >= 0
					&& GetNativeDockDestination(i, position + hint->GetGlobalPosition(), index) >= 0;
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
				hint->SetColor(accepted ? Color(211, 173, 114, 100) : Color(211, 173, 114, 40));
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
				if (target < 0)
				{
					return;
				}

				TabContainer* slot   = m_dockSlots[target];
				if (index >= 0)
				{
					slot->SetDropInsertIndex(index);
					AABB2 const mark = slot->GetTabInsertRect(index);
					m_dockInsertMark->SetPosition(
						slot->GetGlobalPosition() - m_nativeRoot->GetGlobalPosition() + mark.m_mins);
					m_dockInsertMark->UpdateSize(mark.GetDimensions());
					m_dockInsertMark->SetVisible(true);
				}
			});
	}

	m_dockInsertMark = MemNew<ColorRect>();
	m_dockInsertMark->SetSerializable(false);
	m_dockInsertMark->SetMouseFilter(Control::MouseFilter::Ignore);
	m_dockInsertMark->SetColor(Color(211, 173, 114));
	m_dockInsertMark->SetVisible(false);
	m_nativeRoot->AddNode(m_dockInsertMark);
	bool const loading = m_loadingLayout;
	m_loadingLayout    = true;
	ResetNativeLayout();
	m_loadingLayout = loading;
	m_layoutReady   = true;
	LoadNativeLayout();
	UpdateNativeDocking();

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
	if (local.y >= tabs->GetTabBarRect().m_mins.y && local.y < tabs->GetTabBarRect().m_maxs.y)
	{
		index = tabs->GetTabInsertIndex(local);
		return index >= 0 ? slot : -1;
	}

	if (local.y < tabs->GetSize().y * 0.2f)
	{
		return slot < 2 ? 0 : slot;
	}

	if (local.y > tabs->GetSize().y * 0.8f)
	{
		return slot < 2 ? 1 : slot;
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
		slot->SetTabsAtBottom(slot->GetCurrentPage() == m_outputPanel);
		for (Node* child : slot->GetTitleActions()->GetChildren())
		{
			if (child->GetName() == String("Collapse"))
			{
				auto* button = static_cast<Button*>(child);
				button->SetVisible(slot->GetCurrentPage() == m_outputPanel);
				button->SetIcon(EditorIcons::GetIconTexture(m_outputPanel->IsCollapsed() ? "ChevronUp" : "ChevronDown"));
			}
		}
	}

	m_dockSplits[2]->SetVisible(m_dockSlots[0]->IsVisible() || m_dockSlots[1]->IsVisible());
	m_dockSplits[3]->SetVisible(true);
	bool const  docking    = m_nativeRoot->GetViewport() != nullptr && m_nativeRoot->GetViewport()->IsGuiDragging()
							 && m_nativeRoot->GetViewport()->GetGuiDragData().m_type == String("dock");
	float const bodyHeight = std::max(1.f, m_nativeRoot->GetSize().y - m_nativeRoot->GetThemeConstant("top_bar_height", "Editor")
		- m_nativeRoot->GetThemeConstant("status_bar_height", "Editor"));
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
			if (i < 2 && !m_dockSlots[i ^ 1]->IsVisible())
			{
				float const margin = std::min(125.f, bodyHeight * 0.2f);
				size.y             = std::max(1.f, size.y - margin);
				if ((i & 1) != 0)
				{
					position.y += margin;
				}
			}
		}
		else
		{
			float const width = std::min(i == 2 ? 347.5f : 275.f, m_nativeRoot->GetSize().x * 0.25f);
			float const height = std::min(165.f, bodyHeight * 0.25f);
			if (i < 2)
			{
				position = Vector2(5.f,
					static_cast<float>(m_nativeRoot->GetThemeConstant("top_bar_height", "Editor")) + (i == 0 ? 0.f : bodyHeight - height));
				size = Vector2(width, height);
			}
			else if (i == 2)
			{
				position = Vector2(m_nativeRoot->GetSize().x - width - 5.f,
					static_cast<float>(m_nativeRoot->GetThemeConstant("top_bar_height", "Editor")));
				size = Vector2(width, bodyHeight);
			}
			else
			{
				position = m_viewportPanel->GetGlobalPosition() - m_nativeRoot->GetGlobalPosition();
				size = Vector2(m_viewportPanel->GetSize().x, height);
				position.y += std::max(0.f, m_viewportPanel->GetSize().y - height);
			}
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
	for (EditorPanel* panel : std::initializer_list<EditorPanel*>{ m_scenePanel, m_fileSystemPanel, m_inspectorPanel, m_importPanel, m_outputPanel })
	{
		if (!panel->IsOpen())
		{
			TogglePanel(panel);
		}
	}

	MoveNativePanel("Scene", 0);
	MoveNativePanel("FileSystem", 1);
	MoveNativePanel("Inspector", 2);
	MoveNativePanel("Import", 2);
	MoveNativePanel("Output", 3);
	m_dockSlots[0]->SetCurrentTab(0);
	m_outputPanel->SetCollapsed(false);
	m_dockSlots[2]->SetCurrentTab(0);
	Vector2 const bounds(m_nativeRoot->GetViewport()->GetOutputResolution());
	float const width = std::max(1.f, bounds.x - 10.f);
	float const height = std::max(1.f, bounds.y - m_nativeRoot->GetThemeConstant("top_bar_height", "Editor")
		- m_nativeRoot->GetThemeConstant("status_bar_height", "Editor"));
	m_dockSplits[0]->SetSplitRatio(275.f / std::max(1.f, width - 7.5f));
	m_dockSplits[1]->SetSplitRatio(1.f - 347.5f / std::max(1.f, width - 290.f));
	m_dockSplits[2]->SetSplitRatio(490.f / std::max(1.f, height - 7.5f));
	m_dockSplits[3]->SetSplitRatio(1.f - 165.f / std::max(1.f, height - 7.5f));
	m_loadingLayout = loading;
	UpdateNativeDocking();
	if (m_layoutReady && !m_loadingLayout)
	{
		SaveNativeLayout();
	}
}

void EditorUI::ToggleOutputCollapsed()
{
	bool const collapsed = !m_outputPanel->IsCollapsed();
	m_outputPanel->SetCollapsed(collapsed);
	auto* slot = dynamic_cast<TabContainer*>(m_outputPanel->GetParent());
	if (slot == m_dockSlots[3])
	{
		if (collapsed)
		{
			m_outputSplitRatio = m_dockSplits[3]->GetSplitRatio();
		}

		m_dockSplits[3]->SetSplitRatio(collapsed ? 1.f : m_outputSplitRatio);
	}

	UpdateNativeDocking();
}

bool EditorUI::SaveNativeLayout()
{
	if (!m_layoutReady || m_loadingLayout || g_engine == nullptr || g_engine->m_fileSystem == nullptr)
	{
		return false;
	}

	nlohmann::json layout;
	layout["version"] = 2;
	layout["slots"]   = nlohmann::json::array();
	layout["ratios"]  = nlohmann::json::array();
	layout["closed"] = nlohmann::json::array();
	for (DockPanel const& entry : m_dockPanels)
	{
		if (auto* panel = dynamic_cast<EditorPanel*>(entry.m_root); panel != nullptr && !panel->IsOpen())
		{
			layout["closed"].push_back(entry.m_id.ToUtf8());
		}
	}
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
	std::filesystem::path legacy;
	if (g_engine->m_fileSystem->TryGetPhysicalPath("res://.ming/Editor/EditorLayout.ini", legacy))
	{
		std::error_code error;
		std::filesystem::remove(legacy, error);
	}

	auto discard = [this]()
	{
		std::filesystem::path path;
		if (g_engine->m_fileSystem->TryGetPhysicalPath(LayoutPath, path))
		{
			std::error_code error;
			std::filesystem::remove(path, error);
		}

		SaveNativeLayout();
	};
	std::string text;
	if (!g_engine->m_fileSystem->ReadText(LayoutPath, text))
	{
		return;
	}

	try
	{
		nlohmann::json const layout = nlohmann::json::parse(text);
		if (!layout.is_object() || layout.value("version", 0) != 2 || !layout.contains("slots")
			|| !layout["slots"].is_array())
		{
			discard();
			return;
		}

		std::array<std::vector<String>, 4> groups;
		std::array<String, 4>              current;
		std::array<float, 4> ratios;
		for (int i = 0; i < 4; ++i)
		{
			ratios[i] = m_dockSplits[i]->GetSplitRatio();
		}
		std::array<bool, 5>                found  = {};
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

		for (int i = 0; i < static_cast<int>(m_dockPanels.size()); ++i)
		{
			if (!found[i])
			{
				if (auto* panel = dynamic_cast<EditorPanel*>(m_dockPanels[i].m_root); panel != nullptr && panel->IsOpen())
				{
					groups[i == 1 ? 1 : (i == 2 || i == 3) ? 2 : i == 4 ? 3 : 0].push_back(m_dockPanels[i].m_id);
				}
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
		for (DockPanel const& entry : m_dockPanels)
		{
			auto* panel = static_cast<EditorPanel*>(entry.m_root);
			bool closed = false;
			if (layout.contains("closed") && layout["closed"].is_array())
			{
				for (auto const& value : layout["closed"])
				{
					closed = closed || (value.is_string() && value.get<std::string>() == entry.m_id.ToUtf8());
				}
			}

			bool listed = false;
			for (auto const& group : groups)
			{
				listed = listed || std::find(group.begin(), group.end(), entry.m_id) != group.end();
			}

			if ((closed && panel->IsOpen()) || (!closed && listed && !panel->IsOpen()))
			{
				TogglePanel(panel);
			}

			if (closed)
			{
				for (auto& group : groups)
				{
					group.erase(std::remove(group.begin(), group.end(), entry.m_id), group.end());
				}
			}
		}

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
		m_loadingLayout = true;
		ResetNativeLayout();
		m_loadingLayout = false;
		discard();
	}
}
