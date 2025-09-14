#include "Renderer.hpp"

#include <Windows.h>
#include <gl/gl.h>
#include "../Core/Vertex.hpp"
#pragma comment(lib, "opengl32")

// Only visible in this file
// static HGLRC g_openGLRenderingContext = nullptr;

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
    CreateRenderingContext();
}

void Renderer::Shutdown()
{
    // Nothing yet
}

void Renderer::BeginFrame()
{
    // Nothing yet
}

void Renderer::EndFrame()
{
    // Nothing yet
}

void Renderer::CreateRenderingContext()
{
    // Example: Move code from Main_Windows.cpp here
    // This is a placeholder; actual implementation depends on your window/context setup
    // g_openGLRenderingContext = wglCreateContext(hdc);
    // wglMakeCurrent(hdc, g_openGLRenderingContext);
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

