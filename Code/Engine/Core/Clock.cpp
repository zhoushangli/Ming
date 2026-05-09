#include "Engine/Core/Clock.hpp"

#include "Engine/Core/Time.hpp"

#include <thread>

Clock* Clock::s_systemClock = nullptr;

Clock::Clock()
{
	Clock& systemClock = GetSystemClock();
	m_parent           = &systemClock;
	m_parent->AddChild(this);
	Reset();

	m_maxDeltaSeconds = 1.f / kMinFrameRate;
	m_minDeltaSeconds = 1.f / kMaxFrameRate;
}

Clock::Clock(Clock& parent)
{
	m_parent = &parent;
	m_parent->AddChild(this);
	Reset();

	m_maxDeltaSeconds = 1.f / kMinFrameRate;
	m_minDeltaSeconds = 1.f / kMaxFrameRate;
}

Clock::Clock(bool isSystemClock)
{
	if (isSystemClock)
	{
		m_parent = nullptr;
	}
	else
	{
		Clock& systemClock = GetSystemClock();
		m_parent           = &systemClock;
		m_parent->AddChild(this);
	}

	m_maxDeltaSeconds = 1.f / kMinFrameRate;
	m_minDeltaSeconds = 1.f / kMaxFrameRate;
}

Clock::~Clock()
{
	if (m_parent != nullptr)
	{
		m_parent->RemoveChild(this);
		m_parent = nullptr;
	}

	for (Clock* child : m_children)
	{
		if (child != nullptr)
		{
			child->m_parent = nullptr;
		}
	}
	m_children.clear();
}

void Clock::Reset()
{
	m_lastUpdateTimeInSeconds = GetCurrentTimeSeconds();
	m_totalSeconds            = 0.0;
	m_deltaSeconds            = 0.0;
	m_frameCount              = 0;
}

bool Clock::IsPaused() const { return m_isPaused; }

void Clock::Pause() { m_isPaused = true; }

void Clock::Unpause()
{
	m_isPaused        = false;
	m_stepSingleFrame = false;
}

void Clock::TogglePause()
{
	m_isPaused = !m_isPaused;
	if (!m_isPaused)
	{
		m_stepSingleFrame = false;
	}
}

void Clock::StepSingleFrame()
{
	m_isPaused        = false;
	m_stepSingleFrame = true;
}

void Clock::SetTimeScale(double timeScale) { m_timeScale = timeScale; }

double Clock::GetTimeScale() const { return m_timeScale; }

double Clock::GetDeltaSeconds() const { return m_deltaSeconds; }

double Clock::GetTotalSeconds() const { return m_totalSeconds; }

double Clock::GetFrameRate() const
{
	if (m_deltaSeconds <= 0.0)
	{
		return 0.0;
	}
	return 1.0 / m_deltaSeconds;
}

int Clock::GetFrameCount() const { return m_frameCount; }

Clock& Clock::GetSystemClock()
{
	if (Clock::s_systemClock == nullptr)
	{
		Clock::s_systemClock = new Clock(true);
		Clock::s_systemClock->Reset();
	}

	return *Clock::s_systemClock;
}

void Clock::TickSystemClock()
{
	Clock& systemClock = GetSystemClock();
	systemClock.Tick();
}

void Clock::Tick()
{
	double currentTimeSeconds = GetCurrentTimeSeconds();
	double deltaSeconds       = currentTimeSeconds - m_lastUpdateTimeInSeconds;

	while (deltaSeconds < m_minDeltaSeconds)
	{
		std::this_thread::yield();

		currentTimeSeconds = GetCurrentTimeSeconds();
		deltaSeconds       = currentTimeSeconds - m_lastUpdateTimeInSeconds;
	}

	if (deltaSeconds < 0.0)
	{
		deltaSeconds = 0.0;
	}
	if (deltaSeconds > m_maxDeltaSeconds)
	{
		deltaSeconds = m_maxDeltaSeconds;
	}

	m_lastUpdateTimeInSeconds = currentTimeSeconds;
	Advance(deltaSeconds);
}

void Clock::Advance(double deltaTimeSeconds)
{
	double scaledDeltaSeconds = deltaTimeSeconds;

	// Pause handling
	if (m_isPaused)
	{
		scaledDeltaSeconds = 0.0;
	}

	// Time scale handling (still applied even if paused -> remains 0)
	scaledDeltaSeconds *= m_timeScale;

	m_deltaSeconds = scaledDeltaSeconds;
	m_totalSeconds += scaledDeltaSeconds;
	m_frameCount++;

	// Advance children
	for (Clock* child : m_children)
	{
		if (child != nullptr)
		{
			child->Advance(m_deltaSeconds);
		}
	}

	// If we were stepping a single frame, re-pause after this frame.
	if (m_stepSingleFrame)
	{
		m_stepSingleFrame = false;
		m_isPaused        = true;
	}
}

void Clock::AddChild(Clock* childClock)
{
	if (childClock == nullptr)
	{
		return;
	}

	auto found = std::find(m_children.begin(), m_children.end(), childClock);
	if (found != m_children.end())
	{
		return;
	}

	m_children.push_back(childClock);
}

void Clock::RemoveChild(Clock* childClock)
{
	if (childClock == nullptr)
	{
		return;
	}

	auto found = std::find(m_children.begin(), m_children.end(), childClock);
	if (found != m_children.end())
	{
		m_children.erase(found);
	}
}