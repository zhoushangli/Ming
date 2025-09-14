#include "Renderer.hpp"

#include <Windows.h>
#include <gl/gl.h>
#include "../Core/Vertex.hpp"
#pragma comment(lib, "opengl32")

HGLRC g_openGLRenderingContext;

Renderer::Renderer()
{
    // Nothing yet
}

Renderer::~Renderer()
{
    // Nothing yet
}

void Renderer::Startup()
{
	// Nothing yet
}

void Renderer::Shutdown()
{
    // Nothing yet
}

void Renderer::BeginFrame()
{
	glLoadIdentity();
	glOrtho(0.f, 200.f, 0.f, 100.f, 0.f, 1.f); 

	glClearColor(0.f, 0.f, 0.f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT); 
}

void Renderer::EndFrame()
{
    // Nothing yet
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
        clearColor.r / 255.0f,
        clearColor.g / 255.0f,
        clearColor.b / 255.0f,
        clearColor.a / 255.0f
    );
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::BeginCamera(Camera const& camera)
{
    glLoadIdentity();
    camera;
    // Example: Set up orthographic projection
    // glOrtho(left, right, bottom, top, zNear, zFar);
}

void Renderer::EndCamera()
{
    // Nothing yet
}

void Renderer::DrawVertexArray(int numVertexes, Vertex const* vertexes)
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

