#pragma once

#include "MingEngine/Core/Math/IntVec2.hpp"
#include "MingEngine/Engine/Render/CameraContext.hpp"

#include <map>
#include <string>
#include <vector>

class D3D11RenderBackend;
class GPUTexture;
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
	PostProcessPass(std::string const& passName, std::string const& postProcessShaderVirtualPath);
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
	CameraContext const* m_camera;
	GPUTexture*          m_sceneColor;
	GPUTexture*          m_sceneDepth;
	GPUTexture*          m_sceneNormal;
	GPUTexture*          m_ping;
	GPUTexture*          m_pong;
	IntVec2              m_outputResolution;
};

class PostProcessChain
{
public:
	PostProcessChain();
	~PostProcessChain();

	void        AddPass(PostProcessPass const& pass);
	GPUTexture* Render(D3D11RenderBackend& renderer, PostProcessContext const& context);

	void        RegisterCustomTexture(std::string const& name, GPUTexture* texture);
	GPUTexture* GetCustomTexture(std::string const& name) const;

private:
	std::vector<PostProcessPass>       m_passes;
	std::map<std::string, GPUTexture*> m_customTextures;
};
