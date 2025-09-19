#pragma once

#include <thread>
#include <map>
#include <string>
#include <vector>
#include <functional>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

using EventCallback = std::function<void(json& data)>;

class EventChannel
{
public:
    void init();
    void subscribe(std::string event, EventCallback callback);
    // void publish();

private:
    void init_websocket();

private:
    std::map<std::string, EventCallback> events_;
};