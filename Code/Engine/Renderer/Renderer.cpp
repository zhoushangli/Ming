#include "Renderer.hpp"

#include "Camera.hpp"

#include "Engine/Core/Vertex.hpp"
#include "Engine/Core/VertexUtils.hpp"

#include <Windows.h>
#include <gl/gl.h>
#pragma comment(lib, "opengl32")

HGLRC g_openGLRenderingContext = nullptr;

Renderer::Renderer(RendererConfig config) : m_config(config)
{
    CreateRenderingContext();
}

Renderer::~Renderer()
{
}

void Renderer::Startup()
{
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

    HWND windowHandle = ::GetActiveWindow();
    HDC displayDeviceContext = GetDC(windowHandle);

	// These two OpenGL-like functions (wglCreateContext and wglMakeCurrent) will remain here for now.
	int pixelFormatCode = ChoosePixelFormat(displayDeviceContext, &pixelFormatDescriptor);
	SetPixelFormat(displayDeviceContext, pixelFormatCode, &pixelFormatDescriptor);
	g_openGLRenderingContext = wglCreateContext(displayDeviceContext);
	wglMakeCurrent(displayDeviceContext, g_openGLRenderingContext);

	// #SD1ToDo: move all OpenGL functions (including those below) to Renderer.cpp (only!)
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void Renderer::ClearScreen(Rgba8 const& clearColor)
{
    glClearColor(
        static_cast<float>(clearColor.r) / 255.0f,
		static_cast<float>(clearColor.g) / 255.0f,
        static_cast<float>(clearColor.b) / 255.0f,
        static_cast<float>(clearColor.a) / 255.0f
    );
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::BeginCamera(Camera const& camera)
{	
	glLoadIdentity();
	glOrtho(camera.GetLeft(), camera.GetRight(), camera.GetBottom(), camera.GetTop(), 0.f, 1.f);
}

void Renderer::EndCamera()
{

}

void Renderer::DrawVertexArray(int numVertexes, Vertex const* vertexes) const
{
    if (numVertexes % 3 != 0 || vertexes == nullptr) 
    {
        return;
    }

    glBegin(GL_TRIANGLES);
    for (int i = 0; i < numVertexes; ++i)
    {
        glColor4ub(vertexes[i].m_color.r, vertexes[i].m_color.g, vertexes[i].m_color.b, vertexes[i].m_color.a);
        glTexCoord2f(vertexes[i].m_uvTexCoords.x, vertexes[i].m_uvTexCoords.y);
        glVertex3f(vertexes[i].m_position.x, vertexes[i].m_position.y, vertexes[i].m_position.z);
    }
    glEnd();
}

