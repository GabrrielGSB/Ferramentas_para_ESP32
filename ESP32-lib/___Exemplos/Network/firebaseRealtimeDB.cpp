#include <cstdio>
#include <string>
#include <ctime>
#include "esp_log.h"

// Componentes da sua biblioteca
#include "Network.hpp"
#include "JSON.hpp"
#include "Tempo.hpp"
#include "Random.hpp"

using namespace std;

namespace {
    constexpr const char* TAG = "FirebaseApp";

    // =========================================================================
    // CONFIGURAÇÕES DE REDE E FIREBASE
    // =========================================================================
    constexpr const char* SSID_WIFI     = "myssid";
    constexpr const char* SENHA_WIFI    = "21022002";

    // Link oficial do Realtime Database (sem https://)
    constexpr const char* FIREBASE_HOST = "app-ionos-default-rtdb.firebaseio.com";

    // Identificador único do dispositivo
    constexpr const char* DEVICE_ID     = "AQ-2026-6767";

    // =========================================================================
    // TEMPORIZADORES (em milissegundos)
    // =========================================================================
    constexpr uint64_t INTERVALO_ATUAL_MS     = 30000;    // 30 segundos
    constexpr uint64_t INTERVALO_HISTORICO_MS = 3600000;  // 1 hora
}

string gerarPayloadJSON() {
    float corrente             = Random::floatRange(0.50f, 1.20f);  // 0.50A a 1.20A
    float tensao               = Random::floatRange(11.5f, 12.5f);  // 11.5V a 12.5V
    float phEntrada            = Random::floatRange(7.0f, 8.5f);    // pH 7.0 a 8.5
    float phSaida              = Random::floatRange(6.5f, 7.5f);    // pH 6.5 a 7.5
    float condutividadeEntrada = Random::floatRange(400.0f, 500.0f);// 400 a 500 uS/cm
    float condutividadeSaida   = Random::floatRange(30.0f, 50.0f);  // 30 a 50 uS/cm
    int64_t timestamp          = static_cast<int64_t>(time(nullptr));

    JSON doc;
    doc.set("corrente",              static_cast<double>(corrente))
       .set("tensao",                static_cast<double>(tensao))
       .set("ph_entrada",            static_cast<double>(phEntrada))
       .set("ph_saida",              static_cast<double>(phSaida))
       .set("condutividade_entrada", static_cast<double>(condutividadeEntrada))
       .set("condutividade_saida",   static_cast<double>(condutividadeSaida))
       .set("ultima_atualizacao",    timestamp);

    return doc.toString();
}

void tarefaTelemetriaAtual(void* pvParameters) {
    auto* rede = static_cast<Network*>(pvParameters);
    const string urlAtual = string("https://") + FIREBASE_HOST + "/telemetria_atual/" + DEVICE_ID + ".json";

    TickType_t ultimoMomento = xTaskGetTickCount();
    const TickType_t periodoTicks = pdMS_TO_TICKS(INTERVALO_ATUAL_MS); // 30 segundos

    while (true) {
        ESP_LOGI(TAG, "[HTTP] Preparando atualizacao de telemetria_atual (30s)...");
        string payload = gerarPayloadJSON();

        int statusCode = 0;
        string resposta = rede->http.patch(urlAtual, payload, "application/json", &statusCode);

        if (statusCode >= 200 && statusCode < 300) {
            ESP_LOGI(TAG, "[HTTP] PATCH atualizado com sucesso! Codigo: %d", statusCode);
        } else {
            ESP_LOGE(TAG, "[HTTP] Erro ao enviar PATCH. Codigo: %d", statusCode);
        }

        vTaskDelayUntil(&ultimoMomento, periodoTicks);
    }
}

void tarefaHistorico(void* pvParameters) {
    auto* rede = static_cast<Network*>(pvParameters);
    const string urlHistorico = string("https://") + FIREBASE_HOST + "/historico/" + DEVICE_ID + ".json";

    TickType_t ultimoMomento = xTaskGetTickCount();
    const TickType_t periodoTicks = pdMS_TO_TICKS(INTERVALO_HISTORICO_MS); // 1 hora

    while (true) {
        ESP_LOGI(TAG, "[HTTP] Gravando novo ponto no historico por hora (1h)...");
        string payload = gerarPayloadJSON();

        int statusCode = 0;
        string resposta = rede->http.post(urlHistorico, payload, "application/json", &statusCode);

        if (statusCode >= 200 && statusCode < 300) {
            ESP_LOGI(TAG, "[HTTP] POST historico salvo com sucesso! Codigo: %d", statusCode);
        } else {
            ESP_LOGE(TAG, "[HTTP] Erro ao enviar POST para historico. Codigo: %d", statusCode);
        }

        vTaskDelayUntil(&ultimoMomento, periodoTicks);
    }
}

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "Iniciando sistema e conexao Wi-Fi...");

    static Network rede;
    rede.conectar(SSID_WIFI, SENHA_WIFI);

    if (rede.aguardarConexao(10000) == ESP_OK) {
        ESP_LOGI(TAG, "WiFi Conectado! IP: %s", rede.obterIP().c_str());

        // 1. Sincroniza o relógio e só inicia as tasks quando a data for válida
        if (rede.sincronizarHorario()) {
            // Task 1: Envio do estado atual a cada 30 segundos
            xTaskCreate(
                tarefaTelemetriaAtual,
                "task_telemetria_atual",
                6144,
                &rede,
                5,
                nullptr
            );

            // Task 2: Registro de histórico a cada 1 hora
            xTaskCreate(
                tarefaHistorico,
                "task_historico",
                6144,
                &rede,
                5,
                nullptr
            );
        } else {
            ESP_LOGE(TAG, "Telemetria nao iniciada devido a falta de sincronismo de horario.");
        }
    } else {
        ESP_LOGE(TAG, "Falha ao conectar na rede Wi-Fi a tempo.");
    }
}
