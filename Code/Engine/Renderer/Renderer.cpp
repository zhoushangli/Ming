#include "Renderer.hpp"

#include <gl/gl.h>					
#include <windows.h>
#pragma comment( lib, "opengl32" )	

HGLRC g_openGLRenderingContext = nullptr;

Renderer::Renderer()
{
}

Renderer::~Renderer()
{
}

void Renderer::Startup()
{
	CreateRenderingContext();
}

void Renderer::Shutdown()
{

}

void Renderer::BeginFrame() const
{
	
}

void Renderer::EndFrame() const
{
	
}

void Renderer::CreateRenderingContext()
{
	
}

void Renderer::ClearScreen(Rgba8 const& clearColor)
{
	
}

void Renderer::BeginCamera(Camera const& camera)
{
	
}

void Renderer::EndCamera() const
{
}

void Renderer::DrawVertexArray(int numVertexes, Vertex const* vertexes)
{
}