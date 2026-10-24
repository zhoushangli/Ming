#pragma once

#include "MingEngine/Scene/Core/Viewport.hpp"

// Window is also Viewport, so it will also hold a renderable texture
// but specially, Window will blit this texture to the screen
// it will not call the RenderServer to blit, instead it do something like register

// There should only be one Window in the SceneTree, and it should be the root Viewport.
// Otherwise the second window will overwrite the first window's image
class Window : public Viewport
{
    MCLASS(Window, Viewport);

public:
    Window();

protected:
    static void BindMethods() {}
    
    void OnNotification(int notification);

    bool OnWindowResized(EventArgs& args);


private:
    EventConnectionID m_windowResizedEventID = 0;
};