#include "NetworkHandler.hpp" 
#include "Network.hpp"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_mac.h"

namespace {
    constexpr const char* TAG = "NetworkHandler";
    constexpr EventBits_t WIFI_CONNECTED_BIT = BIT0;
    constexpr EventBits_t WIFI_FAIL_BIT      = BIT1;
}

/**
 * @brief Callback estático do loop de eventos para tratar eventos de Wi-Fi e IP.
 * 
 * @param arg Ponteiro genérico repassado no registro (instância `this` de Network).
 * @param eventBase Base do evento (ex: WIFI_EVENT ou IP_EVENT).
 * @param eventId Identificador numérico do evento específico.
 * @param eventData Ponteiro com os dados associados ao evento disparado.
 */
void NetworkHandler::eventHandler(void* arg, esp_event_base_t eventBase, int32_t eventId, void* eventData) {
    auto* self = static_cast<Network*>(arg);
    if (self == nullptr) return;

    if (eventBase == WIFI_EVENT) {
        switch (eventId) {

            case WIFI_EVENT_STA_DISCONNECTED: {
                xEventGroupClearBits(self->m_wifiEventGroup, WIFI_CONNECTED_BIT);

                if (self->tentativas < self->maxTentativas) {
                    self->agendarReconexao();
                    self->tentativas++;
                } 
                else {
                    self->cancelarReconexao();
                    ESP_LOGE(TAG, "Falha ao reconectar a \"%s\". Limite de tentativas (%d) atingido.",
                             self->ssid.c_str(), self->maxTentativas);
                    xEventGroupSetBits(self->m_wifiEventGroup, WIFI_FAIL_BIT);
                }
                break;
            }

            case WIFI_EVENT_AP_STACONNECTED: {
                auto* event = static_cast<wifi_event_ap_staconnected_t*>(eventData);
                ESP_LOGI(TAG, "Cliente conectado ao AP - MAC: " MACSTR ", AID: %d",
                         MAC2STR(event->mac), event->aid);
                break;
            }

            case WIFI_EVENT_AP_STADISCONNECTED: {
                auto* event = static_cast<wifi_event_ap_stadisconnected_t*>(eventData);
                ESP_LOGI(TAG, "Cliente desconectado do AP - MAC: " MACSTR ", AID: %d",
                         MAC2STR(event->mac), event->aid);
                break;
            }

            case WIFI_EVENT_AP_START: {
                ESP_LOGI(TAG, "Access Point iniciado.");
                break;
            }

            case WIFI_EVENT_AP_STOP: {
                ESP_LOGI(TAG, "Access Point parado.");
                break;
            }

            default:
                break;
        }
    } else if (eventBase == IP_EVENT && eventId == IP_EVENT_STA_GOT_IP) {
        self->cancelarReconexao();
        self->tentativas = 0;
        auto* event = static_cast<ip_event_got_ip_t*>(eventData);
        ESP_LOGI(TAG, "Conectado! IP: " IPSTR, IP2STR(&event->ip_info.ip));

        xEventGroupSetBits(self->m_wifiEventGroup, WIFI_CONNECTED_BIT);
        xEventGroupClearBits(self->m_wifiEventGroup, WIFI_FAIL_BIT);
    }
}