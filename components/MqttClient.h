#pragma once

#include <functional>
#include <string>
#include <vector>

#include "mqtt_client.h"

class MqttClient {
public:
    // Assinatura do tratador de mensagens recebidas: tópico e payload.
    using Handler = std::function<void(const std::string& topico, const std::string& payload)>;

    explicit MqttClient(const std::string& uri);

    // Conecta ao broker e bloqueia até o evento de conexão ocorrer.
    void conecta();

    // Assina um tópico (bloqueia até a conexão estar estabelecida).
    void sub(const std::string& topico);

    void pub(const std::string& topico, const std::string& payload);

    // Define a função chamada para toda mensagem recebida nos tópicos assinados.
    void subHandler(Handler handler);

private:
    std::string uri_;
    esp_mqtt_client_handle_t cliente_ = nullptr;
    Handler handler_;
    std::vector<std::string> topicosAssinados_;

    static void eventoHandler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
    void resubscreveTudo();
};
