#pragma once

#include "MingEngine/Engine/Application/EngineBuildPreferences.hpp"
#include "MingEngine/Engine/Application/SystemBase.hpp"

#include "MingEngine/Core/Math/Vector3.hpp"

#include <cstddef>
#include <string>

typedef size_t   SoundID;
typedef size_t   SoundPlaybackID;
constexpr size_t MissingSoundId = (size_t)(-1); // for bad SoundIDs and SoundPlaybackIDs

class AudioSystem;
struct AudioSystemImpl;

struct AudioSystemConfig
{
#if defined(ENGINE_DISABLE_AUDIO)
	bool m_isEnable = false;
#else
	bool m_isEnable = true;
#endif
	int m_listenerCount = 1;
};

class AudioSystem : public SystemBase
{
	MCLASS(AudioSystem, SystemBase)

public:
	AudioSystem(AudioSystemConfig const& config);
	virtual ~AudioSystem();

public:
	void        Startup() override;
	void        Shutdown() override;
	void        BeginFrame() override;
	void        EndFrame() override;
	static void BindMethods();

#if !defined(ENGINE_DISABLE_AUDIO)
	virtual SoundID         CreateOrGetSound(std::string const& soundFilePath);
	virtual SoundPlaybackID StartSound(
		SoundID soundID, bool isLooped = false, float volume = 1.f, float balance = 0.0f, float speed = 1.0f);
	virtual void StopSound(SoundPlaybackID soundPlaybackID);
	virtual void SetSoundPlaybackVolume(SoundPlaybackID soundPlaybackID, float volume); // volume is in [0,1]
	virtual void SetSoundPlaybackBalance(
		SoundPlaybackID soundPlaybackID, float balance); // balance is in [-1,1], where 0 is L/R centered
	virtual void SetSoundPlaybackSpeed(
		SoundPlaybackID soundPlaybackID, float speed); // speed is frequency multiplier (1.0 == normal)

	void UpdateListener(
		int listenerIndex, const Vector3& listenerPosition, const Vector3& listenerForward, const Vector3& listenerUp);
	virtual SoundPlaybackID StartSoundAt(
		SoundID        soundID,
		const Vector3& soundPosition,
		bool           isLooped    = false,
		float          volume      = 1.0f,
		float          balance     = 0.0f,
		float          speed       = 1.0f,
		float          minDistance = 1.0f,
		float          maxDistance = 10.0f);
	virtual void SetSoundPosition(SoundPlaybackID soundPlaybackID, const Vector3& soundPosition);
	bool         IsPlaying(SoundPlaybackID soundPlaybackID);
#endif // !defined( ENGINE_DISABLE_AUDIO )

private:
	AudioSystemImpl* m_impl = nullptr;

private:
	AudioSystemConfig m_config;
};
