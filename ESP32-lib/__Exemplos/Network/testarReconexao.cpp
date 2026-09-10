#include <string>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "Network.hpp"

static const char* TAG = "ExemploReconexao";

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "--- Inicializando Teste de Rede e Reconexao Nao Bloqueante ---");

    Network wifi(ConfigRede::STA);

    wifi.definirHostname("esp32-node");

 
    const std::string ssid = "SUA_REDE_WIFI";
    const std::string senha = "SUA_SENHA_WIFI";
    const int maxTentativas = 6;

    ESP_LOGI(TAG, "Iniciando conexao para SSID: %s (max tentativas: %d)", ssid.c_str(), maxTentativas);
    wifi.conectar(ssid, senha, maxTentativas);

    uint32_t segundos = 0;
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        segundos++;

        if (wifi.estaConectado()) {
            ESP_LOGI(TAG, "[%lus] CONECTADO! Dados da Conexao:", (unsigned long)segundos);
            ESP_LOGI(TAG, "  -> IP:      %s", wifi.obterIP().c_str());
            ESP_LOGI(TAG, "  -> Mascara: %s", wifi.obterMascara().c_str());
            ESP_LOGI(TAG, "  -> Gateway: %s", wifi.obterGateway().c_str());
            ESP_LOGI(TAG, "  -> MAC:     %s", wifi.obterMAC().c_str());
            ESP_LOGI(TAG, "  -> RSSI:    %d dBm", wifi.obterRSSI());
        } else {
            ESP_LOGW(TAG, "[%lus] Desconectado. (O timer em background esta gerenciando as tentativas via backoff)",
                     (unsigned long)segundos);
        }
    }
}
