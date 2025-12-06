#include "Engine/Core/EventSystem.hpp"

#include "Engine/Core/Engine.hpp"

EventSystem::EventSystem(EventSystemConfig const& config) : m_config(config)
{
}

EventSystem::~EventSystem()
{

}

void EventSystem::Startup()
{

}

void EventSystem::Shutdown()
{
    for (auto& pair : m_subscriptionListsByEventName)
    {
        pair.second.clear();
    }
    m_subscriptionListsByEventName.clear();
}

void EventSystem::BeginFrame()
{

}

void EventSystem::EndFrame()
{

}

void EventSystem::SubscribeEventCallbackFunction(std::string const& eventName, EventSystemCallbackFunctionPtr ptr)
{
    m_subscriptionListsByEventName[eventName].push_back(ptr);
}

void EventSystem::UnsubscribeEventCallbackFunction(std::string const& eventName, EventSystemCallbackFunctionPtr ptr)
{
    auto& callbackList = m_subscriptionListsByEventName[eventName];
    auto it = std::find(callbackList.begin(), callbackList.end(), ptr);
    if (it != callbackList.end())
    {
        callbackList.erase(it);
    }
}

int EventSystem::FireEvent(std::string const& eventName, EventArgs& args)
{
    auto& callbackList = m_subscriptionListsByEventName[eventName];
    int numCallbacks = 0;
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

void SubscribeEventCallbackFunction(std::string const& eventName, EventSystemCallbackFunctionPtr ptr)
{
    g_engine->m_eventSystem->SubscribeEventCallbackFunction(eventName, ptr);
}

void UnsubscribeEventCallbackFunction(std::string const& eventName, EventSystemCallbackFunctionPtr ptr)
{
    g_engine->m_eventSystem->UnsubscribeEventCallbackFunction(eventName, ptr);
}

int FireEvent(std::string const& eventName)
{
    return g_engine->m_eventSystem->FireEvent(eventName);
}

int FireEvent(std::string const& eventName, EventArgs& args)
{
    return g_engine->m_eventSystem->FireEvent(eventName, args);
}
