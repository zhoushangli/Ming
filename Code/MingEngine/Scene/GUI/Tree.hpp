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
	struct Item
	{
		uint64_t m_id     = 0;
		uint64_t m_parent = 0;
		String   m_text;
		bool     m_visible  = true;
		bool     m_editable = false;
	};

	Tree();
	void     SetItems(std::vector<Item> items, bool filtering = false);
	void     SetFont(Ref<FontResource> const& font);
	void     Select(uint64_t id, bool notify = false);
	uint64_t GetSelected() const { return m_selected; }
	void     SetCollapsed(uint64_t id, bool collapsed);
	bool     IsCollapsed(uint64_t id) const { return m_collapsed.count(id) != 0; }
	void     EditSelected();
	void     EnsureSelectedVisible();
	Vector2  GetInternalMinimumSize() const override;
	bool     GuiInput(InputEvent const& event) override;
	void     SetSelectedCallback(std::function<void(uint64_t)> callback) { m_selectedCallback = std::move(callback); }
	void     SetActivatedCallback(std::function<void(uint64_t)> callback) { m_activatedCallback = std::move(callback); }
	void     SetRenamedCallback(std::function<void(uint64_t, String const&)> callback)
	{
		m_renamedCallback = std::move(callback);
	}

protected:
	static void BindMethods() {}
	void        OnNotification(int notification);

private:
	void                                         RebuildRows();
	bool                                         HasChildren(uint64_t id) const;
	std::vector<Item>                            m_items;
	std::vector<size_t>                          m_rows;
	std::vector<int>                             m_depths;
	std::vector<Label*>                          m_labels;
	std::unordered_set<uint64_t>                 m_collapsed;
	Ref<FontResource>                            m_font;
	LineEdit*                                    m_edit          = nullptr;
	uint64_t                                     m_selected      = 0;
	uint64_t                                     m_editing       = 0;
	uint64_t                                     m_lastClicked   = 0;
	double                                       m_lastClickTime = 0.0;
	float                                        m_contentWidth  = 32.f;
	bool                                         m_filtering     = false;
	std::function<void(uint64_t)>                m_selectedCallback;
	std::function<void(uint64_t)>                m_activatedCallback;
	std::function<void(uint64_t, String const&)> m_renamedCallback;
};
