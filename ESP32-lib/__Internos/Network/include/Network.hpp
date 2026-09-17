#pragma once

#include <string>
#include "esp_event.h"
#include "esp_log.h" 
#include "esp_timer.h"
#include "esp_netif.h"
#include "HTTP.hpp" 

/*
struct dados_wifi{
       float condutividade=0;
       float corrent=0;
       float tensao=0;
       float potencia=0;
       float ph=0;
       float salmora=0;
       int timestamp=0;
};
*/
enum class ConfigRede {
    STA,
    AP,
    AP_STA
};

/**
 * @file Network.hpp
 * @brief Classe simples para operacoes de rede no ESP32 usando ESP-IDF.
 */
class Network {
    friend class NetworkHandler;

public:
    HTTP http;
    
    explicit Network(ConfigRede modo = ConfigRede::STA);
    ~Network();

    esp_err_t estaInicializado() const { return m_ultimoErro; }

    esp_err_t escanear();
    esp_err_t conectar(const std::string& ssid, const std::string& senha, int maxTentativas = 5);
    esp_err_t iniciarAP(const std::string& ssid, const std::string& senha = "",
                   uint8_t canal = 1, uint8_t maxConexoes = 4);
    esp_err_t iniciarAPSTA(const std::string& apSsid, const std::string& apSenha,
                      const std::string& staSsid, const std::string& staSenha,
                      int maxTentativas = 5);

    esp_err_t aguardarConexao(uint32_t timeoutMs = 10000);
    bool estaConectado() const;

    ConfigRede modoAtual() const { return config_rede; }

    // Getters de Rede
    std::string obterIP() const;
    std::string obterMascara() const;
    std::string obterGateway() const;
    std::string obterMAC() const;
    int8_t obterRSSI() const;
    
    // Configuração de Identificação
    bool definirHostname(const std::string& nome);

    static void timerReconexaoCallback(void* arg);

private:
    esp_err_t init();

    void agendarReconexao();
    void cancelarReconexao();

    EventGroupHandle_t m_wifiEventGroup;
    std::string ssid;
    std::string senha;
    ConfigRede config_rede;
    bool m_initialized;
    esp_err_t m_ultimoErro;
    int maxTentativas;
    int tentativas;
    esp_netif_t* m_netifSta = nullptr;
    esp_netif_t* m_netifAp  = nullptr;

    esp_timer_handle_t m_timerReconexao;
    uint32_t m_backoffBaseMs = 1000;
    uint32_t m_backoffMaxMs = 60000;
};