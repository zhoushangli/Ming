#pragma once

#include "MingEngine/Engine/Application/SystemBase.hpp"

#include "MingEngine/Core/NamedStrings.hpp"

#include <functional>
#include <map>
#include <string>
#include <vector>

using EventArgs         = NamedStrings;
using EventConnectionID = uint64_t;
using EventCallback     = std::function<bool(EventArgs&)>;

struct EventSubscription
{
	EventConnectionID m_id       = 0;
	ObjectID          m_targetID = ObjectID::Invalid;
	EventCallback     m_callback;
};

struct EventSystemConfig
{
	bool m_isEnable = true;
};

class EventSystem : public SystemBase
{
	MCLASS(EventSystem, SystemBase)

public:
	EventSystem(EventSystemConfig const& config);
	~EventSystem();

	void Startup() override;
	void Shutdown() override;
	void BeginFrame() override;
	void EndFrame() override;

	template <typename T>
	EventConnectionID RegisterEvent(std::string const& eventName, T* target, bool (T::*memberFunc)(EventArgs&))
	{
		EventCallback callback = [target, memberFunc](EventArgs& args) -> bool
		{
			return (target->*memberFunc)(args);
		};

		return RegisterEvent(eventName, callback);
	}

	EventConnectionID RegisterEvent(std::string const& eventName, EventCallback callback);
	void              UnregisterEvent(std::string const& eventName, EventConnectionID connectionID);
	int               FireEvent(std::string const& eventName, EventArgs& args);
	int               FireEvent(std::string const& eventName);

	bool    IsEventRegistered(std::string const& eventName) const;
	Strings GetRegisteredEventNames() const;

	static void BindMethods();

protected:
	EventSystemConfig m_config;

	EventConnectionID                                     m_nextConnectionID = 1;
	std::map<std::string, std::vector<EventSubscription>> m_eventMap;
};

EventConnectionID RegisterEvent(std::string const& eventName, EventCallback ptr);

template <typename T>
EventConnectionID RegisterEvent(std::string const& eventName, T* target, bool (T::*memberFunc)(EventArgs&))
{
	EventCallback callback = [target, memberFunc](EventArgs& args) -> bool
	{
		return (target->*memberFunc)(args);
	};

	return RegisterEvent(eventName, callback);
}

void UnregisterEvent(std::string const& eventName, EventConnectionID connectionID);
int  FireEvent(std::string const& eventName);
int  FireEvent(std::string const& eventName, EventArgs& args);
