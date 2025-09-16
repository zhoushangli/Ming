#pragma once

#include <Engine/Core/Rgba8.hpp>
#include <Engine/Core/Vertex.hpp>

class Camera;

class Renderer
{
public:
    Renderer();
    ~Renderer();

    void Startup();
    void Shutdown();

    void BeginFrame();
    void EndFrame();

    void CreateRenderingContext();

    void ClearScreen(Rgba8 const& clearColor);

    void BeginCamera(Camera const& camera);
    void EndCamera();

    void DrawVertexArray(int numVertexes, Vertex const* vertexes);

private:
	Camera* m_currentCamera = nullptr;
};

