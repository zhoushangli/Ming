#pragma once

#include "MingEngine/Scene/GUI/PopupPanel.hpp"
#include "MingEngine/Scene/GUI/VBoxContainer.hpp"
#include "MingEngine/Scene/GUI/HBoxContainer.hpp"
#include "MingEngine/Scene/GUI/Button.hpp"
#include "MingEngine/Scene/GUI/Label.hpp"
#include "MingEngine/Scene/GUI/LineEdit.hpp"

namespace EditorPopupUtils
{
PopupPanel*    Create(Control* owner, String const& title, VBoxContainer*& content);
void           Layout(PopupPanel* popup, Vector2 preferredSize);
void           Open(PopupPanel* popup, Control* owner, Vector2 preferredSize);
Button*        AddButton(Control* parent, String const& text, std::function<void()> callback);
HBoxContainer* AddActions(Control* parent);
Label*         AddLabel(Control* parent, String const& text, std::string const& variation = {});
void           ClearContent(Control* content);
}
