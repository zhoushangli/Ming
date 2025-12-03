#pragma once

#include <string>
#include <vector>
#include <map>

typedef void (*EventSystemCallbackFunctionPtr)();

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
/*    void FireEvent(std::string const& eventName, EventArgs& args);*/
    void FireEvent(std::string const& eventName);

protected:
    EventSystemConfig m_config;
    std::map<std::string, std::vector<EventSystemCallbackFunctionPtr>> m_subscriptionListsByEventName;
};