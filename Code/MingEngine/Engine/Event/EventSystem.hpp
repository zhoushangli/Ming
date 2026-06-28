#pragma once

#include "MingEngine/Engine/Application/SystemBase.hpp"

#include "MingEngine/Core/NamedStrings.hpp"

#include <functional>
#include <map>
#include <string>
#include <vector>

using EventArgs                = NamedStrings;
using EventCallbackFunctionPtr = bool (*)(EventArgs&);

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

	void RegisterEvent(std::string const& eventName, EventCallbackFunctionPtr ptr);
	void UnregisterEvent(std::string const& eventName, EventCallbackFunctionPtr ptr);
	int  FireEvent(std::string const& eventName, EventArgs& args);
	int  FireEvent(std::string const& eventName);

	bool    IsEventRegistered(std::string const& eventName) const;
	Strings GetRegisteredEventNames() const;

	static void BindMethods();

protected:
	EventSystemConfig                                            m_config;
	std::map<std::string, std::vector<EventCallbackFunctionPtr>> m_subscriptionListsByEventName;
};

void RegisterEvent(std::string const& eventName, EventCallbackFunctionPtr ptr);
void UnregisterEvent(std::string const& eventName, EventCallbackFunctionPtr ptr);
int  FireEvent(std::string const& eventName);
int  FireEvent(std::string const& eventName, EventArgs& args);
