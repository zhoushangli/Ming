#pragma once

#include "Engine/Math/IntVec2.hpp"
#include "Engine/Renderer/Camera.hpp"

#include <string>
#include <vector>

class D3D11RenderBackend;
class Texture;
class Shader;

struct OutputTextureRef
{
public:
	OutputTextureRef(std::string const& name) : m_name(name) {};
	bool IsValid() const { return m_name != "Undefined"; }

public:
	std::string m_name = "Undefined";
};

struct InputTextureRef
{
public:
	InputTextureRef(std::string const& name, int slot) : m_name(name), m_slot(slot) {};
	bool IsValid() const { return m_name != "Undefined" && m_slot != -1; }

public:
	std::string m_name = "Undefined";
	int         m_slot = -1;
};

class PostProcessPass
{
public:
	PostProcessPass(std::string const& passName, std::string const& postProcessShaderName);
	~PostProcessPass();

	bool HasCustomOutput() const;
	bool HasCustomInputs() const;

public:
	bool m_isEnabled = true;

	std::string  m_name = "Undefined";
	std::wstring m_wideName; // For use in debug annotations
	Shader*      m_postProcessShader = nullptr;

	OutputTextureRef             m_customOutput = OutputTextureRef("Undefined");
	std::vector<InputTextureRef> m_customInputs;
};

struct PostProcessContext
{
	Camera const*  m_camera;
	Texture* m_sceneColor;
	Texture* m_sceneDepth;
	Texture* m_sceneNormal;
	IntVec2  m_outputResolution;
};

class PostProcessChain
{
public:
	PostProcessChain();
	~PostProcessChain();

	void AddPass(PostProcessPass const& pass);
	void Resize(D3D11RenderBackend& renderer, IntVec2 dimensions);
	Texture* Render(D3D11RenderBackend& renderer, PostProcessContext const& context);

private:
	std::vector<PostProcessPass> m_passes;
	Shader*                      m_postProcessCopyShader = nullptr;
	Texture*                     m_ping                  = nullptr;
	Texture*                     m_pong                  = nullptr;
};