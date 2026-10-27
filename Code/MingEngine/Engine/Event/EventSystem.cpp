#include "MingEngine/Engine/Event/EventSystem.hpp"

#include "MingEngine/Engine/Application/Engine.hpp"

EventSystem::EventSystem(EventSystemConfig const& config) : m_config(config)
{
}

EventSystem::~EventSystem()
{
}

void EventSystem::BindMethods()
{
}

void EventSystem::Startup()
{
}

void EventSystem::Shutdown()
{
	for (auto& pair : m_eventMap)
	{
		pair.second.clear();
	}

	m_eventMap.clear();
}

void EventSystem::BeginFrame()
{
}

void EventSystem::EndFrame()
{
}

EventConnectionID EventSystem::RegisterEvent(std::string const& eventName, EventCallback callback)
{
	EventConnectionID connectionID = m_nextConnectionID++;
	EventSubscription subscription;
	subscription.m_id       = connectionID;
	subscription.m_targetID = ObjectID::Invalid; // No specific target for this subscription
	subscription.m_callback = callback;

	m_eventMap[eventName].push_back(subscription);

	return connectionID;
}

void EventSystem::UnregisterEvent(std::string const& eventName, EventConnectionID connectionID)
{
	auto it = m_eventMap.find(eventName);
	if (it != m_eventMap.end())
	{
		std::vector<EventSubscription>& callbackList = it->second;
		callbackList.erase(
			std::remove_if(
				callbackList.begin(),
				callbackList.end(),
				[connectionID](const EventSubscription& subscription)
				{
					return subscription.m_id == connectionID;
				}),
			callbackList.end());

		if (callbackList.empty())
		{
			m_eventMap.erase(it);
		}
	}
}

int EventSystem::FireEvent(std::string const& eventName, EventArgs& args)
{
	std::vector<EventSubscription> const& subscriptionList = m_eventMap[eventName];
	int                                   numCallbacks     = 0;
	for (const EventSubscription& subscription : subscriptionList)
	{
		if (subscription.m_callback)
		{
			subscription.m_callback(args);
			numCallbacks++;
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
	auto it = m_eventMap.find(eventName);
	return it != m_eventMap.end();
}

Strings EventSystem::GetRegisteredEventNames() const
{
	Strings registeredEventNames;
	registeredEventNames.reserve(m_eventMap.size());

	for (auto const& pair : m_eventMap)
	{
		registeredEventNames.push_back(pair.first);
	}

	return registeredEventNames;
}

EventConnectionID RegisterEvent(std::string const& eventName, EventCallback ptr)
{
	return g_engine->m_eventSystem->RegisterEvent(eventName, ptr);
}

void UnregisterEvent(std::string const& eventName, EventConnectionID connectionID)
{
	g_engine->m_eventSystem->UnregisterEvent(eventName, connectionID);
}

int FireEvent(std::string const& eventName)
{
	return g_engine->m_eventSystem->FireEvent(eventName);
}

int FireEvent(std::string const& eventName, EventArgs& args)
{
	return g_engine->m_eventSystem->FireEvent(eventName, args);
}
