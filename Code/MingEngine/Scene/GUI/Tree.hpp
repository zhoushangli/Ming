#pragma once

#include "MingEngine/Scene/GUI/Control.hpp"
#include "MingEngine/Scene/Resource/FontResource.hpp"

#include <functional>
#include <unordered_set>

class Label;
class LineEdit;

class Tree : public Control
{
	MCLASS(Tree, Control);

public:
	struct ItemAction
	{
		Ref<TextureResource> m_icon;
		Color m_color = Color::White;
		bool m_enabled = false;
	};

	struct Item
	{
		uint64_t m_ID       = 0;
		uint64_t m_parentID = 0;
		String   m_text;
		bool     m_visible  = true;
		bool     m_editable = false;
		Ref<TextureResource> m_icon;
		Color m_iconColor = Color::White;
		std::vector<ItemAction> m_actions;
		bool m_dimmed = false;
	};

private:
	struct ThemeCache
	{
		Ref<FontResource>  font;
		int                font_size = 16;
		int                row_height = 1;
		int                indent = 0;
		int icon_size = 16;
		int fold_size = 12;
		int icon_separation = 4;
		Color              font_color;
		Color              font_selected_color;
		Color guide_color;
		int guide_width = 0;
		int children_guide_width = 0;
		Color children_guide_color;
		Ref<TextureResource> arrow_collapsed;
		Ref<TextureResource> arrow_expanded;
		Ref<ThemeStyleBox> panel;
		Ref<ThemeStyleBox> hover;
		Ref<ThemeStyleBox> selected;
		Ref<ThemeStyleBox> focus;
		Ref<ThemeStyleBox> drop_mark;
	};

public:
	Tree();

	uint64_t GetSelected() const { return m_selected; }
	uint64_t GetItemAtPosition(Vector2 const& position) const;
	bool     IsCollapsed(uint64_t id) const { return m_collapsed.count(id) != 0; }
	Vector2  GetInternalMinimumSize() const override;
	float    GetRowHeight() const;
	AABB2    GetItemRect(uint64_t id) const;
	AABB2 GetItemActionRect(uint64_t id, int action) const;

	void SetItems(std::vector<Item> items, bool filtering = false);
	void SetFont(Ref<FontResource> const& font);
	void SetClipText(bool enabled) { m_clipText = enabled; UpdateRowLayout(); }
	void SetItemActionCallback(std::function<void(uint64_t, int)> callback) { m_itemActionCallback = std::move(callback); }
	void SetCollapsed(uint64_t id, bool collapsed);

	void     SetSelectedCallback(std::function<void(uint64_t)> callback) { m_selectedCallback = std::move(callback); }
	void     SetActivatedCallback(std::function<void(uint64_t)> callback) { m_activatedCallback = std::move(callback); }
	void     SetRenamedCallback(std::function<void(uint64_t, String const&)> callback)
	{
		m_renamedCallback = std::move(callback);
	}

	void SetInputCallback(std::function<bool(InputEvent const&)> callback) { m_inputCallback = std::move(callback); }

	void Select(uint64_t id, bool notify = false);
	void EditSelected();
	void EnsureSelectedVisible();

	bool GuiInput(InputEvent const& event) override;
	bool GetDragData(Vector2 const& position, GuiDragData& data) override;
	void DragHover(Vector2 const& position, GuiDragData const& data, bool accepted) override;

protected:
	static void BindMethods();

	void OnThemeChanged() override;
	void OnNotification(int notification);

private:
	bool HasChildren(uint64_t id) const;
	int GetRowAtPosition(Vector2 const& position) const;
	AABB2 GetRowRect(size_t row) const;
	float GetFoldWidth(size_t row) const;
	float GetRowTextX(size_t row) const;
	float GetRowTextEnd(size_t row) const;

	void RebuildRows();
	void UpdateRowLayout();

private:
	std::vector<Item>                            m_items;
	std::vector<size_t>                          m_rows;
	std::vector<int>                             m_depths;
	std::vector<Label*>                          m_labels;
	std::unordered_set<uint64_t>                 m_collapsed;
	ThemeCache                                   m_themeCache;
	LineEdit*                                    m_edit          = nullptr;
	uint64_t                                     m_selected      = 0;
	uint64_t                                     m_editing       = 0;
	uint64_t                                     m_lastClicked   = 0;
	uint64_t                                     m_dropItem = 0;
	uint64_t                                     m_hoveredItem = 0;
	double                                       m_lastClickTime = 0.0;
	float                                        m_contentWidth  = 32.f;
	bool                                         m_filtering     = false;
	bool                                         m_updatingRowLayout = false;
	bool m_clipText = false;
	std::function<void(uint64_t, int)> m_itemActionCallback;
	std::function<bool(InputEvent const&)> m_inputCallback;
	std::function<void(uint64_t)>                m_selectedCallback;
	std::function<void(uint64_t)>                m_activatedCallback;
	std::function<void(uint64_t, String const&)> m_renamedCallback;
};
