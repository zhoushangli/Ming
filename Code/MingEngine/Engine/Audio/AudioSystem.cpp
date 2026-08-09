#include "MingEngine/Engine/Audio/AudioSystem.hpp"

#include "MingEngine/Engine/Application/EngineBuildPreferences.hpp"

#include "MingEngine/Core/ErrorWarningAssert.hpp"
#include "MingEngine/Core/StringUtils.hpp"

void AudioSystem::BindMethods() {}

//-----------------------------------------------------------------------------------------------
// To disable the audio backend entirely for any game,
//	#define ENGINE_DISABLE_AUDIO in your game's Code/MingEngine/Engine/Application/EngineBuildPreferences.hpp file.
//
// Note that this #include is an exception to the rule "engine code doesn't know about game code".
//	Purpose: Each game can now direct the engine via #defines to build differently for that game.
//	Downside: ALL games must now have this Code/MingEngine/Engine/Application/EngineBuildPreferences.hpp file.
//
// SD1 NOTE: THIS MEANS *EVERY* GAME MUST HAVE AN EngineBuildPreferences.hpp FILE IN ITS CODE/GAME FOLDER!!
#if defined(ENGINE_DISABLE_AUDIO)
#pragma message("AudioSystem disabled in EngineBuildPreferences.hpp")
AudioSystem::AudioSystem(AudioSystemConfig const& config) : m_config(config) {}
AudioSystem::~AudioSystem() {}
void AudioSystem::Startup() {}
void AudioSystem::Shutdown() {}
void AudioSystem::BeginFrame() {}
void AudioSystem::EndFrame() {}
#else
#pragma message("AudioSystem (miniaudio) enabled in EngineBuildPreferences.hpp")

#pragma warning(push, 0)
#define MINIAUDIO_IMPLEMENTATION
#include "ThirdParty/miniaudio/miniaudio.h"
#pragma warning(pop)

#include <algorithm>
#include <map>
#include <memory>
#include <vector>

struct AudioSystemImpl
{
	ma_engine                                            m_engine{};
	bool                                                 m_isInitialized = false;
	std::map<std::string, SoundID>                       m_registeredSoundIDs;
	std::vector<std::string>                             m_registeredSoundPaths;
	std::map<SoundPlaybackID, std::unique_ptr<ma_sound>> m_activeSounds;
	SoundPlaybackID                                      m_nextPlaybackID = 0;
};

namespace
{

ma_vec3f EngineToAudioPosition(Vector3 const& position) { return ma_vec3f{ -position.y, position.z, -position.x }; }

bool CheckMiniaudioResult(ma_result result, char const* operation)
{
	if (result == MA_SUCCESS)
		return true;

	ERROR_RECOVERABLE(Stringf("AudioSystem: %s failed with miniaudio result %i", operation, static_cast<int>(result)));
	return false;
}

ma_sound* FindActiveSound(AudioSystemImpl* impl, SoundPlaybackID soundPlaybackID, char const* operation)
{
	if (impl == nullptr || soundPlaybackID == MissingSoundId)
	{
		ERROR_RECOVERABLE(Stringf("AudioSystem: attempt to %s with a missing playback ID", operation));
		return nullptr;
	}

	auto const found = impl->m_activeSounds.find(soundPlaybackID);
	if (found == impl->m_activeSounds.end())
	{
		ERROR_RECOVERABLE(Stringf("AudioSystem: attempt to %s with an expired playback ID", operation));
		return nullptr;
	}

	return found->second.get();
}

SoundPlaybackID AllocatePlaybackID(AudioSystemImpl& impl)
{
	while (impl.m_nextPlaybackID == MissingSoundId
		   || impl.m_activeSounds.find(impl.m_nextPlaybackID) != impl.m_activeSounds.end())
		++impl.m_nextPlaybackID;

	return impl.m_nextPlaybackID++;
}

SoundPlaybackID StartSoundInternal(
	AudioSystemImpl& impl,
	SoundID          soundID,
	Vector3 const*   soundPosition,
	bool             isLooped,
	float            volume,
	float            balance,
	float            speed,
	float            minDistance,
	float            maxDistance)
{
	if (!impl.m_isInitialized || soundID >= impl.m_registeredSoundPaths.size())
		return MissingSoundId;

	ma_uint32 flags = MA_SOUND_FLAG_DECODE;
	if (soundPosition == nullptr)
		flags |= MA_SOUND_FLAG_NO_SPATIALIZATION;

	auto      sound  = std::make_unique<ma_sound>();
	ma_result result = ma_sound_init_from_file(
		&impl.m_engine,
		impl.m_registeredSoundPaths[soundID].c_str(),
		flags,
		nullptr,
		nullptr,
		sound.get());
	if (!CheckMiniaudioResult(result, "initialize sound playback"))
		return MissingSoundId;

	ma_sound_set_looping(sound.get(), isLooped ? MA_TRUE : MA_FALSE);
	ma_sound_set_volume(sound.get(), volume);
	ma_sound_set_pan(sound.get(), balance);
	ma_sound_set_pitch(sound.get(), speed);

	if (soundPosition != nullptr)
	{
		ma_vec3f const position = EngineToAudioPosition(*soundPosition);
		ma_sound_set_position(sound.get(), position.x, position.y, position.z);
		ma_sound_set_attenuation_model(sound.get(), ma_attenuation_model_inverse);
		ma_sound_set_min_distance(sound.get(), minDistance);
		ma_sound_set_max_distance(sound.get(), maxDistance);
	}

	result = ma_sound_start(sound.get());
	if (!CheckMiniaudioResult(result, "start sound playback"))
	{
		ma_sound_uninit(sound.get());
		return MissingSoundId;
	}

	SoundPlaybackID const playbackID = AllocatePlaybackID(impl);
	impl.m_activeSounds.emplace(playbackID, std::move(sound));
	return playbackID;
}

} // namespace

AudioSystem::AudioSystem(AudioSystemConfig const& config) : m_impl(new AudioSystemImpl()), m_config(config) {}

AudioSystem::~AudioSystem()
{
	Shutdown();
	delete m_impl;
	m_impl = nullptr;
}

void AudioSystem::Startup()
{
	if (m_impl == nullptr || m_impl->m_isInitialized)
		return;

	int const listenerCount = std::clamp(m_config.m_listenerCount, 1, static_cast<int>(MA_ENGINE_MAX_LISTENERS));
	if (listenerCount != m_config.m_listenerCount)
	{
		ERROR_RECOVERABLE(
			Stringf("AudioSystem: listener count %i is invalid; using %i", m_config.m_listenerCount, listenerCount));
	}

	ma_engine_config engineConfig = ma_engine_config_init();
	engineConfig.listenerCount    = static_cast<ma_uint32>(listenerCount);

	ma_result const result = ma_engine_init(&engineConfig, &m_impl->m_engine);
	if (CheckMiniaudioResult(result, "initialize audio engine"))
		m_impl->m_isInitialized = true;
}

void AudioSystem::Shutdown()
{
	if (m_impl == nullptr || !m_impl->m_isInitialized)
		return;

	for (auto& activeSound : m_impl->m_activeSounds)
	{
		CheckMiniaudioResult(ma_sound_stop(activeSound.second.get()), "stop sound during shutdown");
		ma_sound_uninit(activeSound.second.get());
	}
	m_impl->m_activeSounds.clear();

	ma_resource_manager* resourceManager = ma_engine_get_resource_manager(&m_impl->m_engine);
	if (resourceManager != nullptr)
	{
		for (std::string const& soundPath : m_impl->m_registeredSoundPaths)
		{
			CheckMiniaudioResult(
				ma_resource_manager_unregister_file(resourceManager, soundPath.c_str()),
				"unregister sound resource");
		}
	}

	m_impl->m_registeredSoundIDs.clear();
	m_impl->m_registeredSoundPaths.clear();
	m_impl->m_nextPlaybackID = 0;

	ma_engine_uninit(&m_impl->m_engine);
	m_impl->m_isInitialized = false;
}

void AudioSystem::BeginFrame()
{
	if (m_impl == nullptr || !m_impl->m_isInitialized)
		return;

	for (auto sound = m_impl->m_activeSounds.begin(); sound != m_impl->m_activeSounds.end();)
	{
		if (ma_sound_at_end(sound->second.get()) == MA_FALSE)
		{
			++sound;
			continue;
		}

		ma_sound_uninit(sound->second.get());
		sound = m_impl->m_activeSounds.erase(sound);
	}
}

void AudioSystem::EndFrame() {}

SoundID AudioSystem::CreateOrGetSound(std::string const& soundFilePath)
{
	if (m_impl == nullptr || !m_impl->m_isInitialized)
		return MissingSoundId;

	auto const found = m_impl->m_registeredSoundIDs.find(soundFilePath);
	if (found != m_impl->m_registeredSoundIDs.end())
		return found->second;

	ma_resource_manager* resourceManager = ma_engine_get_resource_manager(&m_impl->m_engine);
	if (resourceManager == nullptr)
		return MissingSoundId;

	ma_result const result = ma_resource_manager_register_file(
		resourceManager,
		soundFilePath.c_str(),
		MA_RESOURCE_MANAGER_DATA_SOURCE_FLAG_DECODE);
	if (!CheckMiniaudioResult(result, "register sound resource"))
		return MissingSoundId;

	SoundID const soundID                       = m_impl->m_registeredSoundPaths.size();
	m_impl->m_registeredSoundIDs[soundFilePath] = soundID;
	m_impl->m_registeredSoundPaths.push_back(soundFilePath);
	return soundID;
}

SoundPlaybackID AudioSystem::StartSound(SoundID soundID, bool isLooped, float volume, float balance, float speed)
{
	if (m_impl == nullptr)
		return MissingSoundId;

	return StartSoundInternal(*m_impl, soundID, nullptr, isLooped, volume, balance, speed, 1.0f, 10.0f);
}

void AudioSystem::StopSound(SoundPlaybackID soundPlaybackID)
{
	ma_sound* sound = FindActiveSound(m_impl, soundPlaybackID, "stop sound");
	if (sound == nullptr)
		return;

	CheckMiniaudioResult(ma_sound_stop(sound), "stop sound playback");
	ma_sound_uninit(sound);
	m_impl->m_activeSounds.erase(soundPlaybackID);
}

void AudioSystem::SetSoundPlaybackVolume(SoundPlaybackID soundPlaybackID, float volume)
{
	ma_sound* sound = FindActiveSound(m_impl, soundPlaybackID, "set sound volume");
	if (sound != nullptr)
		ma_sound_set_volume(sound, volume);
}

void AudioSystem::SetSoundPlaybackBalance(SoundPlaybackID soundPlaybackID, float balance)
{
	ma_sound* sound = FindActiveSound(m_impl, soundPlaybackID, "set sound balance");
	if (sound != nullptr)
		ma_sound_set_pan(sound, balance);
}

void AudioSystem::SetSoundPlaybackSpeed(SoundPlaybackID soundPlaybackID, float speed)
{
	ma_sound* sound = FindActiveSound(m_impl, soundPlaybackID, "set sound speed");
	if (sound != nullptr)
		ma_sound_set_pitch(sound, speed);
}

void AudioSystem::UpdateListener(
	int listenerIndex, Vector3 const& listenerPosition, Vector3 const& listenerForward, Vector3 const& listenerUp)
{
	if (m_impl == nullptr || !m_impl->m_isInitialized || listenerIndex < 0
		|| listenerIndex >= static_cast<int>(ma_engine_get_listener_count(&m_impl->m_engine)))
	{
		ERROR_RECOVERABLE("AudioSystem: attempt to update an invalid listener");
		return;
	}

	ma_vec3f const position = EngineToAudioPosition(listenerPosition);
	ma_vec3f const forward  = EngineToAudioPosition(listenerForward);
	ma_vec3f const up       = EngineToAudioPosition(listenerUp);

	ma_engine_listener_set_position(
		&m_impl->m_engine,
		static_cast<ma_uint32>(listenerIndex),
		position.x,
		position.y,
		position.z);
	ma_engine_listener_set_direction(
		&m_impl->m_engine,
		static_cast<ma_uint32>(listenerIndex),
		forward.x,
		forward.y,
		forward.z);
	ma_engine_listener_set_world_up(&m_impl->m_engine, static_cast<ma_uint32>(listenerIndex), up.x, up.y, up.z);
}

SoundPlaybackID AudioSystem::StartSoundAt(
	SoundID        soundID,
	Vector3 const& soundPosition,
	bool           isLooped,
	float          volume,
	float          balance,
	float          speed,
	float          minDistance,
	float          maxDistance)
{
	if (m_impl == nullptr)
		return MissingSoundId;

	return StartSoundInternal(
		*m_impl,
		soundID,
		&soundPosition,
		isLooped,
		volume,
		balance,
		speed,
		minDistance,
		maxDistance);
}

void AudioSystem::SetSoundPosition(SoundPlaybackID soundPlaybackID, Vector3 const& soundPosition)
{
	ma_sound* sound = FindActiveSound(m_impl, soundPlaybackID, "set sound position");
	if (sound == nullptr)
		return;

	ma_vec3f const position = EngineToAudioPosition(soundPosition);
	ma_sound_set_position(sound, position.x, position.y, position.z);
}

bool AudioSystem::IsPlaying(SoundPlaybackID soundPlaybackID)
{
	ma_sound* sound = FindActiveSound(m_impl, soundPlaybackID, "check sound playback state");
	return sound != nullptr && ma_sound_is_playing(sound) == MA_TRUE;
}

#endif // !defined( ENGINE_DISABLE_AUDIO )
