#pragma once

#include "MingEngine/Editor/UI/EditorPanel.hpp"
#include "MingEngine/Editor/UI/Popup/CreateNodePopup.hpp"
#include "MingEngine/Scene/GUI/Tree.hpp"
#include "MingEngine/Scene/GUI/LineEdit.hpp"
#include <map>

class ScenePanel final : public EditorPanel
{
	MCLASS(ScenePanel, EditorPanel);

public:
	ScenePanel();
	Tree* GetTree() const { return m_tree; }
	void  OpenCreateNode();

protected:
	void OnInitialize() override;
	void OnRender(EditorUIContext& context) override;

private:
	void UpdateItems();
	void DeleteSelected();
	bool HandleTreeInput(InputEvent const& event);
	bool CanReparent(ObjectID source, uint64_t target) const;
	bool CreateNode(std::string const& type, std::string& error);

private:
	Tree*                        m_tree        = nullptr;
	LineEdit*                    m_filter      = nullptr;
	Label*                       m_empty       = nullptr;
	VBoxContainer*               m_rootActions = nullptr;
	std::map<uint64_t, ObjectID> m_nodeItems;
	CreateNodePopup              m_createNodePopup;
	ObjectID                     m_createSceneID  = ObjectID::Invalid;
	ObjectID                     m_createParentID = ObjectID::Invalid;
};
