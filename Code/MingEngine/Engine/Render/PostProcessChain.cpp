#include "MingEngine/Engine/Render/PostProcessChain.hpp"

#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Core/ErrorWarningAssert.hpp"
#include "MingEngine/Engine/Render/D3D11RenderBackend.hpp"
#include "PostProcessChain.hpp"

PostProcessPass::PostProcessPass(std::string const& passName, std::string const& postProcessShaderName)
	: m_name(passName), m_wideName(passName.begin(), passName.end())
{
	m_postProcessShader = g_engine->m_renderer->CreateOrGetShader(postProcessShaderName.c_str());
}

PostProcessPass::~PostProcessPass() {}

bool PostProcessPass::HasCustomOutput() const
{
	{
		return m_customOutput.IsValid();
	};
}

bool PostProcessPass::HasCustomInputs() const
{
	for (const auto& input : m_customInputs)
	{
		if (input.IsValid())
		{
			return true;
		}
	}
	return false;
}

void PostProcessChain::AddPass(PostProcessPass const& pass) { m_passes.push_back(pass); }

PostProcessChain::PostProcessChain() { IntVec2 const fullResolution = g_engine->m_window->GetClientDimensions(); }

PostProcessChain::~PostProcessChain() {}

Texture* PostProcessChain::Render(D3D11RenderBackend& renderer, PostProcessContext const& context)
{
	IntVec2 const fullResolution = g_engine->m_window->GetClientDimensions();

	renderer.BindCamera(*context.m_camera);
	renderer.SetBlendMode(BlendMode::OPAQUE);
	renderer.SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
	renderer.SetDepthMode(DepthMode::READ_ONLY_ALWAYS);

	Texture* sceneColor  = context.m_sceneColor;
	Texture* sceneDepth  = context.m_sceneDepth;
	Texture* sceneNormal = context.m_sceneNormal;

	Texture* ping = context.m_ping;
	Texture* pong = context.m_pong;

	std::vector<PostProcessPass const*> enabledPasses;
	enabledPasses.reserve(m_passes.size());

	for (PostProcessPass const& pass : m_passes)
	{
		if (pass.m_isEnabled && pass.m_postProcessShader != nullptr)
		{
			enabledPasses.push_back(&pass);
		}
	}

	// Main Chain Color is the final scene color
	// We keep track of it so when case like
	// PingPong ---> CustomOutput --(used as input in later pass)--> PingPong
	// We can bind the correct texture as input for the later pass
	Texture* mainChainColorTexture = sceneColor;

	for (PostProcessPass const* pass : enabledPasses)
	{
		Texture* outputTexture = nullptr;

		if (pass->HasCustomInputs())
		{
			for (auto const& customInput : pass->m_customInputs)
			{
				if (!customInput.IsValid())
				{
					continue;
				}

				GUARANTEE_OR_DIE(
					customInput.m_slot >= PostProcessTextureSlot::CustomInputStart,
					Stringf(
						"PostProcessPass '%s' custom input '%s' uses reserved texture slot %d; custom inputs must use "
						"slot %d or higher",
						pass->m_name.c_str(),
						customInput.m_name.c_str(),
						customInput.m_slot,
						PostProcessTextureSlot::CustomInputStart));
				GUARANTEE_OR_DIE(
					customInput.m_slot < PostProcessTextureSlot::MaxSamplerSlots,
					Stringf(
						"PostProcessPass '%s' custom input '%s' uses texture slot %d, but max supported slot is %d",
						pass->m_name.c_str(),
						customInput.m_name.c_str(),
						customInput.m_slot,
						PostProcessTextureSlot::MaxSamplerSlots - 1));

				Texture* customTexture = renderer.GetTextureFromFileName(customInput.m_name.c_str());
				if (customTexture != nullptr)
				{
					renderer.BindTexture(customTexture, customInput.m_slot);
					renderer.BindSampler(SamplerMode::POINT_CLAMP, customInput.m_slot);
				}
			}
		}

		bool usesMainChainColor = false;
		if (pass->HasCustomOutput())
		{
			outputTexture = renderer.GetTextureFromFileName(pass->m_customOutput.m_name.c_str());
		}
		else
		{
			usesMainChainColor = true;
			outputTexture      = (mainChainColorTexture == ping) ? pong : ping;
		}

		renderer.BindRenderTarget(outputTexture);
		renderer.BindPostProcessInputs(mainChainColorTexture, sceneDepth, sceneNormal);
		renderer.DrawFullscreenTriangle(pass->m_postProcessShader, pass->m_wideName.c_str());
		renderer.UnbindAllShaderResourceViews();

		if (usesMainChainColor)
			mainChainColorTexture = outputTexture;
	}
	return mainChainColorTexture;
}
