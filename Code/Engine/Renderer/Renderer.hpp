#pragma once

extern struct Rgba8;
extern class Camera;
extern class Vertex;

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
	void EndCamera(Camera const& camera);
	void DrawVertexArray(int numVertexes, Vertex const* vertexes);
};

