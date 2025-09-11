#include "Renderer.hpp"

#include <windows.h>
#include <gl/gl.h>					
#pragma comment( lib, "opengl32" )	

HWND g_hWnd = nullptr;
HDC g_displayDeviceContext = nullptr;	
HGLRC g_openGLRenderingContext = nullptr;

Renderer::Renderer() = default;

Renderer::~Renderer() = default;

void Renderer::Startup()
{
	CreateRenderingContext();
}

void Renderer::Shutdown()
{

}

void Renderer::BeginFrame()
{
	
}

void Renderer::EndFrame()
{
	
}

void Renderer::CreateRenderingContext()
{
	// Creates an OpenGL rendering context (RC) and binds it to the current window's device context (DC)
	PIXELFORMATDESCRIPTOR pixelFormatDescriptor;
	memset(&pixelFormatDescriptor, 0, sizeof(pixelFormatDescriptor));
	pixelFormatDescriptor.nSize = sizeof(pixelFormatDescriptor);
	pixelFormatDescriptor.nVersion = 1;
	pixelFormatDescriptor.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pixelFormatDescriptor.iPixelType = PFD_TYPE_RGBA;
	pixelFormatDescriptor.cColorBits = 24;
	pixelFormatDescriptor.cDepthBits = 24;
	pixelFormatDescriptor.cAccumBits = 0;
	pixelFormatDescriptor.cStencilBits = 8;

	// These two OpenGL-like functions (wglCreateContext and wglMakeCurrent) will remain here for now.
	int pixelFormatCode = ChoosePixelFormat(g_displayDeviceContext, &pixelFormatDescriptor);
	SetPixelFormat(g_displayDeviceContext, pixelFormatCode, &pixelFormatDescriptor);
	g_openGLRenderingContext = wglCreateContext(g_displayDeviceContext);
	wglMakeCurrent(g_displayDeviceContext, g_openGLRenderingContext);

	// #SD1ToDo: move all OpenGL functions (including those below) to Renderer.cpp (only!)
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void Renderer::ClearScreen(Rgba8 const& clearColor)
{
	
}

void Renderer::BeginCamera(Camera const& camera)
{
	
}

void Renderer::EndCamera(Camera const& camera)
{
}

void Renderer::DrawVertexArray(int numVertexes, Vertex const* vertexes)
{
}