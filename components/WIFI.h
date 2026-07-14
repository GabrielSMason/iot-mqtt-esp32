#pragma once

#include <string>

#include "esp_event.h"

class WIFI {
public:
    WIFI(const std::string& ssid, const std::string& senha);

    void conecta();
private:
    std::string ssid_;
    std::string senha_;

    static void eventoHandler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

    esp_event_handler_instance_t instanciaWifiEvent_ = nullptr;
    esp_event_handler_instance_t instanciaIpEvent_ = nullptr;
};
