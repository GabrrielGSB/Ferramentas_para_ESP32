#include "esp_log.h"
#include "Network.hpp"

extern "C" void app_main(void) {
    static const char* TAG = "main";

    // Cria a rede em modo AP_STA: AP local + conexão como cliente externo simultaneamente
    Network net(ConfigRede::AP_STA);

    // Sobe o AP e, em seguida, tenta conectar na rede externa
    net.iniciarAPSTA(
        "ESP32-Config", "12345678",   // SSID e senha do AP local
        "Jefloyd", "cre@tivo" // SSID e senha da rede externa (STA)
    );

    ESP_LOGI(TAG, "AP ativo. Procure a rede \"ESP32-Config\" no seu celular.");
    ESP_LOGI(TAG, "IP padrao do AP: 192.168.4.1");
    ESP_LOGI(TAG, "Aguardando conexao STA com a rede externa...");

    if (net.aguardarConexao(15000)) {
        ESP_LOGI(TAG, "STA conectado com sucesso a rede externa!");
    } else {
        ESP_LOGW(TAG, "STA nao conseguiu conectar a rede externa (timeout ou falha).");
        ESP_LOGW(TAG, "O AP local continua ativo mesmo assim.");
    }

    // app_main pode retornar; tudo continua rodando via as tasks do FreeRTOS/ESP-IDF
}