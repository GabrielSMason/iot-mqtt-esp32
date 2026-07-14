#pragma once

#include <functional>
#include <string>
#include <vector>

#include "mqtt_client.h"

class MQTT {
public:
    using Handler = std::function<void(const std::string& topico, const std::string& payload)>;

    explicit MQTT(const std::string& uri);

    void conecta();

    void sub(const std::string& topico);

    void pub(const std::string& topico, const std::string& payload);
    
    void subHandler(Handler handler);

private:
    std::string uri_;
    esp_mqtt_client_handle_t cliente_ = nullptr;
    Handler handler_;
    std::vector<std::string> topicosAssinados_;

    static void eventoHandler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
    void resubscreveTudo();
};
