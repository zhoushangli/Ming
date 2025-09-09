#pragma once

#include "Camera.hpp"
#include <Engine/Core/Rgba8.hpp>
#include <Engine/Core/Vertex.hpp>

class Renderer
{
public:
	Renderer();
	~Renderer();

	void Startup();
	void Shutdown();
	void BeginFrame() const;
	void EndFrame() const;

	void CreateRenderingContext();

	void ClearScreen(Rgba8 const& clearColor);
	void BeginCamera(Camera const& camera);
	void EndCamera() const;

	void DrawVertexArray(int numVertexes, Vertex const* vertexes);
};

