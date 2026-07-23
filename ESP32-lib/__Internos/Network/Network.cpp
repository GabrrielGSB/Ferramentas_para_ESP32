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

Network::Network() : m_initialized(false) {
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
    ESP_ERROR_CHECK(esp_wifi_start());

    m_initialized = true;
}

void Network::scanNetworks() {
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