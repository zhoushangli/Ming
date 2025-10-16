#pragma once


struct Vec2;
struct Rgba8;
struct Vertex;
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

    void DrawVertexArray(int numVertexes, Vertex const* vertexes) const;

private:
	Camera* m_currentCamera = nullptr;
};

