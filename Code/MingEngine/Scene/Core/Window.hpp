#pragma once

#include "MingEngine/Scene/Core/Viewport.hpp"

// There should only be one Window in the SceneTree, and it should be the root Viewport.
// Otherwise the second window will overwrite the first window's image
class Window : public Viewport
{
    MCLASS(Window, Viewport);

public:
    Window();
    void SyncWindowSize();

protected:
    static void BindMethods() {}
    void OnNotification(int notification);
};