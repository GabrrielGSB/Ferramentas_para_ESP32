#pragma once

#include <string>
#include "esp_event.h"
#include "esp_http_client.h"
#include "esp_log.h" 

/**
 * @file Network.hpp
 * @brief Classe simples para operacoes de rede no ESP32 usando ESP-IDF.
 *
 * Nesta primeira versao, a classe implementa apenas o escaneamento
 * de redes WiFi disponiveis, imprimindo no terminal (log) as
 * informacoes mais importantes de cada rede encontrada.
 */

class Network {
public:
    Network();
    ~Network();


    void escanear();
    void conectar(const std::string& ssid, const std::string& senha, int maxTentativas = 5);

    bool aguardarConexao(uint32_t timeoutMs = 10000);
    bool estaConectado() const;

    std::string httpGet(const std::string& url);

private:
    void init();

    static void _eventHandler(void* arg, esp_event_base_t eventBase,
                              int32_t eventId, void* eventData);

    static esp_err_t _httpEventHandler(esp_http_client_event_t *evt);

    bool m_initialized;
    EventGroupHandle_t m_wifiEventGroup;
    std::string ssid;
    std::string senha;
    int maxTentativas;
    int tentativas;
};