#include "MingEngine/Core/Object/ResourceSaver.hpp"

#include "MingEngine/Engine/File/VirtualPath.hpp"

int                      ResourceSaver::s_saverCount = 0;
Ref<ResourceFormatSaver> ResourceSaver::s_saver[MaxSavers];

void ResourceSaver::AddSaver(Ref<ResourceFormatSaver> saver)
{
	if (saver.IsValid() && s_saverCount < MaxSavers)
	{
		s_saver[s_saverCount++] = saver;
	}
}

bool ResourceSaver::Save(std::string const& virtualPath, Variant const& value)
{
	VirtualPath parsedPath;
	if (!parsedPath.Parse(virtualPath))
	{
		return false;
	}

	std::string const normalizedPath = parsedPath.ToString();
	for (int i = 0; i < s_saverCount; ++i)
	{
		if (s_saver[i]->CanSave(normalizedPath, value))
		{
			return s_saver[i]->Save(normalizedPath, value);
		}
	}

	return false;
}