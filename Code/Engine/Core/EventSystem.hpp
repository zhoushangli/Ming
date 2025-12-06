#pragma once

#include "Engine/Core/NamedStrings.hpp"

#include <string>
#include <vector>
#include <map>

using EventSystemCallbackFunctionPtr = bool (*)(NamedStrings&);

struct EventSystemConfig
{
    bool m_isEnable = true;
};

class EventSystem
{
public:
    EventSystem(EventSystemConfig const& config);
    ~EventSystem();

    void Startup();
    void Shutdown();
    void BeginFrame();
    void EndFrame();

    void SubscribeEventCallbackFunction(std::string const& eventName, EventSystemCallbackFunctionPtr ptr);
    void UnsubscribeEventCallbackFunction(std::string const& eventName, EventSystemCallbackFunctionPtr ptr);
    int  FireEvent(std::string const& eventName, NamedStrings& args);
    int  FireEvent(std::string const& eventName);

protected:
    EventSystemConfig m_config;
    std::map<std::string, std::vector<EventSystemCallbackFunctionPtr>> m_subscriptionListsByEventName;
};

void SubscribeEventCallbackFunction(std::string const& eventName, EventSystemCallbackFunctionPtr ptr);
void UnsubscribeEventCallbackFunction(std::string const& eventName, EventSystemCallbackFunctionPtr ptr);
int FireEvent(std::string const& eventName);
int FireEvent(std::string const& eventName, NamedStrings& args);