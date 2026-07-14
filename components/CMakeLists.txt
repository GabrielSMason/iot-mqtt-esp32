idf_component_register(
    SRCS "DS18B20.cpp" "WIFI.cpp" "MQTT.cpp"
    INCLUDE_DIRS "."
    REQUIRES esp_driver_gpio esp_event mqtt
    PRIV_REQUIRES esp_wifi esp_netif nvs_flash
)
