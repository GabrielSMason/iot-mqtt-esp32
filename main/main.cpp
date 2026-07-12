#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <string>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "DS18B20.h"
#include "MqttClient.h"
#include "WifiManager.h"

static constexpr const char* WIFI_SSID = "Gabriel";
static constexpr const char* WIFI_SENHA = "02152628";
static constexpr const char* MQTT_URI = "mqtt://broker.emqx.io";

// Prefixo único para isolar os tópicos deste dispositivo dos demais clientes
// que também usam o broker público broker.emqx.io.
static constexpr const char* PREFIXO = "/gabriel_t2";

static const std::string TOPICO_CONFIGURA_ALTA = std::string(PREFIXO) + "/configura/alta";
static const std::string TOPICO_CONFIGURA_BAIXA = std::string(PREFIXO) + "/configura/baixa";
static const std::string TOPICO_INFORMA_TEMPERATURA = std::string(PREFIXO) + "/informa/temperaturaCorrente";
static const std::string TOPICO_INFORMA_LIMITE_ALTA = std::string(PREFIXO) + "/informa/limiteAlta";
static const std::string TOPICO_INFORMA_LIMITE_BAIXA = std::string(PREFIXO) + "/informa/limiteBaixa";
static const std::string TOPICO_RESPONDE_TEMPERATURA = std::string(PREFIXO) + "/responde/temperaturaCorrente";
static const std::string TOPICO_RESPONDE_LIMITE_ALTA = std::string(PREFIXO) + "/responde/limiteAlta";
static const std::string TOPICO_RESPONDE_LIMITE_BAIXA = std::string(PREFIXO) + "/responde/limiteBaixa";
static const std::string TOPICO_ALERTA_ALTA = std::string(PREFIXO) + "/alerta/temperaturaAlta";
static const std::string TOPICO_ALERTA_BAIXA = std::string(PREFIXO) + "/alerta/temperaturaBaixa";

static constexpr TickType_t CICLO_LEITURA = pdMS_TO_TICKS(5000);

// Atomic pois são lidos/escritos tanto pela task do MQTT (callback de
// tratadorMensagens) quanto pelo loop principal em app_main.
// Limites configuráveis remotamente via /configura/alta e /configura/baixa.
static std::atomic<float> limiteAlto{30.0f};
static std::atomic<float> limiteBaixo{10.0f};
static std::atomic<float> ultimaTemperatura{0.0f};

static DS18B20 sensor;
static MqttClient mqtt(MQTT_URI);

static void tratadorMensagens(const std::string& topico, const std::string& payload) {
    printf("Mensagem recebida em %s: %s\n", topico.c_str(), payload.c_str());

    if (topico == TOPICO_CONFIGURA_ALTA) {
        limiteAlto = std::atof(payload.c_str());
        printf("Limite alto atualizado para %.2f\n", limiteAlto.load());
    } else if (topico == TOPICO_CONFIGURA_BAIXA) {
        limiteBaixo = std::atof(payload.c_str());
        printf("Limite baixo atualizado para %.2f\n", limiteBaixo.load());
    } else if (topico == TOPICO_INFORMA_TEMPERATURA) {
        char buffer[16];
        std::snprintf(buffer, sizeof(buffer), "%.2f", ultimaTemperatura.load());
        mqtt.pub(TOPICO_RESPONDE_TEMPERATURA, buffer);
    } else if (topico == TOPICO_INFORMA_LIMITE_ALTA) {
        char buffer[16];
        std::snprintf(buffer, sizeof(buffer), "%.2f", limiteAlto.load());
        mqtt.pub(TOPICO_RESPONDE_LIMITE_ALTA, buffer);
    } else if (topico == TOPICO_INFORMA_LIMITE_BAIXA) {
        char buffer[16];
        std::snprintf(buffer, sizeof(buffer), "%.2f", limiteBaixo.load());
        mqtt.pub(TOPICO_RESPONDE_LIMITE_BAIXA, buffer);
    }
}

extern "C" void app_main() {
    WifiManager wifi(WIFI_SSID, WIFI_SENHA);
    wifi.conecta();

    mqtt.conecta();
    mqtt.subHandler(tratadorMensagens);

    mqtt.sub(TOPICO_CONFIGURA_ALTA);
    mqtt.sub(TOPICO_CONFIGURA_BAIXA);
    mqtt.sub(TOPICO_INFORMA_TEMPERATURA);
    mqtt.sub(TOPICO_INFORMA_LIMITE_ALTA);
    mqtt.sub(TOPICO_INFORMA_LIMITE_BAIXA);

    while (true) {
        float temperatura = sensor.le();
        ultimaTemperatura = temperatura;

        char buffer[16];
        std::snprintf(buffer, sizeof(buffer), "%.2f", temperatura);
        printf("Temperatura lida: %s C\n", buffer);

        // Publica a cada ciclo (não só sob consulta), para acompanhamento
        // contínuo em clientes MQTT como o MQTT Explorer.
        mqtt.pub(TOPICO_RESPONDE_TEMPERATURA, buffer);

        if (temperatura > limiteAlto.load()) {
            mqtt.pub(TOPICO_ALERTA_ALTA, buffer);
        } else if (temperatura < limiteBaixo.load()) {
            mqtt.pub(TOPICO_ALERTA_BAIXA, buffer);
        }

        vTaskDelay(CICLO_LEITURA);
    }
}
