#pragma once

#include "MingEngine/Core/Object/RefCounted.hpp"
#include "MingEngine/Core/Object/Variant.hpp"

#include <string>

class ResourceFormatSaver : public RefCounted
{
	MCLASS(ResourceFormatSaver, RefCounted)

public:
	virtual bool CanSave(std::string const& virtualPath, Variant const& value) const = 0;
	virtual bool Save(std::string const& virtualPath, Variant const& value) = 0;

protected:
	static void BindMethods() {};
};

class ResourceSaver
{
public:
	static void AddSaver(Ref<ResourceFormatSaver> saver);
	static bool Save(std::string const& virtualPath, Variant const& value);

private:
	static constexpr int MaxSavers = 64;

	static int                      s_saverCount;
	static Ref<ResourceFormatSaver> s_saver[MaxSavers];
};