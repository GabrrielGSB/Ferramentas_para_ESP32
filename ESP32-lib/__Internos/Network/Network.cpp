#include "Network.hpp"

#include <cstring>

#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"

namespace {
    constexpr const char* TAG = "Network";
    constexpr uint16_t MAX_AP_RECORDS = 20;

    // Converte o enum de autenticacao do IDF para uma string legivel.
    const char* authModeToString(wifi_auth_mode_t authMode) {
        switch (authMode) {
            case WIFI_AUTH_OPEN:            return "OPEN";
            case WIFI_AUTH_WEP:             return "WEP";
            case WIFI_AUTH_WPA_PSK:         return "WPA_PSK";
            case WIFI_AUTH_WPA2_PSK:        return "WPA2_PSK";
            case WIFI_AUTH_WPA_WPA2_PSK:    return "WPA_WPA2_PSK";
            case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2_ENTERPRISE";
            case WIFI_AUTH_WPA3_PSK:        return "WPA3_PSK";
            case WIFI_AUTH_WPA2_WPA3_PSK:   return "WPA2_WPA3_PSK";
            default:                        return "UNKNOWN";
        }
    }
}

Network::Network()
    : m_initialized(false),
      ssid(),
      senha(),
      maxTentativas(0),
      tentativas(0) {
    init();
}

Network::~Network() {
}

void Network::init() {
    if (m_initialized) {
        return;
    }

    // NVS e necessario para o driver WiFi armazenar calibracoes/config.
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    // Registra o handler estatico para os eventos de WiFi (ex.: desconexao)
    // e de IP (ex.: IP obtido apos conectar), repassando "this" para que o
    // handler consiga acessar o estado da instancia.
    // >>> Esta era a peca que faltava: sem isso, eventHandler() nunca
    //     e chamado e a reconexao automatica nao acontece.
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &Network::eventHandler, this, nullptr));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &Network::eventHandler, this, nullptr));

    ESP_ERROR_CHECK(esp_wifi_start());

    m_initialized = true;
}

void Network::eventHandler(void* arg, esp_event_base_t eventBase,
                           int32_t eventId, void* eventData) {
    Network* self = static_cast<Network*>(arg);
    if (self == nullptr) {
        return;
    }

    if (eventBase == WIFI_EVENT && eventId == WIFI_EVENT_STA_DISCONNECTED) {
        if (self->tentativas < self->maxTentativas) {
            self->tentativas++;
            ESP_LOGW(TAG, "Desconectado da rede \"%s\". Tentando reconectar (%d/%d)...",
                     self->ssid.c_str(), self->tentativas, self->maxTentativas);
            esp_wifi_connect();
        } else {
            ESP_LOGE(TAG, "Falha ao reconectar a \"%s\" apos %d tentativas. Desistindo.",
                     self->ssid.c_str(), self->maxTentativas);
        }
    } else if (eventBase == IP_EVENT && eventId == IP_EVENT_STA_GOT_IP) {
        self->tentativas = 0;
        auto* event = static_cast<ip_event_got_ip_t*>(eventData);
        uint32_t ip = event->ip_info.ip.addr;
        ESP_LOGI(TAG, "Conectado a \"%s\"! IP obtido: %d.%d.%d.%d",
                 self->ssid.c_str(),
                 static_cast<int>(ip & 0xFF),
                 static_cast<int>((ip >> 8) & 0xFF),
                 static_cast<int>((ip >> 16) & 0xFF),
                 static_cast<int>((ip >> 24) & 0xFF));
    }
}

void Network::escanearRedes() {
    if (!m_initialized) {
        init();
    }

    ESP_LOGI(TAG, "Iniciando escaneamento de redes WiFi...");

    // Scan bloqueante (block = true): a funcao so retorna quando o
    // escaneamento terminar.
    wifi_scan_config_t scanConfig = {};
    scanConfig.show_hidden = true;

    esp_err_t err = esp_wifi_scan_start(&scanConfig, true);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao iniciar o scan: %s", esp_err_to_name(err));
        return;
    }

    uint16_t apCount = 0;
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&apCount));

    if (apCount == 0) {
        ESP_LOGW(TAG, "Nenhuma rede encontrada.");
        return;
    }

    if (apCount > MAX_AP_RECORDS) {
        apCount = MAX_AP_RECORDS;
    }

    wifi_ap_record_t apRecords[MAX_AP_RECORDS];
    memset(apRecords, 0, sizeof(apRecords));

    uint16_t apCountToFetch = apCount;
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&apCountToFetch, apRecords));

    ESP_LOGI(TAG, "Redes encontradas: %d", apCountToFetch);
    ESP_LOGI(TAG, "-------------------------------------------------------------");

    for (uint16_t i = 0; i < apCountToFetch; ++i) {
        const wifi_ap_record_t& ap = apRecords[i];

        ESP_LOGI(TAG,
                 "SSID: %-32s | RSSI: %4d dBm | Canal: %2d | Auth: %-15s | BSSID: "
                 "%02X:%02X:%02X:%02X:%02X:%02X",
                 reinterpret_cast<const char*>(ap.ssid),
                 ap.rssi,
                 ap.primary,
                 authModeToString(ap.authmode),
                 ap.bssid[0], ap.bssid[1], ap.bssid[2],
                 ap.bssid[3], ap.bssid[4], ap.bssid[5]);
    }

    ESP_LOGI(TAG, "-------------------------------------------------------------");
}

void Network::connect(const std::string& ssid, const std::string& senha, int maxTentativas) {
    if (!m_initialized) {
        init();
    }

    // IMPORTANTE: os parametros tem o mesmo nome dos membros da classe
    // (shadowing). Por isso usamos "this->" para deixar explicito que
    // estamos gravando nos membros, e nao apenas manipulando variaveis
    // locais que desaparecem ao fim da funcao.
    this->ssid = ssid;
    this->senha = senha;
    this->maxTentativas = maxTentativas;
    this->tentativas = 0;

    wifi_config_t wifiConfig = {};

    strncpy(reinterpret_cast<char*>(wifiConfig.sta.ssid),
            ssid.c_str(), sizeof(wifiConfig.sta.ssid) - 1);

    strncpy(reinterpret_cast<char*>(wifiConfig.sta.password),
            senha.c_str(), sizeof(wifiConfig.sta.password) - 1);

    wifiConfig.sta.threshold.authmode = senha.empty() ? WIFI_AUTH_OPEN : WIFI_AUTH_WPA2_PSK;

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifiConfig));

    ESP_LOGI(TAG, "Conectando a rede \"%s\" (max. %d tentativas de reconexao)...",
             ssid.c_str(), maxTentativas);

    esp_err_t err = esp_wifi_connect();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao iniciar conexao: %s", esp_err_to_name(err));
    }
}