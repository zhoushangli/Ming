#include "MingEngine/Engine/Event/EventSystem.hpp"

#include "MingEngine/Engine/Application/Engine.hpp"

EventSystem::EventSystem(EventSystemConfig const& config) : m_config(config) {}

EventSystem::~EventSystem() {}

void EventSystem::BindMethods() {}

void EventSystem::Startup() {}

void EventSystem::Shutdown()
{
	for (auto& pair : m_subscriptionListsByEventName)
	{
		pair.second.clear();
	}
	m_subscriptionListsByEventName.clear();
}

void EventSystem::BeginFrame() {}

void EventSystem::EndFrame() {}

void EventSystem::RegisterEvent(std::string const& eventName, EventCallbackFunctionPtr ptr)
{
	m_subscriptionListsByEventName[eventName].push_back(ptr);
}

void EventSystem::UnregisterEvent(std::string const& eventName, EventCallbackFunctionPtr ptr)
{
	auto& callbackList = m_subscriptionListsByEventName[eventName];
	auto  it           = std::find(callbackList.begin(), callbackList.end(), ptr);
	if (it != callbackList.end())
	{
		callbackList.erase(it);
	}
}

int EventSystem::FireEvent(std::string const& eventName, EventArgs& args)
{
	auto& callbackList = m_subscriptionListsByEventName[eventName];
	int   numCallbacks = 0;
	for (auto& callback : callbackList)
	{
		numCallbacks++;
		if (callback(args))
		{
			break; // Stop propagation if callback returns true
		}
	}

	return numCallbacks;
}

int EventSystem::FireEvent(std::string const& eventName)
{
	NamedStrings args;
	return FireEvent(eventName, args);
}

bool EventSystem::IsEventRegistered(std::string const& eventName) const
{
	auto it = m_subscriptionListsByEventName.find(eventName);
	return it != m_subscriptionListsByEventName.end();
}

Strings EventSystem::GetRegisteredEventNames() const
{
	Strings registeredEventNames;
	registeredEventNames.reserve(m_subscriptionListsByEventName.size());

	for (auto const& pair : m_subscriptionListsByEventName)
	{
		registeredEventNames.push_back(pair.first);
	}

	return registeredEventNames;
}

void RegisterEvent(std::string const& eventName, EventCallbackFunctionPtr ptr)
{
	g_engine->m_eventSystem->RegisterEvent(eventName, ptr);
}

void UnregisterEvent(std::string const& eventName, EventCallbackFunctionPtr ptr)
{
	g_engine->m_eventSystem->UnregisterEvent(eventName, ptr);
}

int FireEvent(std::string const& eventName) { return g_engine->m_eventSystem->FireEvent(eventName); }

int FireEvent(std::string const& eventName, EventArgs& args)
{
	return g_engine->m_eventSystem->FireEvent(eventName, args);
}
