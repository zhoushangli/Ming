#pragma once

#include "MingEngine/Scene/Resource/MaterialResource.hpp"

class StandardMaterial2D : public MaterialResource
{
	MCLASS(StandardMaterial2D, MaterialResource)

public:
	StandardMaterial2D();

	void SetTint(Color const& value) { SetParameter("tint", value); }
	Color GetTint() { return GetParameter("tint").As<Color>(); }

	void SetBlendMode(BlendMode value) { MaterialResource::SetBlendMode(value); }
	BlendMode GetBlendMode() const { return m_blendMode; }
	void SetTextureFilter(TextureFilter value);
	TextureFilter GetTextureFilter() const;
	void SetTextureRepeat(TextureRepeat value);
	TextureRepeat GetTextureRepeat() const;

protected:
	static void BindMethods();
	void PrepareShader() override;
};
