#include "Network.hpp"

#include <cstring>

#include "esp_log.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "esp_crt_bundle.h"
#include "NetworkHandler.hpp"

namespace {
    constexpr const char* TAG = "Network";
    constexpr uint16_t    MAX_AP_RECORDS = 20;
    constexpr EventBits_t WIFI_CONNECTED_BIT = BIT0;
    constexpr EventBits_t WIFI_FAIL_BIT      = BIT1;

    /**
     * @brief Converte o modo de autenticação do Wi-Fi para uma string legível.
     * 
     * @param authMode Enumeração do tipo wifi_auth_mode_t contendo o modo de autenticação do AP.
     * @return const char* Ponteiro para a string literal correspondente ao modo de segurança.
     */
    const char* authModeParaString(wifi_auth_mode_t authMode) {
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

     /**
     * @brief Converte o ModoRede interno para o wifi_mode_t do ESP-IDF.
     */
    wifi_mode_t configRedeParaWifiMode(ConfigRede config) {
        switch (config) {
            case ConfigRede::STA:    return WIFI_MODE_STA;
            case ConfigRede::AP:     return WIFI_MODE_AP;
            case ConfigRede::AP_STA: return WIFI_MODE_APSTA;
            default:               return WIFI_MODE_STA;
        }
    }
}

/**
 * @brief Construtor da classe Network.
 * 
 * @param config_rede Modo de operação desejado (STA, AP ou AP_STA). Padrão: STA.
 */
Network::Network(ConfigRede config_rede) 
    : m_wifiEventGroup(nullptr),
      ssid(), 
      senha(),
      config_rede(config_rede), 
      m_initialized(false), 
      m_ultimoErro(ESP_OK),
      maxTentativas(0), 
      tentativas(0),
      m_netifSta(nullptr),
      m_netifAp(nullptr),
      m_timerReconexao(nullptr) {
      
    m_wifiEventGroup = xEventGroupCreate();
    if (m_wifiEventGroup == nullptr) {
        m_ultimoErro = ESP_ERR_NO_MEM;
        ESP_LOGE(TAG, "Falha ao alocar EventGroup");
        return;
    }

    m_ultimoErro = init();
}

/**
 * @brief Destrutor da classe Network.
 */
Network::~Network() {
    if (m_wifiEventGroup != nullptr) {
        vEventGroupDelete(m_wifiEventGroup);
    }

    if (m_timerReconexao != nullptr) {
        esp_timer_stop(m_timerReconexao);
        esp_timer_delete(m_timerReconexao);
        m_timerReconexao = nullptr;
    }
}

/**
 * @brief Inicializa a pilha de rede e o hardware Wi-Fi no modo configurado (STA, AP ou AP_STA).
 * 
 * Configura e inicializa a partição NVS, a camada Netif, o loop de eventos padrão,
 * o driver Wi-Fi com configurações padrão, cria a(s) netif(s) necessária(s) de acordo
 * com o modo escolhido, e registra os manipuladores de eventos.
 */
esp_err_t Network::init() {
    if (m_initialized) {
        return ESP_OK;
    }

    // NVS e necessario para o driver WiFi armazenar calibracoes/config.
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ret = nvs_flash_erase();
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Falha ao apagar particao NVS: %s", esp_err_to_name(ret));
            m_initialized = false;
            m_ultimoErro = ret;
            return ret;
        }
        ret = nvs_flash_init();
    }
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao inicializar NVS: %s", esp_err_to_name(ret));
        m_initialized = false;
        m_ultimoErro = ret;
        return ret;
    }

    ret = esp_netif_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao inicializar esp_netif: %s", esp_err_to_name(ret));
        m_initialized = false;
        m_ultimoErro = ret;
        return ret;
    }

    ret = esp_event_loop_create_default();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "Falha ao criar event loop padrao: %s", esp_err_to_name(ret));
        m_initialized = false;
        m_ultimoErro = ret;
        return ret;
    }

    // Cria as netifs de acordo com o modo escolhido.
    if (config_rede == ConfigRede::STA || 
        config_rede == ConfigRede::AP_STA) {
        m_netifSta = esp_netif_create_default_wifi_sta();
    }
    if (config_rede == ConfigRede::AP || 
        config_rede == ConfigRede::AP_STA) {
        m_netifAp = esp_netif_create_default_wifi_ap();
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ret = esp_wifi_init(&cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha em esp_wifi_init: %s", esp_err_to_name(ret));
        m_initialized = false;
        m_ultimoErro = ret;
        return ret;
    }

    ret = esp_wifi_set_mode(configRedeParaWifiMode(config_rede));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha em esp_wifi_set_mode: %s", esp_err_to_name(ret));
        m_initialized = false;
        m_ultimoErro = ret;
        return ret;
    }

    ret = esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &NetworkHandler::eventHandler, this, nullptr);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao registrar handler de WIFI_EVENT: %s", esp_err_to_name(ret));
        m_initialized = false;
        m_ultimoErro = ret;
        return ret;
    }

    ret = esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &NetworkHandler::eventHandler, this, nullptr);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao registrar handler de IP_EVENT: %s", esp_err_to_name(ret));
        m_initialized = false;
        m_ultimoErro = ret;
        return ret;
    }

    ret = esp_wifi_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha em esp_wifi_start: %s", esp_err_to_name(ret));
        m_initialized = false;
        m_ultimoErro = ret;
        return ret;
    }

    ESP_LOGI(TAG, "Wi-Fi inicializado no modo: %s",
             config_rede == ConfigRede::STA ? "STA" :
             config_rede == ConfigRede::AP  ? "AP"  : "AP_STA");

    esp_timer_create_args_t timerArgs = {
        .callback = &Network::timerReconexaoCallback,
        .arg = this,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "wifi_reconnect_tmr",
        .skip_unhandled_events = true
    };
    esp_timer_create(&timerArgs, &m_timerReconexao);

    m_initialized = true;
    m_ultimoErro = ESP_OK;
    return ESP_OK;
}

/**
 * @brief Executa uma varredura bloqueante das redes Wi-Fi disponíveis e imprime o resultado.
 * 
 * Caso o subsistema não tenha sido inicializado, a função invoca init() automaticamente.
 * Os APs encontrados (até MAX_AP_RECORDS) são listados no terminal com SSID, RSSI, canal, autenticação e BSSID.
 */
esp_err_t Network::escanear() {
    if (!m_initialized) {
        esp_err_t ret = init();
        if (ret != ESP_OK) return ret;
    }

    ESP_LOGI(TAG, "Iniciando escaneamento de redes WiFi...");

    wifi_scan_config_t scanConfig = {};
    scanConfig.show_hidden = true;

    esp_err_t err = esp_wifi_scan_start(&scanConfig, true);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao iniciar o scan: %s", esp_err_to_name(err));
        return err;
    }

    uint16_t apCount = 0;
    esp_err_t ret = esp_wifi_scan_get_ap_num(&apCount);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Erro ao obter total de APs: %s", esp_err_to_name(ret));
        return ret;
    }

    if (apCount == 0) {
        ESP_LOGW(TAG, "Nenhuma rede encontrada.");
        return ESP_OK;
    }

    if (apCount > MAX_AP_RECORDS) {
        apCount = MAX_AP_RECORDS;
    }

    wifi_ap_record_t apRecords[MAX_AP_RECORDS];
    memset(apRecords, 0, sizeof(apRecords));

    uint16_t apCountToFetch = apCount;
    ret = esp_wifi_scan_get_ap_records(&apCountToFetch, apRecords);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Erro ao obter registros de APs: %s", esp_err_to_name(ret));
        return ret;
    }

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
                 authModeParaString(ap.authmode),
                 ap.bssid[0], ap.bssid[1], ap.bssid[2],
                 ap.bssid[3], ap.bssid[4], ap.bssid[5]);
    }

    ESP_LOGI(TAG, "-------------------------------------------------------------");
    return ESP_OK;
}

/**
 * @brief Configura as credenciais e inicia a conexão a um ponto de acesso Wi-Fi.
 * 
 * @param ssid Nome da rede Wi-Fi (SSID).
 * @param senha Senha da rede Wi-Fi (deixe vazia para redes abertas).
 * @param maxTentativas Número máximo de tentativas consecutivas de reconexão em caso de queda.
 */
esp_err_t Network::conectar(const std::string& ssid, const std::string& senha, int maxTentativas) {
    if (!m_initialized) {
        esp_err_t ret = init();
        if (ret != ESP_OK) return ret;
    }

    cancelarReconexao();
    this->ssid          = ssid;
    this->senha         = senha;
    this->maxTentativas = maxTentativas;
    this->tentativas    = 0;
    xEventGroupClearBits(m_wifiEventGroup, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT);

    wifi_config_t wifiConfig = {};

    strncpy(reinterpret_cast<char*>(wifiConfig.sta.ssid),
            ssid.c_str(), sizeof(wifiConfig.sta.ssid) - 1);

    strncpy(reinterpret_cast<char*>(wifiConfig.sta.password),
            senha.c_str(), sizeof(wifiConfig.sta.password) - 1);

    wifiConfig.sta.threshold.authmode = senha.empty() ? WIFI_AUTH_OPEN : WIFI_AUTH_WPA2_PSK;

    esp_err_t err = esp_wifi_set_config(WIFI_IF_STA, &wifiConfig);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao configurar WiFi: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "Conectando a rede \"%s\" (max. %d tentativas de reconexao)...",
             ssid.c_str(), maxTentativas);

    err = esp_wifi_connect();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao iniciar conexao: %s", esp_err_to_name(err));
        return err;
    }

    return ESP_OK;
}

/**
 * @brief Configura e inicia o modo Access Point (AP) do Wi-Fi.
 * 
 * @param ssid Nome da rede a ser divulgada pelo AP.
 * @param senha Senha do AP. Deixe vazia para uma rede aberta (sem senha).
 *              Se não vazia, deve ter no mínimo 8 caracteres (exigência WPA2).
 * @param canal Canal Wi-Fi (1 a 13, dependendo da regulação local).
 * @param maxConexoes Número máximo de estações (clientes) conectadas simultaneamente.
 */
esp_err_t Network::iniciarAP(const std::string& ssid, const std::string& senha,
                         uint8_t canal, uint8_t maxConexoes) {
    if (config_rede != ConfigRede::AP && config_rede != ConfigRede::AP_STA) {
        ESP_LOGE(TAG, "iniciarAP() chamado, mas o modo atual nao suporta AP. "
                      "Crie o Network com ConfigRede::AP ou ConfigRede::AP_STA.");
        return ESP_ERR_INVALID_STATE;
    }

    if (!m_initialized) {
        esp_err_t ret = init();
        if (ret != ESP_OK) return ret;
    }

    if (!senha.empty() && senha.size() < 8) {
        ESP_LOGE(TAG, "Senha do AP deve ter no minimo 8 caracteres (ou vazia para rede aberta).");
        return ESP_ERR_INVALID_ARG;
    }

    wifi_config_t wifiConfig = {};

    strncpy(reinterpret_cast<char*>(wifiConfig.ap.ssid),
            ssid.c_str(), sizeof(wifiConfig.ap.ssid) - 1);
    wifiConfig.ap.ssid_len = static_cast<uint8_t>(ssid.size());

    strncpy(reinterpret_cast<char*>(wifiConfig.ap.password),
            senha.c_str(), sizeof(wifiConfig.ap.password) - 1);

    wifiConfig.ap.channel        = canal;
    wifiConfig.ap.max_connection = maxConexoes;
    wifiConfig.ap.authmode       = senha.empty() ? WIFI_AUTH_OPEN : WIFI_AUTH_WPA2_PSK;
    wifiConfig.ap.pmf_cfg.required = false;

    esp_err_t err = esp_wifi_set_config(WIFI_IF_AP, &wifiConfig);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao configurar AP: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "AP iniciado: SSID=\"%s\" | Canal=%d | Auth=%s | MaxConexoes=%d",
             ssid.c_str(), canal, authModeParaString(wifiConfig.ap.authmode), maxConexoes);
    
    return ESP_OK;
}

/**
 * @brief Inicia simultaneamente o Access Point (AP) e a conexão como estação (STA).
 * 
 * Método de conveniência para o modo ModoRede::AP_STA: configura o AP local
 * e, em seguida, inicia a tentativa de conexão a uma rede externa como cliente.
 * 
 * @param apSsid SSID do Access Point a ser criado.
 * @param apSenha Senha do Access Point (vazia para rede aberta).
 * @param staSsid SSID da rede externa à qual se conectar como estação.
 * @param staSenha Senha da rede externa.
 * @param maxTentativas Número máximo de tentativas de reconexão do STA em caso de queda.
 */
esp_err_t Network::iniciarAPSTA(const std::string& apSsid, const std::string& apSenha,
                           const std::string& staSsid, const std::string& staSenha,
                           int maxTentativas) {
    if (config_rede != ConfigRede::AP_STA) {
        ESP_LOGE(TAG, "iniciarAPSTA() chamado, mas o modo atual nao e AP_STA. "
                      "Crie o Network com ConfigRede::AP_STA para usar este metodo.");
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t ret = iniciarAP(apSsid, apSenha);
    if (ret != ESP_OK) {
        return ret;
    }
    return conectar(staSsid, staSenha, maxTentativas);
}

esp_err_t Network::aguardarConexao(uint32_t timeoutMs) {
    if (m_wifiEventGroup == nullptr) return ESP_ERR_INVALID_STATE;

    EventBits_t bits = xEventGroupWaitBits(
        m_wifiEventGroup,
        WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
        pdFALSE,
        pdFALSE,
        pdMS_TO_TICKS(timeoutMs)
    );

    if (bits & WIFI_CONNECTED_BIT) {
        return ESP_OK;
    } else if (bits & WIFI_FAIL_BIT) {
        return ESP_FAIL;
    }

    return ESP_ERR_TIMEOUT;
}

bool Network::estaConectado() const {
    if (m_wifiEventGroup == nullptr) return false;
    return (xEventGroupGetBits(m_wifiEventGroup) & WIFI_CONNECTED_BIT) != 0;
}

/**
 * @brief Obtém o endereço IPv4 atual formatado como string.
 */
std::string Network::obterIP() const {
    esp_netif_t* netif = (m_netifSta != nullptr) ? m_netifSta : m_netifAp;
    if (netif == nullptr) return "0.0.0.0";

    esp_netif_ip_info_t ipInfo;
    if (esp_netif_get_ip_info(netif, &ipInfo) != ESP_OK) {
        return "0.0.0.0";
    }

    char buf[16] = {0};
    snprintf(buf, sizeof(buf), IPSTR, IP2STR(&ipInfo.ip));
    return std::string(buf);
}

/**
 * @brief Obtém a máscara de sub-rede atual formatada como string.
 */
std::string Network::obterMascara() const {
    esp_netif_t* netif = (m_netifSta != nullptr) ? m_netifSta : m_netifAp;
    if (netif == nullptr) return "0.0.0.0";

    esp_netif_ip_info_t ipInfo;
    if (esp_netif_get_ip_info(netif, &ipInfo) != ESP_OK) {
        return "0.0.0.0";
    }

    char buf[16] = {0};
    snprintf(buf, sizeof(buf), IPSTR, IP2STR(&ipInfo.netmask));
    return std::string(buf);
}

/**
 * @brief Obtém o endereço do Gateway padrão formatado como string.
 */
std::string Network::obterGateway() const {
    esp_netif_t* netif = (m_netifSta != nullptr) ? m_netifSta : m_netifAp;
    if (netif == nullptr) return "0.0.0.0";

    esp_netif_ip_info_t ipInfo;
    if (esp_netif_get_ip_info(netif, &ipInfo) != ESP_OK) {
        return "0.0.0.0";
    }

    char buf[16] = {0};
    snprintf(buf, sizeof(buf), IPSTR, IP2STR(&ipInfo.gw));
    return std::string(buf);
}

/**
 * @brief Obtém o endereço físico MAC da interface ativa formatado como string (XX:XX:XX:XX:XX:XX).
 */
std::string Network::obterMAC() const {
    uint8_t mac[6] = {0};
    wifi_interface_t ifx = (config_rede == ConfigRede::AP) ? WIFI_IF_AP : WIFI_IF_STA;

    if (esp_wifi_get_mac(ifx, mac) != ESP_OK) {
        return "00:00:00:00:00:00";
    }

    char buf[18] = {0};
    snprintf(buf, sizeof(buf), MACSTR, MAC2STR(mac));
    return std::string(buf);
}

/**
 * @brief Obtém a potência do sinal Wi-Fi (RSSI) em dBm quando conectado como STA.
 * 
 * @return Potência em dBm, ou 0 se desconectado / não aplicável.
 */
int8_t Network::obterRSSI() const {
    if (!estaConectado()) {
        return 0;
    }

    wifi_ap_record_t apInfo;
    if (esp_wifi_sta_get_ap_info(&apInfo) != ESP_OK) {
        return 0;
    }

    return apInfo.rssi;
}

/**
 * @brief Define o hostname do dispositivo na rede.
 * 
 * @param nome Novo hostname a ser atribuído à(s) interface(s) de rede.
 * @return true se configurado com sucesso em ao menos uma interface, false em caso de falha.
 */
bool Network::definirHostname(const std::string& nome) {
    if (nome.empty()) return false;

    bool sucesso = false;

    if (m_netifSta != nullptr) {
        esp_err_t ret = esp_netif_set_hostname(m_netifSta, nome.c_str());
        if (ret == ESP_OK) {
            sucesso = true;
        } else {
            ESP_LOGW(TAG, "Falha ao definir hostname na interface STA: %s", esp_err_to_name(ret));
        }
    }

    if (m_netifAp != nullptr) {
        esp_err_t ret = esp_netif_set_hostname(m_netifAp, nome.c_str());
        if (ret == ESP_OK) {
            sucesso = true;
        } else {
            ESP_LOGW(TAG, "Falha ao definir hostname na interface AP: %s", esp_err_to_name(ret));
        }
    }

    return sucesso;
}

void Network::timerReconexaoCallback(void* arg) {
    auto* self = static_cast<Network*>(arg);
    if (self) {
        ESP_LOGI(TAG, "Disparando tentativa de reconexao Wi-Fi...");
        esp_wifi_connect();
    }
}

void Network::cancelarReconexao() {
    if (m_timerReconexao && esp_timer_is_active(m_timerReconexao)) {
        esp_timer_stop(m_timerReconexao);
    }
}

void Network::agendarReconexao() {
    cancelarReconexao();

    // Calcula tempo com backoff exponencial: base * 2^tentativas
    uint32_t shift = (tentativas > 30) ? 30 : tentativas;
    uint64_t delayMs = static_cast<uint64_t>(m_backoffBaseMs) * (1ULL << shift);

    if (delayMs > m_backoffMaxMs) {
        delayMs = m_backoffMaxMs;
    }

    ESP_LOGW(TAG, "Reconexao agendada em %llu ms (tentativa %d/%d)...",
             delayMs, tentativas + 1, maxTentativas);

    // esp_timer recebe intervalo em microsegundos
    esp_timer_start_once(m_timerReconexao, delayMs * 1000ULL);
}