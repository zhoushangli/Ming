#include "Engine/Core/EventSystem.hpp"

EventSystem::EventSystem(EventSystemConfig const& config)
{

}

EventSystem::~EventSystem()
{

}

void EventSystem::SubscribeEventCallbackFunction(std::string const& eventName, EventSystemCallbackFunctionPtr ptr)
{
    m_subscriptionListsByEventName[eventName].push_back(ptr);
}

void EventSystem::UnsubscribeEventCallbackFunction(std::string const& eventName, EventSystemCallbackFunctionPtr ptr)
{
    
}

// void EventSystem::FireEvent(std::string const& eventName, EventArgs& args)
// {
// 
// }

void EventSystem::FireEvent(std::string const& eventName)
{
    if (m_subscriptionListsByEventName.find(eventName) != m_subscriptionListsByEventName.end())
    {
        for (EventSystemCallbackFunctionPtr callbackPtr : m_subscriptionListsByEventName[eventName])
        {
            callbackPtr();
        }
    }
}
