#include "MQTT.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#define TAG "MQTT"
#define MQTT_CONECTADO_BIT BIT0
static EventGroupHandle_t s_eventGroup = nullptr;

MQTT::MQTT(const std::string& uri) : uri_(uri) {
    s_eventGroup = xEventGroupCreate();
}

void MQTT::eventoHandler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    auto* self = static_cast<MQTT*>(arg);
    auto* evento = static_cast<esp_mqtt_event_handle_t>(event_data);

    switch (evento->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT conectado!");
            xEventGroupSetBits(s_eventGroup, MQTT_CONECTADO_BIT);
            self->resubscreveTudo();
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "MQTT desconectado.");
            xEventGroupClearBits(s_eventGroup, MQTT_CONECTADO_BIT);
            break;
        case MQTT_EVENT_DATA: {
            std::string topico(evento->topic, evento->topic_len);
            std::string payload(evento->data, evento->data_len);
            if (self->handler_) {
                self->handler_(topico, payload);
            }
            break;
        }
        default:
            ESP_LOGD(TAG, "Evento MQTT id:%d", evento->event_id);
            break;
    }
}

void MQTT::conecta() {
    esp_mqtt_client_config_t cfg = {};
    cfg.broker.address.uri = uri_.c_str();

    cliente_ = esp_mqtt_client_init(&cfg);
    esp_mqtt_client_register_event(cliente_, MQTT_EVENT_ANY, &MQTT::eventoHandler, this);
    esp_mqtt_client_start(cliente_);

    ESP_LOGI(TAG, "Conectando ao broker '%s'...", uri_.c_str());
    xEventGroupWaitBits(s_eventGroup, MQTT_CONECTADO_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
}

void MQTT::sub(const std::string& topico) {
    xEventGroupWaitBits(s_eventGroup, MQTT_CONECTADO_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
    topicosAssinados_.push_back(topico);
    esp_mqtt_client_subscribe(cliente_, topico.c_str(), 0);
}

void MQTT::resubscreveTudo() {
    for (const std::string& topico : topicosAssinados_) {
        esp_mqtt_client_subscribe(cliente_, topico.c_str(), 0);
    }
}

void MQTT::pub(const std::string& topico, const std::string& payload) {
    esp_mqtt_client_publish(cliente_, topico.c_str(), payload.c_str(), payload.size(), 0, 0);
}

void MQTT::subHandler(Handler handler) {
    handler_ = std::move(handler);
}
