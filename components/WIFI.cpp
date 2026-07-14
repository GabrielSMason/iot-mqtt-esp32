#include "WIFI.h"

#include <cstring>

#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "nvs_flash.h"

#define TAG "WIFI"
#define WIFI_CONECTADO_BIT BIT0
static EventGroupHandle_t s_eventGroup = nullptr;

WIFI::WIFI(const std::string& ssid, const std::string& senha)
    : ssid_(ssid), senha_(senha) {}

void WIFI::eventoHandler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        auto* evento = static_cast<wifi_event_sta_disconnected_t*>(event_data);
        ESP_LOGW(TAG, "Wi-Fi desconectado (motivo=%d), tentando reconectar...", evento->reason);
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        auto* evento = static_cast<ip_event_got_ip_t*>(event_data);
        ESP_LOGI(TAG, "Conectado! IP obtido: " IPSTR, IP2STR(&evento->ip_info.ip));
        xEventGroupSetBits(s_eventGroup, WIFI_CONECTADO_BIT);
    }
}

void WIFI::conecta() {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    s_eventGroup = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfgInit = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfgInit));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &WIFI::eventoHandler, nullptr, &instanciaWifiEvent_));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &WIFI::eventoHandler, nullptr, &instanciaIpEvent_));

    wifi_config_t cfg = {};
    std::strncpy(reinterpret_cast<char*>(cfg.sta.ssid), ssid_.c_str(), sizeof(cfg.sta.ssid) - 1);
    std::strncpy(reinterpret_cast<char*>(cfg.sta.password), senha_.c_str(), sizeof(cfg.sta.password) - 1);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &cfg));
    ESP_ERROR_CHECK(esp_wifi_start());

    esp_wifi_set_max_tx_power(34);

    esp_wifi_set_ps(WIFI_PS_NONE);

    ESP_LOGI(TAG, "Conectando ao Wi-Fi '%s'...", ssid_.c_str());
    xEventGroupWaitBits(s_eventGroup, WIFI_CONECTADO_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
}
