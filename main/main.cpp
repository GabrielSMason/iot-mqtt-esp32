#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <string>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "DS18B20.h"
#include "MQTT.h"
#include "WIFI.h"

#define WIFI_SSID "Gabriel"
#define WIFI_SENHA "02152628"
#define MQTT_URI "mqtt://broker.emqx.io"

#define TOPICO_CONFIGURA_ALTA "/configura/alta"
#define TOPICO_CONFIGURA_BAIXA "/configura/baixa"
#define TOPICO_INFORMA_TEMPERATURA "/informa/temperaturaCorrente"
#define TOPICO_INFORMA_LIMITE_ALTA "/informa/limiteAlta"
#define TOPICO_INFORMA_LIMITE_BAIXA "/informa/limiteBaixa"
#define TOPICO_RESPONDE_TEMPERATURA "/responde/temperaturaCorrente"
#define TOPICO_RESPONDE_LIMITE_ALTA "/responde/limiteAlta"
#define TOPICO_RESPONDE_LIMITE_BAIXA "/responde/limiteBaixa"
#define TOPICO_ALERTA_ALTA "/alerta/temperaturaAlta"
#define TOPICO_ALERTA_BAIXA "/alerta/temperaturaBaixa"

static std::atomic<float> limiteAlto{30.0f};
static std::atomic<float> limiteBaixo{10.0f};
static std::atomic<float> ultimaTemperatura{0.0f};

static DS18B20 meuSensor;
static MQTT meuMqtt(MQTT_URI);

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
        meuMqtt.pub(TOPICO_RESPONDE_TEMPERATURA, buffer);
    } else if (topico == TOPICO_INFORMA_LIMITE_ALTA) {
        char buffer[16];
        std::snprintf(buffer, sizeof(buffer), "%.2f", limiteAlto.load());
        meuMqtt.pub(TOPICO_RESPONDE_LIMITE_ALTA, buffer);
    } else if (topico == TOPICO_INFORMA_LIMITE_BAIXA) {
        char buffer[16];
        std::snprintf(buffer, sizeof(buffer), "%.2f", limiteBaixo.load());
        meuMqtt.pub(TOPICO_RESPONDE_LIMITE_BAIXA, buffer);
    }
}

extern "C" void app_main() {
    WIFI meuWiFi(WIFI_SSID, WIFI_SENHA);
    meuWiFi.conecta();

    meuMqtt.conecta();
    meuMqtt.subHandler(tratadorMensagens);

    meuMqtt.sub(TOPICO_CONFIGURA_ALTA);
    meuMqtt.sub(TOPICO_CONFIGURA_BAIXA);
    meuMqtt.sub(TOPICO_INFORMA_TEMPERATURA);
    meuMqtt.sub(TOPICO_INFORMA_LIMITE_ALTA);
    meuMqtt.sub(TOPICO_INFORMA_LIMITE_BAIXA);

    while (true) {
        float temperatura = meuSensor.le();
        ultimaTemperatura = temperatura;

        char buffer[16];
        std::snprintf(buffer, sizeof(buffer), "%.2f", temperatura);
        printf("Temperatura lida: %s C\n", buffer);

        if (temperatura > limiteAlto.load()) {
            meuMqtt.pub(TOPICO_ALERTA_ALTA, buffer);
        } else if (temperatura < limiteBaixo.load()) {
            meuMqtt.pub(TOPICO_ALERTA_BAIXA, buffer);
        }

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
