#include "MingEngine/Core/IO/ResourceSaver.hpp"

#include "MingEngine/Engine/File/FileSystem.hpp"

std::function<void(VirtualPath const&)> ResourceSaver::s_savedCallback;

void ResourceSaver::SetSavedCallback(std::function<void(VirtualPath const&)> callback)
{
	s_savedCallback = std::move(callback);
}

int                      ResourceSaver::s_saverCount = 0;
Ref<ResourceFormatSaver> ResourceSaver::s_saver[MaxSavers];

void ResourceSaver::AddSaver(Ref<ResourceFormatSaver> saver)
{
	if (saver.IsValid() && s_saverCount < MaxSavers)
	{
		s_saver[s_saverCount++] = saver;
	}
}

bool ResourceSaver::Save(VirtualPath const& virtualPath, Variant const& value)
{
	if (!virtualPath.IsValid())
	{
		return false;
	}

	for (int i = 0; i < s_saverCount; ++i)
	{
		if (s_saver[i]->CanSave(virtualPath, value))
		{
			bool const saved = s_saver[i]->Save(virtualPath, value);
			if (saved)
			{
				NotifySaved(virtualPath);
			}
			return saved;
		}
	}

	return false;
}

void ResourceSaver::NotifySaved(VirtualPath const& virtualPath)
{
	if (s_savedCallback)
	{
		s_savedCallback(virtualPath);
	}
}
