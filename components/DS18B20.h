#pragma once

#include "driver/gpio.h"

class DS18B20 {
public:
    explicit DS18B20(gpio_num_t pin = GPIO_NUM_33);

    float le();

private:
    gpio_num_t pino_;
    float ultimaTemperatura_;

    bool reset();
    void escreveBit(bool bit);
    bool leBit();
    void escreveByte(uint8_t valor);
    uint8_t leByte();
    uint8_t CRC(const uint8_t dados[], int tamanho);
    bool tentaLer(float& resultado);

    void pinoBaixo();
    void pinoEntrada();
};
