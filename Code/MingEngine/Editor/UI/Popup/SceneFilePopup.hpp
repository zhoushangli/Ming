#pragma once

#include "MingEngine/Editor/UI/EditorUIContext.hpp"
#include "MingEngine/Editor/UI/Popup/EditorPopupUtils.hpp"
#include "MingEngine/Engine/File/VirtualPath.hpp"
#include "MingEngine/Scene/GUI/Tree.hpp"

#include <functional>
#include <map>

class SceneFilePopup
{
public:
	enum class Mode
	{
		Open,
		Save,
		MainScene
	};

public:
	void Open(
		Mode                                    mode,
		VirtualPath const&                      initial,
		std::function<bool(VirtualPath const&)> accepted,
		std::function<void()>                   cancelled = {});
	void Render(EditorUIContext& context);
	bool Confirm();

private:
	void Navigate(VirtualPath const& directory);
	void Refresh();

private:
	EditorUIContext                         m_context;
	PopupPanel*                             m_popup     = nullptr;
	VBoxContainer*                          m_content   = nullptr;
	LineEdit*                               m_path      = nullptr;
	LineEdit*                               m_name      = nullptr;
	Tree*                                   m_tree      = nullptr;
	Label*                                  m_error     = nullptr;
	Button*                                 m_confirm   = nullptr;
	Mode                                    m_mode      = Mode::Open;
	VirtualPath                             m_directory = VirtualPath("res://");
	VirtualPath                             m_initial;
	VirtualPath                             m_overwrite;
	std::map<uint64_t, VirtualPath>         m_entries;
	std::function<bool(VirtualPath const&)> m_accepted;
	std::function<void()>                   m_cancelled;
	bool                                    m_requested = false;
	bool                                    m_accepting = false;
};
