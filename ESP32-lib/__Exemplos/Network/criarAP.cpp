#include "esp_log.h"
#include "Network.hpp"

extern "C" void app_main(void) {
    static const char* TAG = "main";

    Network net(ConfigRede::AP);

    net.iniciarAP("ESP32-Teste", "12345678");

    ESP_LOGI(TAG, "AP ativo. Procure a rede \"ESP32-Teste\" no seu celular.");
    ESP_LOGI(TAG, "IP padrao do AP: 192.168.4.1");
}