#include "Network.hpp"

#include <cstring>

#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "esp_crt_bundle.h"

namespace {
    constexpr const char* TAG = "Network";
    constexpr uint16_t MAX_AP_RECORDS = 20;
    constexpr EventBits_t WIFI_CONNECTED_BIT = BIT0;
    constexpr EventBits_t WIFI_FAIL_BIT      = BIT1;

    /**
     * @brief Converte o modo de autenticação do Wi-Fi para uma string legível.
     * 
     * @param authMode Enumeração do tipo wifi_auth_mode_t contendo o modo de autenticação do AP.
     * @return const char* Ponteiro para a string literal correspondente ao modo de segurança.
     */
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

/**
 * @brief Construtor padrão da classe Network.
 * 
 * Inicializa os atributos internos com valores padrão e executa
 * a inicialização da infraestrutura de rede (NVS, Netif e Wi-Fi).
 */
Network::Network() 
    : m_initialized(false), 
      m_wifiEventGroup(nullptr),
      ssid(), 
      senha(), 
      maxTentativas(0), 
      tentativas(0) {
    m_wifiEventGroup = xEventGroupCreate();
    init();
}

/**
 * @brief Destrutor da classe Network.
 */
Network::~Network() {
    if (m_wifiEventGroup != nullptr) {
        vEventGroupDelete(m_wifiEventGroup);
    }
}

/**
 * @brief Inicializa a pilha de rede e o hardware Wi-Fi no modo Station (STA).
 * 
 * Configura e inicializa a partição NVS, a camada Netif, o loop de eventos padrão,
 * o driver Wi-Fi com configurações padrão e registra os manipuladores de eventos.
 */
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

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &Network::_eventHandler, this, nullptr));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &Network::_eventHandler, this, nullptr));

    ESP_ERROR_CHECK(esp_wifi_start());

    m_initialized = true;
}

/**
 * @brief Callback estático do loop de eventos para tratar eventos de Wi-Fi e IP.
 * 
 * @param arg Ponteiro genérico repassado no registro (instância `this` de Network).
 * @param eventBase Base do evento (ex: WIFI_EVENT ou IP_EVENT).
 * @param eventId Identificador numérico do evento específico.
 * @param eventData Ponteiro com os dados associados ao evento disparado.
 */
void Network::_eventHandler(void* arg, esp_event_base_t eventBase, int32_t eventId, void* eventData) {
    auto* self = static_cast<Network*>(arg);
    if (self == nullptr) return;

    if (eventBase == WIFI_EVENT && eventId == WIFI_EVENT_STA_DISCONNECTED) {
        xEventGroupClearBits(self->m_wifiEventGroup, WIFI_CONNECTED_BIT);
        if (self->tentativas < self->maxTentativas) {
            self->tentativas++;
            ESP_LOGW(TAG, "Desconectado. Tentando reconectar (%d/%d)...", self->tentativas, self->maxTentativas);
            esp_wifi_connect();
        } else {
            ESP_LOGE(TAG, "Falha ao reconectar a \"%s\".", self->ssid.c_str());
            xEventGroupSetBits(self->m_wifiEventGroup, WIFI_FAIL_BIT);
        }
    } 
    else if (eventBase == IP_EVENT && eventId == IP_EVENT_STA_GOT_IP) {
        self->tentativas = 0;
        auto* event = static_cast<ip_event_got_ip_t*>(eventData);
        ESP_LOGI(TAG, "Conectado! IP: " IPSTR, IP2STR(&event->ip_info.ip));
        
        // Notifica que o IP foi obtido com sucesso
        xEventGroupSetBits(self->m_wifiEventGroup, WIFI_CONNECTED_BIT);
        xEventGroupClearBits(self->m_wifiEventGroup, WIFI_FAIL_BIT);
    }
}

/**
 * @brief Callback estático do cliente HTTP para tratar eventos de resposta.
 * 
 * @param evt Estrutura contendo informações sobre o evento HTTP ocorrido.
 * @return esp_err_t Código de status da execução do callback (ESP_OK para sucesso).
 */
esp_err_t Network::_httpEventHandler(esp_http_client_event_t *evt) {
    switch (evt->event_id) {
        case HTTP_EVENT_ON_DATA: {
            auto* responseBuffer = static_cast<std::string*>(evt->user_data);
            if (responseBuffer != nullptr && evt->data != nullptr && evt->data_len > 0) {
                responseBuffer->append(static_cast<char*>(evt->data), evt->data_len);
            }
            break;
        }
        case HTTP_EVENT_ERROR:
            ESP_LOGE("Network", "Erro durante a execução HTTP");
            break;
        default:
            break;
    }
    return ESP_OK;
}

/**
 * @brief Executa uma varredura bloqueante das redes Wi-Fi disponíveis e imprime o resultado.
 * 
 * Caso o subsistema não tenha sido inicializado, a função invoca init() automaticamente.
 * Os APs encontrados (até MAX_AP_RECORDS) são listados no terminal com SSID, RSSI, canal, autenticação e BSSID.
 */
void Network::escanear() {
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

/**
 * @brief Configura as credenciais e inicia a conexão a um ponto de acesso Wi-Fi.
 * 
 * @param ssid Nome da rede Wi-Fi (SSID).
 * @param senha Senha da rede Wi-Fi (deixe vazia para redes abertas).
 * @param maxTentativas Número máximo de tentativas consecutivas de reconexão em caso de queda.
 */
void Network::conectar(const std::string& ssid, const std::string& senha, int maxTentativas) {
    if (!m_initialized) {
        init();
    }

    this->ssid          = ssid;
    this->senha         = senha;
    this->maxTentativas = maxTentativas;
    this->tentativas    = 0;

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

/**
 * @brief Realiza uma requisição HTTP GET para a URL especificada e retorna o corpo da resposta.
 * 
 * @param url URL completa do recurso a ser acessado (ex.: "http://example.com/api/data").
 * @return std::string Corpo da resposta HTTP como string. Retorna string vazia em caso de erro.
 */
std::string Network::httpGet(const std::string& url) {
    std::string responseBody;

    esp_http_client_config_t config = {};
    config.url = url.c_str();
    config.event_handler = _httpEventHandler;
    config.user_data = &responseBody;
    config.timeout_ms = 8000; // Requisições TLS demoram mais pelo handshake
    
    // Habilita a validação de certificados SSL/TLS padrão da Mozilla embutidos no ESP-IDF
    config.crt_bundle_attach = esp_crt_bundle_attach;

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == nullptr) {
        ESP_LOGE("Network", "Falha ao inicializar esp_http_client");
        return "";
    }

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        int statusCode = esp_http_client_get_status_code(client);
        ESP_LOGI("Network", "HTTP GET finalizado com status: %d", statusCode);
        ESP_LOGI("Network", "Resposta: %s", responseBody.c_str());
    } else {
        ESP_LOGE("Network", "Falha na requisição HTTP: %s", esp_err_to_name(err));
        responseBody.clear();
    }

    esp_http_client_cleanup(client);
    return responseBody;
}

bool Network::aguardarConexao(uint32_t timeoutMs) {
    if (m_wifiEventGroup == nullptr) return false;

    EventBits_t bits = xEventGroupWaitBits(
        m_wifiEventGroup,
        WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
        pdFALSE,
        pdFALSE,
        pdMS_TO_TICKS(timeoutMs)
    );

    return (bits & WIFI_CONNECTED_BIT) != 0;
}

bool Network::estaConectado() const {
    if (m_wifiEventGroup == nullptr) return false;
    return (xEventGroupGetBits(m_wifiEventGroup) & WIFI_CONNECTED_BIT) != 0;
}









