#pragma once

#include <string>
#include "esp_event.h"

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

    /**
     * @brief Escaneia as redes WiFi disponiveis e imprime no terminal
     *        as informacoes de cada uma (SSID, RSSI, canal, tipo de
     *        autenticacao e BSSID).
     */
    void escanearRedes();

    /**
     * @brief Conecta-se a uma rede WiFi especifica. Caso a conexao
     *        caia apos ter sido estabelecida (ou caso a tentativa
     *        inicial falhe), a classe tenta reconectar
     *        automaticamente ate maxTentativas vezes.
     *
     * @param ssid          Nome da rede (SSID).
     * @param senha         Senha da rede. Pode ser vazia para redes abertas.
     * @param maxTentativas Numero maximo de tentativas de reconexao
     *                      (parametro opcional, padrao = 5).
     */
    void connect(const std::string& ssid, const std::string& senha, int maxTentativas = 5);

private:
    /**
     * @brief Garante que o stack WiFi/NVS/netif esteja inicializado
     *        antes de qualquer operacao de rede. E chamado
     *        internamente pelo construtor.
     */
    void init();

    /**
     * @brief Handler estatico de eventos WiFi/IP. Como o ESP-IDF
     *        exige uma funcao livre (ou estatica) como callback,
     *        repassamos o ponteiro "this" da instancia via arg
     *        para poder acessar o estado da classe.
     */
    static void eventHandler(void* arg, esp_event_base_t eventBase,
                              int32_t eventId, void* eventData);

    bool m_initialized;
    std::string ssid;
    std::string senha;
    int maxTentativas;
    int tentativas;
};