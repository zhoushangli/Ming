#pragma once

#include "MingEngine/Core/Object/Resource.hpp"
#include "MingEngine/Core/Render/RID.hpp"

class FontResource : public Resource
{
	MCLASS(FontResource, Resource)

public:
	~FontResource() override;

	static Ref<FontResource> CreateDefault(bool sizeInEm = false);

	RID GetFontRID() const { return m_fontRID; }

	void SetFontRID(RID rid);
	bool CopyFrom(Resource&& other) override;

protected:
	static void BindMethods() {}

private:
	RID m_fontRID = RID::Invalid;
};
