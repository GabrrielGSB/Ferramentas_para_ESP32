#pragma once

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
    void scanNetworks();

private:
    /**
     * @brief Garante que o stack WiFi/NVS/netif esteja inicializado
     *        antes de qualquer operacao de rede. E chamado
     *        internamente pelo construtor.
     */
    void init();

    bool m_initialized;
};