#include <cstdio>
#include <string>
#include <ctime>
#include "esp_log.h"
#include "esp_random.h"
#include "esp_netif_sntp.h"

// Componentes da sua biblioteca
#include "Network.hpp"
#include "JSON.hpp"
#include "Tempo.hpp"

using namespace std;

namespace {
    constexpr const char* TAG = "FirebaseApp";

    // =========================================================================
    // CONFIGURAÇÕES DE REDE E FIREBASE
    // =========================================================================
    constexpr const char* SSID_WIFI     = "Fatima";
    constexpr const char* SENHA_WIFI    = "fatimaoliveira";

    // Link oficial do Realtime Database (sem https://)
    constexpr const char* FIREBASE_HOST = "app-ionos-default-rtdb.firebaseio.com";

    // Identificador único do dispositivo
    constexpr const char* DEVICE_ID     = "AQ-2026-3333";

    // =========================================================================
    // TEMPORIZADORES (em milissegundos)
    // =========================================================================
    constexpr uint64_t INTERVALO_ATUAL_MS     = 30000;    // 30 segundos
    constexpr uint64_t INTERVALO_HISTORICO_MS = 3600000;  // 1 hora
}

/**
 * @brief Sincroniza o relógio interno do ESP32 com servidores NTP mundiais e do Google.
 *        Isso é OBRIGATÓRIO para validação de certificados SSL/TLS (HTTPS).
 */
static bool sincronizarHorario() {
    ESP_LOGI(TAG, "Iniciando sincronizacao de relogio via NTP...");

    // Usa o servidor oficial do Google (time.google.com)
    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("time.google.com");
    esp_netif_sntp_init(&config);

    // Aguarda até obter uma data válida (> ano 2024)
    int tentativas = 0;
    while (tentativas < 15) {
        time_t agora = time(nullptr);
        struct tm info = {};
        localtime_r(&agora, &info);

        if (info.tm_year >= (2024 - 1900)) {
            ESP_LOGI(TAG, "Data sincronizada: %02d/%02d/%04d %02d:%02d:%02d (Timestamp: %lld)",
                     info.tm_mday, info.tm_mon + 1, info.tm_year + 1900,
                     info.tm_hour, info.tm_min, info.tm_sec, (long long)agora);
            return true;
        }

        ESP_LOGI(TAG, "Aguardando sincronizacao NTP... (%d/15)", tentativas + 1);
        delay_s(1);
        tentativas++;
    }

    ESP_LOGE(TAG, "FALHA CRITICA: Nao foi possivel sincronizar o relogio. Certificados SSL irao falhar!");
    return false;
}

static float randomFloat(float min, float max) {
    uint32_t r = esp_random();
    return min + (static_cast<float>(r) / static_cast<float>(UINT32_MAX)) * (max - min);
}

string gerarPayloadJSON() {
    float corrente             = randomFloat(0.50f, 1.20f);  // 0.50A a 1.20A
    float tensao               = randomFloat(11.5f, 12.5f);  // 11.5V a 12.5V
    float phEntrada            = randomFloat(7.0f, 8.5f);    // pH 7.0 a 8.5
    float phSaida              = randomFloat(6.5f, 7.5f);    // pH 6.5 a 7.5
    float condutividadeEntrada = randomFloat(400.0f, 500.0f);// 400 a 500 uS/cm
    float condutividadeSaida   = randomFloat(30.0f, 50.0f);  // 30 a 50 uS/cm
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

void tarefaTelemetria(void* pvParameters) {
    auto* rede = static_cast<Network*>(pvParameters);

    const string urlAtual = string("https://") + FIREBASE_HOST + "/telemetria_atual/" + DEVICE_ID + ".json";
    const string urlHistorico = string("https://") + FIREBASE_HOST + "/historico/" + DEVICE_ID + ".json";

    uint64_t ultimoEnvioAtual = 0;
    uint64_t ultimoEnvioHistorico = 0;

    while (true) {
        uint64_t tempoAtual = millis();

        // TIMER 1: Envia Estado Atual (PATCH) a cada 30 segundos
        if (tempoAtual - ultimoEnvioAtual >= INTERVALO_ATUAL_MS) {
            ESP_LOGI(TAG, "[HTTP] Preparando atualizacao de telemetria_atual (30s)...");
            string payload = gerarPayloadJSON();

            int statusCode = 0;
            string resposta = rede->http.patch(urlAtual, payload, "application/json", &statusCode);

            if (statusCode >= 200 && statusCode < 300) {
                ESP_LOGI(TAG, "[HTTP] PATCH atualizado com sucesso! Codigo: %d", statusCode);
            } else {
                ESP_LOGE(TAG, "[HTTP] Erro ao enviar PATCH. Codigo: %d", statusCode);
            }

            ultimoEnvioAtual = tempoAtual;
        }

        // TIMER 2: Grava Ponto no Histórico (POST) a cada 1 hora
        if (tempoAtual - ultimoEnvioHistorico >= INTERVALO_HISTORICO_MS) {
            ESP_LOGI(TAG, "[HTTP] Gravando novo ponto no historico por hora (1h)...");
            string payload = gerarPayloadJSON();

            int statusCode = 0;
            string resposta = rede->http.post(urlHistorico, payload, "application/json", &statusCode);

            if (statusCode >= 200 && statusCode < 300) {
                ESP_LOGI(TAG, "[HTTP] POST historico salvo com sucesso! Codigo: %d", statusCode);
            } else {
                ESP_LOGE(TAG, "[HTTP] Erro ao enviar POST para historico. Codigo: %d", statusCode);
            }

            ultimoEnvioHistorico = tempoAtual;
        }

        delay_ms(500);
    }
}

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "Iniciando sistema e conexao Wi-Fi...");

    static Network rede;
    rede.conectar(SSID_WIFI, SENHA_WIFI);

    if (rede.aguardarConexao(10000) == ESP_OK) {
        ESP_LOGI(TAG, "WiFi Conectado! IP: %s", rede.obterIP().c_str());

        // 1. Sincroniza o relógio e só inicia a task quando a data for válida
        if (sincronizarHorario()) {
            xTaskCreate(
                tarefaTelemetria,
                "tarefa_telemetria",
                8192,
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
