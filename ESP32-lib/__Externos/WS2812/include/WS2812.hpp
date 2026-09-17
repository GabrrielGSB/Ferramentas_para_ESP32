#pragma once

#ifndef WS2812_HPP
#define WS2812_HPP

#include <stdint.h>
#include <vector>
#include <algorithm>
#include "driver/gpio.h"
#include "esp_err.h"
#include "driver/rmt_tx.h"
#include "driver/rmt_encoder.h"

/**
 * @brief Representação de uma cor RGB (8 bits por canal).
 */
struct CorRGB {
    uint8_t r;
    uint8_t g;
    uint8_t b;

    constexpr CorRGB() : r(0), g(0), b(0) {}
    constexpr CorRGB(uint8_t vermelho, uint8_t verde, uint8_t azul)
        : r(vermelho), g(verde), b(azul) {}

    // Cores estáticas pré-definidas
    static const CorRGB Preto;
    static const CorRGB Branco;
    static const CorRGB Vermelho;
    static const CorRGB Verde;
    static const CorRGB Azul;
    static const CorRGB Amarelo;
    static const CorRGB Ciano;
    static const CorRGB Magenta;
    static const CorRGB Laranja;
    static const CorRGB Roxo;

    /**
     * @brief Converte matiz/saturação/valor (HSV) para CorRGB.
     * @param h Matiz (0 - 360)
     * @param s Saturação (0.0 - 1.0)
     * @param v Valor / Brilho (0.0 - 1.0)
     */
    static CorRGB deHSV(uint16_t h, float s, float v);
};

/**
 * @brief Classe para controle de fitas e LEDs endereçáveis WS2812/WS2812B (Neopixel)
 *        utilizando o periférico RMT do ESP32 (ESP-IDF v5+).
 */
class WS2812 {
public:
    /**
     * @brief Construtor da classe WS2812.
     * @param pino Pino GPIO conectado à linha de dados (DIN) do WS2812.
     * @param numLeds Quantidade total de LEDs em cascata (padrão: 1).
     * @param brilhoInicial Brilho global inicial (0 a 255, padrão: 255).
     */
    explicit WS2812(gpio_num_t pino, uint16_t numLeds = 1, uint8_t brilhoInicial = 255);

    ~WS2812();

    // Impede cópia para proteger handles do RMT
    WS2812(const WS2812&) = delete;
    WS2812& operator=(const WS2812&) = delete;

    /**
     * @brief Inicializa o canal transmissor RMT e o encoder de temporização.
     * @return esp_err_t ESP_OK em caso de sucesso.
     */
    esp_err_t inicializar();

    /**
     * @brief Define a cor de um LED específico pelo índice.
     * @param index Índice do LED (0 até numLeds - 1).
     * @param r Intensidade do canal Vermelho (0-255).
     * @param g Intensidade do canal Verde (0-255).
     * @param b Intensidade do canal Azul (0-255).
     */
    void definirCor(uint16_t index, uint8_t r, uint8_t g, uint8_t b);

    /**
     * @brief Define a cor de um LED específico pelo índice usando CorRGB.
     * @param index Índice do LED (0 até numLeds - 1).
     * @param cor Objeto CorRGB.
     */
    void definirCor(uint16_t index, const CorRGB& cor);

    /**
     * @brief Define a mesma cor para todos os LEDs da fita.
     */
    void definirTodos(uint8_t r, uint8_t g, uint8_t b);
    void definirTodos(const CorRGB& cor);

    /**
     * @brief Apaga todos os LEDs (cor preta 0,0,0) no buffer interno.
     */
    void limpar();

    /**
     * @brief Transmite o buffer de pixels atual para os LEDs físicos via hardware RMT.
     * @param timeoutMs Tempo máximo de espera para transmissão em ms.
     * @return esp_err_t ESP_OK em caso de sucesso.
     */
    esp_err_t atualizar(uint32_t timeoutMs = 1000);

    /**
     * @brief Ajusta o nível de brilho global aplicado no envio dos dados.
     * @param brilho Valor de 0 (completamente apagado) a 255 (brilho máximo).
     */
    void setBrilho(uint8_t brilho);

    /**
     * @brief Obtém o brilho global configurado.
     */
    uint8_t getBrilho() const { return m_brilho; }

    /**
     * @brief Obtém a quantidade total de LEDs configurada.
     */
    uint16_t getNumLeds() const { return m_numLeds; }

    /**
     * @brief Obtém a cor atual de um pixel no buffer (sem fator de brilho aplicado).
     */
    CorRGB getCor(uint16_t index) const;

    /**
     * @brief Gera um gradiente de efeito arco-íris sobre todos os LEDs.
     * @param deslocamentoMatiz Deslocamento inicial da matiz (0-360) para animações contínuas.
     */
    void efeitoArcoIris(uint16_t deslocamentoMatiz = 0);

private:
    gpio_num_t m_pino;
    uint16_t   m_numLeds;
    uint8_t    m_brilho;
    bool       m_inicializado;

    // Buffers de cores
    std::vector<CorRGB>  m_pixels;
    std::vector<uint8_t> m_bufferEnvio; // Formato GRB escalado por brilho

    // Handles do subsistema RMT ESP-IDF
    rmt_channel_handle_t m_rmtCanal;
    rmt_encoder_handle_t m_rmtEncoder;

    esp_err_t criarEncoderLed();
    void prepararBufferEnvio();
};

#endif // WS2812_HPP
