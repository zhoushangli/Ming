#pragma once

#include "MingEngine/Engine/Application/SystemBase.hpp"
#include "MingEngine/Engine/Application/EngineBuildPreferences.hpp"

#include "MingEngine/Core/Math/Vec3.hpp"

#include "ThirdParty/fmod/fmod.hpp"

#include <map>
#include <string>
#include <vector>

typedef size_t   SoundID;
typedef size_t   SoundPlaybackID;
constexpr size_t MissingSoundId = (size_t)(-1); // for bad SoundIDs and SoundPlaybackIDs

class AudioSystem;

struct AudioConfig
{
#if defined(ENGINE_DISABLE_AUDIO)
	bool m_isEnable = false;
#else
	bool m_isEnable = true;
#endif
};

class AudioSystem : public SystemBase
{
	MCLASS(AudioSystem, SystemBase)

public:
	AudioSystem(AudioConfig const& config);
	virtual ~AudioSystem();

public:
	void         Startup() override;
	void         Shutdown() override;
	void         BeginFrame() override;
	void         EndFrame() override;
	static void  BindMethods();

#if !defined(ENGINE_DISABLE_AUDIO)
	virtual SoundID         CreateOrGetSound(std::string const& soundFilePath, FMOD_MODE mode = FMOD_2D);
	virtual SoundPlaybackID StartSound(
		SoundID soundID,
		bool    isLooped = false,
		float   volume   = 1.f,
		float   balance  = 0.0f,
		float   speed    = 1.0f,
		bool    isPaused = false
	);
	virtual void StopSound(SoundPlaybackID soundPlaybackID);
	virtual void SetSoundPlaybackVolume(SoundPlaybackID soundPlaybackID, float volume); // volume is in [0,1]
	virtual void SetSoundPlaybackBalance(
		SoundPlaybackID soundPlaybackID, float balance
	); // balance is in [-1,1], where 0 is L/R centered
	virtual void SetSoundPlaybackSpeed(
		SoundPlaybackID soundPlaybackID, float speed
	); // speed is frequency multiplier (1.0 == normal)

	virtual void ValidateResult(FMOD_RESULT result);

	void SetNumListeners(int numListeners);
	void UpdateListener(
		int listenerIndex, const Vec3& listenerPosition, const Vec3& listenerForward, const Vec3& listenerUp
	);
	virtual SoundPlaybackID StartSoundAt(
		SoundID     soundID,
		const Vec3& soundPosition,
		bool        isLooped    = false,
		float       volume      = 1.0f,
		float       balance     = 0.0f,
		float       speed       = 1.0f,
		bool        isPaused    = false,
		float       minDistance = 1.0f,
		float       maxDistance = 10.0f
	);
	virtual void SetSoundPosition(SoundPlaybackID soundPlaybackID, const Vec3& soundPosition);
	bool         IsPlaying(SoundPlaybackID soundPlaybackID);

protected:
	FMOD::System*                  m_fmodSystem;
	std::map<std::string, SoundID> m_registeredSoundIDs;
	std::vector<FMOD::Sound*>      m_registeredSounds;
#endif // !defined( ENGINE_DISABLE_AUDIO )
	AudioConfig m_config;
};

