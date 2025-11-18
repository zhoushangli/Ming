#include "Engine/Renderer/Renderer.hpp"

#include "Engine/Core/Engine.hpp"
#include "Engine/Core/Vertex.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Renderer/Texture.hpp"

#include "ThirdParty/stb/stb_image.h"

#include <Windows.h>
#include <gl/gl.h>
#pragma comment(lib, "opengl32")

HGLRC g_openGLRenderingContext = nullptr;

Renderer::Renderer(RendererConfig config) : m_config(config)
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
    for (Texture* texture : m_loadedTextures)
	{
		delete texture;
    }
}

void Renderer::BeginFrame()
{

}

void Renderer::EndFrame()
{
	HDC displayDeviceContext = (HDC)g_engine->m_window->m_displayDeviceContext;
	SwapBuffers(displayDeviceContext);
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

	GUARANTEE_OR_DIE(g_engine, "No Engine instance!");
	GUARANTEE_OR_DIE(g_engine->m_window, "No Window instance!");
	GUARANTEE_OR_DIE(g_engine->m_window->m_displayDeviceContext, "No Window Display Device Context!");

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

Texture* Renderer::CreateOrGetTextureFromFile(char const* imageFilePath)
{
	// See if we already have this texture previously loaded
	Texture* existingTexture = GetTextureFromFileName(imageFilePath); // You need to write this
	if (existingTexture)
	{
		return existingTexture;
	}

	// Never seen this texture before!  Let's load it.
	Texture* newTexture = CreateTextureFromFile(imageFilePath);
	return newTexture;
}


Texture* Renderer::CreateTextureFromFile(char const* imageFilePath)
{
	IntVec2 dimensions = IntVec2::ZERO;		// This will be filled in for us to indicate image width & height
	int bytesPerTexel = 0;					// ...and how many color components the image had (e.g. 3=RGB=24bit, 4=RGBA=32bit)

	// Load (and decompress) the image RGB(A) bytes from a file on disk into a memory buffer (array of bytes)
	stbi_set_flip_vertically_on_load(1); // We prefer uvTexCoords has origin (0,0) at BOTTOM LEFT
	unsigned char* texelData = stbi_load(imageFilePath, &dimensions.x, &dimensions.y, &bytesPerTexel, 0);

	// Check if the load was successful
	GUARANTEE_OR_DIE(texelData, Stringf("Failed to load image \"%s\"", imageFilePath));

	Texture* newTexture = CreateTextureFromData(imageFilePath, dimensions, bytesPerTexel, texelData);

	// Free the raw image texel data now that we've sent a copy of it down to the GPU to be stored in video memory
	stbi_image_free(texelData);

	return newTexture;
}

Texture* Renderer::GetTextureFromFileName(char const* imageFilePath)
{
    if (imageFilePath == nullptr) 
	{
        return nullptr;
    }

	std::string filPath = std::string(imageFilePath);
    for (Texture* texture : m_loadedTextures)
    {
        if (texture && texture->m_name == filPath)
        {
            return texture;
        }
    }

    return nullptr;
}

Texture* Renderer::CreateTextureFromData(char const* name, IntVec2 dimensions, int bytesPerTexel, uint8_t* texelData)
{
	// Check if the load was successful
	GUARANTEE_OR_DIE(texelData, Stringf("CreateTextureFromData failed for \"%s\" - texelData was null!", name));
//	GUARANTEE_OR_DIE(bytesPerTexel >= 3 && bytesPerTeCreateTextureFromData(char const* name, IntVec2 dimensions, int bytesPerTexel, uint8_t * texelData)xel <= 4, Stringf("CreateTextureFromData failed for \"%s\" - unsupported BPP=%i (must be 3 or 4)", name, bytesPerTexel));
	GUARANTEE_OR_DIE(dimensions.x > 0 && dimensions.y > 0, Stringf("CreateTextureFromData failed for \"%s\" - illegal texture dimensions (%i x %i)", name, dimensions.x, dimensions.y));

	Texture* newTexture = new Texture();
	newTexture->m_name = name; // NOTE: m_name must be a std::string, otherwise it may point to temporary data!
	newTexture->m_dimensions = dimensions;

	// Enable OpenGL texturing
	glEnable(GL_TEXTURE_2D);

	// Tell OpenGL that our pixel data is single-byte aligned
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	// Ask OpenGL for an unused texName (ID number) to use for this texture
	glGenTextures(1, (GLuint*)&newTexture->m_textureID);

	// Tell OpenGL to bind (set) this as the currently active texture
	glBindTexture(GL_TEXTURE_2D, newTexture->m_textureID);

	// Set texture clamp vs. wrap (repeat) default settings
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); // GL_CLAMP or GL_REPEAT
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); // GL_CLAMP or GL_REPEAT

	// Set magnification (texel > pixel) and minification (texel < pixel) filters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); // one of: GL_NEAREST, GL_LINEAR
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); // one of: GL_NEAREST, GL_LINEAR, GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST_MIPMAP_LINEAR, GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR_MIPMAP_LINEAR

	// Pick the appropriate OpenGL format (RGB or RGBA) for this texel data
	GLenum bufferFormat = GL_RGBA; // the format our source pixel data is in; any of: GL_RGB, GL_RGBA, GL_LUMINANCE, GL_LUMINANCE_ALPHA, ...
	if (bytesPerTexel == 3)
	{
		bufferFormat = GL_RGB;
	}
	GLenum internalFormat = bufferFormat; // the format we want the texture to be on the card; technically allows us to translate into a different texture format as we upload to OpenGL

	// Upload the image texel data (raw pixels bytes) to OpenGL under the currently-bound OpenGL texture ID
	glTexImage2D(			// Upload this pixel data to our new OpenGL texture
		GL_TEXTURE_2D,		// Creating this as a 2d texture
		0,					// Which mipmap level to use as the "root" (0 = the highest-quality, full-res image), if mipmaps are enabled
		internalFormat,		// Type of texel format we want OpenGL to use for this texture internally on the video card
		dimensions.x,		// Texel-width of image; for maximum compatibility, use 2^N + 2^B, where N is some integer in the range [3,11], and B is the border thickness [0,1]
		dimensions.y,		// Texel-height of image; for maximum compatibility, use 2^M + 2^B, where M is some integer in the range [3,11], and B is the border thickness [0,1]
		0,					// Border size, in texels (must be 0 or 1, recommend 0)
		bufferFormat,		// Pixel format describing the composition of the pixel data in buffer
		GL_UNSIGNED_BYTE,	// Pixel color components are unsigned bytes (one byte per color channel/component)
		texelData);		// Address of the actual pixel data bytes/buffer in system memory

	m_loadedTextures.push_back(newTexture);
	return newTexture;
}

BitmapFont* Renderer::CreateOrGetBitmapFont(char const* fontFilePathNameWithNoExtension)
{
    Texture* fontTexture = CreateOrGetTextureFromFile(Stringf("%s.png", fontFilePathNameWithNoExtension).c_str());
    BitmapFont* newBitmapFont = new BitmapFont(fontFilePathNameWithNoExtension, *fontTexture);
	return newBitmapFont;
}

void Renderer::BindTexture(Texture* texture)
{
	if (texture)
	{
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, texture->m_textureID);
	}
	else
	{
		glDisable(GL_TEXTURE_2D);
	}
}

void Renderer::DrawVertexArray(std::vector<Vertex> const& verts) const
{
	DrawVertexArray(static_cast<int>(verts.size()), verts.data());
}
