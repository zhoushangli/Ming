#pragma once

#include "MingEngine/Scene/GUI/Control.hpp"
#include "MingEngine/Scene/Resource/TextureResource.hpp"

class TextureRect : public Control
{
	MCLASS(TextureRect, Control);

public:
	enum class ExpandMode
	{
		KeepSize,
		IgnoreSize,
		FitWidth,
		FitWidthProportional,
		FitHeight,
		FitHeightProportional,
	};

	enum class StretchMode
	{
		Scale,
		Tile,
		Keep,
		KeepCentered,
		KeepAspect,
		KeepAspectCentered,
		KeepAspectCovered,
	};

public:
	Ref<TextureResource> GetTexture() const { return m_texture; }
	bool                 GetHorizontalFlip() const { return m_horizontalFlip; }
	bool                 GetVerticalFlip() const { return m_verticalFlip; }
	ExpandMode           GetExpandMode() const { return m_expandMode; }
	StretchMode          GetStretchMode() const { return m_stretchMode; }

	void SetTexture(Ref<TextureResource> const& texture);
	void SetHorizontalFlip(bool horizontalFlip);
	void SetVerticalFlip(bool verticalFlip);
	void SetExpandMode(ExpandMode expandMode);
	void SetStretchMode(StretchMode stretchMode);

protected:
	static void BindMethods() {}

	void OnNotification(int notification);

private:
	bool                 m_horizontalFlip = false;
	bool                 m_verticalFlip   = false;
	Ref<TextureResource> m_texture;
	ExpandMode           m_expandMode  = ExpandMode::KeepSize;
	StretchMode          m_stretchMode = StretchMode::Scale;
};
