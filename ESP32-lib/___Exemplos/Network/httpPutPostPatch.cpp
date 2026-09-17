#include <cstdio>
#include "Network.hpp"
#include "JSON.hpp"
#include "Tempo.hpp"
#include "esp_log.h"

using namespace std;

namespace {
    constexpr const char* TAG = "ExemploHTTP";
}

void tarefaEnvioHTTP(void* pvParameters) {
    auto* rede = static_cast<Network*>(pvParameters);
    int contador = 0;

    while (true) {
        contador++;
        ESP_LOGI(TAG, "--- Iniciando ciclo de envio #%d ---", contador);

        // 1. Criar um payload JSON com a classe JSON
        JSON doc;
        doc.set("dispositivo", "ESP32-S3")
           .set("ciclo",        contador)
           .set("temperatura",  25.4 + (contador % 5))
           .set("uptime_ms",    (int64_t)millis());

        string payloadJson = doc.toString();
        ESP_LOGI(TAG, "Payload gerado: %s", payloadJson.c_str());

        // 2. Enviar via HTTP POST através do submódulo rede->http
        int statusPost = 0;
        string respostaPost = rede->http.post(
            "https://httpbin.org/post", 
            payloadJson, 
            "application/json", 
            &statusPost
        );

        ESP_LOGI(TAG, "[POST] Retorno HTTP Status: %d", statusPost);
        if (statusPost >= 200 && statusPost < 300) {
            ESP_LOGI(TAG, "[POST] Sucesso! Tamanho da resposta: %d bytes", (int)respostaPost.size());
        } else {
            ESP_LOGE(TAG, "[POST] Falha na requisicao!");
        }

        // Aguarda 3 segundos
        delay_s(3);

        // 3. Enviar atualização parcial via HTTP PATCH
        JSON patchDoc;
        patchDoc.set("status", "operando_normal")
                .set("ultimo_ping", (int64_t)millis());

        int statusPatch = 0;
        string respostaPatch = rede->http.patch(
            "https://httpbin.org/patch", 
            patchDoc.toString(), 
            "application/json", 
            &statusPatch
        );

        ESP_LOGI(TAG, "[PATCH] Retorno HTTP Status: %d", statusPatch);

        // Aguarda 3 segundos
        delay_s(3);

        // 4. Substituição / Atualização completa via HTTP PUT
        JSON putDoc;
        putDoc.set("dispositivo", "ESP32-S3")
              .set("modo_operacao", "producao")
              .set("versao_firmware", "1.0.0");

        int statusPut = 0;
        string respostaPut = rede->http.put(
            "https://httpbin.org/put",
            putDoc.toString(),
            "application/json",
            &statusPut
        );

        ESP_LOGI(TAG, "[PUT] Retorno HTTP Status: %d", statusPut);

        // Aguarda 15 segundos antes do próximo ciclo
        delay_s(15);
    }
}

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "Inicializando conexao Wi-Fi...");
    static Network rede;

    // Substitua pelo SSID e senha da sua rede local
    rede.conectar("myssid", "21022002");

    if (rede.aguardarConexao(10000) == ESP_OK) {
        ESP_LOGI(TAG, "Conectado com sucesso! IP: %s", rede.obterIP().c_str());

        xTaskCreate(
            tarefaEnvioHTTP,
            "tarefa_http",
            8192,
            &rede,
            5,
            nullptr
        );
    } else {
        ESP_LOGE(TAG, "Falha ao conectar no Wi-Fi a tempo.");
    }
}
