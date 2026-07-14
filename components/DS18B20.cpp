#include "DS18B20.h"

#include <cmath>

#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define CMD_SKIP_ROM 0xCC
#define CMD_CONVERT_T 0x44
#define CMD_READ_SCRATCHPAD 0xBE

static portMUX_TYPE s_oneWireMux = portMUX_INITIALIZER_UNLOCKED;

DS18B20::DS18B20(gpio_num_t pin) : pino_(pin), ultimaTemperatura_(NAN) {
    gpio_reset_pin(pino_);
    pinoEntrada();
}

void DS18B20::pinoBaixo() {
    gpio_set_direction(pino_, GPIO_MODE_OUTPUT);
    gpio_set_level(pino_, 0);
}

void DS18B20::pinoEntrada() {
    gpio_set_direction(pino_, GPIO_MODE_INPUT);
    gpio_set_pull_mode(pino_, GPIO_PULLUP_ONLY);
}

bool DS18B20::reset() {
    taskENTER_CRITICAL(&s_oneWireMux);
    pinoBaixo();
    esp_rom_delay_us(480);
    pinoEntrada();
    esp_rom_delay_us(70);
    bool presenca = (gpio_get_level(pino_) == 0);
    esp_rom_delay_us(410);
    taskEXIT_CRITICAL(&s_oneWireMux);
    return presenca;
}

void DS18B20::escreveBit(bool bit) {
    taskENTER_CRITICAL(&s_oneWireMux);
    pinoBaixo();
    esp_rom_delay_us(bit ? 6 : 60);
    pinoEntrada();
    esp_rom_delay_us(bit ? 64 : 10);
    taskEXIT_CRITICAL(&s_oneWireMux);
}

bool DS18B20::leBit() {
    taskENTER_CRITICAL(&s_oneWireMux);
    pinoBaixo();
    esp_rom_delay_us(2);
    pinoEntrada();
    esp_rom_delay_us(11);
    bool valor = gpio_get_level(pino_);
    esp_rom_delay_us(47);
    taskEXIT_CRITICAL(&s_oneWireMux);
    return valor;
}

void DS18B20::escreveByte(uint8_t valor) {
    for (int i = 0; i < 8; i++) {
        escreveBit(valor & 0x01);
        valor >>= 1;
    }
}

uint8_t DS18B20::leByte() {
    uint8_t valor = 0;
    for (int i = 0; i < 8; i++) {
        valor >>= 1;
        if (leBit()) valor |= 0x80;
    }
    return valor;
}

uint8_t DS18B20::CRC(const uint8_t dados[], int tamanho) {
    uint8_t crc = 0;
    for (int i = 0; i < tamanho; i++) {
        uint8_t byte = dados[i];
        for (int j = 0; j < 8; j++) {
            uint8_t mix = (crc ^ byte) & 0x01;
            crc >>= 1;
            if (mix) crc ^= 0x8C;
            byte >>= 1;
        }
    }
    return crc;
}

bool DS18B20::tentaLer(float& resultado) {
    if (!reset()) {
        return false;
    }
    escreveByte(CMD_SKIP_ROM);
    escreveByte(CMD_CONVERT_T);
    vTaskDelay(pdMS_TO_TICKS(750));

    if (!reset()) {
        return false;
    }
    escreveByte(CMD_SKIP_ROM);
    escreveByte(CMD_READ_SCRATCHPAD);

    uint8_t scratchpad[9];
    for (uint8_t& byte : scratchpad) {
        byte = leByte();
    }

    if (CRC(scratchpad, 8) != scratchpad[8]) {
        return false;
    }

    int16_t bruto = (scratchpad[1] << 8) | scratchpad[0];
    resultado = bruto / 16.0f;
    return true;
}

float DS18B20::le() {
    constexpr int MAX_TENTATIVAS = 20;
    for (int tentativa = 0; tentativa < MAX_TENTATIVAS; tentativa++) {
        float resultado;
        if (tentaLer(resultado)) {
            ultimaTemperatura_ = resultado;
            return ultimaTemperatura_;
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
    printf("[DS18B20] Falha na leitura apos %d tentativas, mantendo ultimo valor valido.\n", MAX_TENTATIVAS);
    return ultimaTemperatura_;
}
