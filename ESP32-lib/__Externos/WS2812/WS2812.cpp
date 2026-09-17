#include "WS2812.hpp"
#include <cmath>
#include <cstring>
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* TAG = "WS2812";

// Definições de constantes estáticas para cores comuns
const CorRGB CorRGB::Preto   = CorRGB(0, 0, 0);
const CorRGB CorRGB::Branco  = CorRGB(255, 255, 255);
const CorRGB CorRGB::Vermelho= CorRGB(255, 0, 0);
const CorRGB CorRGB::Verde   = CorRGB(0, 255, 0);
const CorRGB CorRGB::Azul    = CorRGB(0, 0, 255);
const CorRGB CorRGB::Amarelo = CorRGB(255, 255, 0);
const CorRGB CorRGB::Ciano   = CorRGB(0, 255, 255);
const CorRGB CorRGB::Magenta = CorRGB(255, 0, 255);
const CorRGB CorRGB::Laranja = CorRGB(255, 128, 0);
const CorRGB CorRGB::Roxo    = CorRGB(128, 0, 255);

CorRGB CorRGB::deHSV(uint16_t h, float s, float v) {
    h = h % 360;
    s = std::max(0.0f, std::min(1.0f, s));
    v = std::max(0.0f, std::min(1.0f, v));

    float c = v * s;
    float x = c * (1.0f - std::fabs(std::fmod(h / 60.0f, 2.0f) - 1.0f));
    float m = v - c;

    float rPrim = 0, gPrim = 0, bPrim = 0;

    if (h < 60) {
        rPrim = c; gPrim = x; bPrim = 0;
    } else if (h < 120) {
        rPrim = x; gPrim = c; bPrim = 0;
    } else if (h < 180) {
        rPrim = 0; gPrim = c; bPrim = x;
    } else if (h < 240) {
        rPrim = 0; gPrim = x; bPrim = c;
    } else if (h < 300) {
        rPrim = x; gPrim = 0; bPrim = c;
    } else {
        rPrim = c; gPrim = 0; bPrim = x;
    }

    return CorRGB(
        static_cast<uint8_t>((rPrim + m) * 255.0f),
        static_cast<uint8_t>((gPrim + m) * 255.0f),
        static_cast<uint8_t>((bPrim + m) * 255.0f)
    );
}

// =============================================================================
// IMPLEMENTAÇÃO DA CLASSE WS2812
// =============================================================================

WS2812::WS2812(gpio_num_t pino, uint16_t numLeds, uint8_t brilhoInicial)
    : m_pino(pino),
      m_numLeds(numLeds > 0 ? numLeds : 1),
      m_brilho(brilhoInicial),
      m_inicializado(false),
      m_pixels(m_numLeds, CorRGB::Preto),
      m_bufferEnvio(m_numLeds * 3, 0),
      m_rmtCanal(nullptr),
      m_rmtEncoder(nullptr) {}

WS2812::~WS2812() {
    if (m_inicializado) {
        limpar();
        atualizar(100);

        if (m_rmtEncoder != nullptr) {
            rmt_del_encoder(m_rmtEncoder);
            m_rmtEncoder = nullptr;
        }

        if (m_rmtCanal != nullptr) {
            rmt_disable(m_rmtCanal);
            rmt_del_channel(m_rmtCanal);
            m_rmtCanal = nullptr;
        }
        m_inicializado = false;
    }
}

esp_err_t WS2812::criarEncoderLed() {
    // Configuração do Bytes Encoder padrão do ESP-IDF (Clock 10MHz -> 1 tick = 100ns)
    // Bit 0: ~300ns HIGH (3 ticks), ~900ns LOW (9 ticks)
    // Bit 1: ~900ns HIGH (9 ticks), ~300ns LOW (3 ticks)
    rmt_bytes_encoder_config_t bytes_encoder_config = {};

    bytes_encoder_config.bit0.level0 = 1;
    bytes_encoder_config.bit0.duration0 = 3;
    bytes_encoder_config.bit0.level1 = 0;
    bytes_encoder_config.bit0.duration1 = 9;

    bytes_encoder_config.bit1.level0 = 1;
    bytes_encoder_config.bit1.duration0 = 9;
    bytes_encoder_config.bit1.level1 = 0;
    bytes_encoder_config.bit1.duration1 = 3;

    bytes_encoder_config.flags.msb_first = 1;

    return rmt_new_bytes_encoder(&bytes_encoder_config, &m_rmtEncoder);
}

esp_err_t WS2812::inicializar() {
    if (m_inicializado) {
        return ESP_OK;
    }

    // Configuração do canal TX RMT (Clock 10MHz para resolução de 100ns por tick)
    rmt_tx_channel_config_t tx_chan_config = {};
    tx_chan_config.clk_src = RMT_CLK_SRC_DEFAULT;
    tx_chan_config.gpio_num = m_pino;
    tx_chan_config.mem_block_symbols = 64;
    tx_chan_config.resolution_hz = 10 * 1000 * 1000; // 10MHz -> 1 tick = 100ns
    tx_chan_config.trans_queue_depth = 4;

    esp_err_t err = rmt_new_tx_channel(&tx_chan_config, &m_rmtCanal);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao criar canal RMT TX: %s", esp_err_to_name(err));
        return err;
    }

    err = criarEncoderLed();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao criar encoder WS2812: %s", esp_err_to_name(err));
        rmt_del_channel(m_rmtCanal);
        m_rmtCanal = nullptr;
        return err;
    }

    err = rmt_enable(m_rmtCanal);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao habilitar canal RMT: %s", esp_err_to_name(err));
        rmt_del_encoder(m_rmtEncoder);
        m_rmtEncoder = nullptr;
        rmt_del_channel(m_rmtCanal);
        m_rmtCanal = nullptr;
        return err;
    }

    m_inicializado = true;

    // Garante que os LEDs iniciem apagados fisicamente
    limpar();
    atualizar();

    ESP_LOGI(TAG, "WS2812 inicializado no pino GPIO %d com %u LEDs.", m_pino, m_numLeds);
    return ESP_OK;
}

void WS2812::definirCor(uint16_t index, uint8_t r, uint8_t g, uint8_t b) {
    if (index < m_numLeds) {
        m_pixels[index] = CorRGB(r, g, b);
    }
}

void WS2812::definirCor(uint16_t index, const CorRGB& cor) {
    if (index < m_numLeds) {
        m_pixels[index] = cor;
    }
}

void WS2812::definirTodos(uint8_t r, uint8_t g, uint8_t b) {
    definirTodos(CorRGB(r, g, b));
}

void WS2812::definirTodos(const CorRGB& cor) {
    for (uint16_t i = 0; i < m_numLeds; i++) {
        m_pixels[i] = cor;
    }
}

void WS2812::limpar() {
    definirTodos(CorRGB::Preto);
}

CorRGB WS2812::getCor(uint16_t index) const {
    if (index < m_numLeds) {
        return m_pixels[index];
    }
    return CorRGB::Preto;
}

void WS2812::setBrilho(uint8_t brilho) {
    m_brilho = brilho;
}

void WS2812::prepararBufferEnvio() {
    // Protocolo do WS2812 recebe na ordem: Verde (G), Vermelho (R), Azul (B)
    for (uint16_t i = 0; i < m_numLeds; i++) {
        const CorRGB& pixel = m_pixels[i];
        // Aplica escala do brilho (0 a 255)
        uint8_t g = (static_cast<uint16_t>(pixel.g) * m_brilho) / 255;
        uint8_t r = (static_cast<uint16_t>(pixel.r) * m_brilho) / 255;
        uint8_t b = (static_cast<uint16_t>(pixel.b) * m_brilho) / 255;

        m_bufferEnvio[i * 3 + 0] = g;
        m_bufferEnvio[i * 3 + 1] = r;
        m_bufferEnvio[i * 3 + 2] = b;
    }
}

esp_err_t WS2812::atualizar(uint32_t timeoutMs) {
    if (!m_inicializado || m_rmtCanal == nullptr || m_rmtEncoder == nullptr) {
        return ESP_ERR_INVALID_STATE;
    }

    prepararBufferEnvio();

    rmt_transmit_config_t tx_config = {};
    tx_config.loop_count = 0;      // Transmissão única
    tx_config.flags.eot_level = 0; // Nível baixo ao término da transmissão para reset

    esp_err_t err = rmt_transmit(m_rmtCanal, m_rmtEncoder, m_bufferEnvio.data(), m_bufferEnvio.size(), &tx_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Erro ao transmitir dados via RMT: %s", esp_err_to_name(err));
        return err;
    }

    // Aguarda o término da transmissão dos pulsos físicos
    err = rmt_tx_wait_all_done(m_rmtCanal, pdMS_TO_TICKS(timeoutMs));
    if (err != ESP_OK) {
        return err;
    }

    // Tempo de reset para travar os dados nos LEDs (mínimo 50us em nível baixo)
    esp_rom_delay_us(60);
    return ESP_OK;
}

void WS2812::efeitoArcoIris(uint16_t deslocamentoMatiz) {
    if (m_numLeds == 0) return;
    
    for (uint16_t i = 0; i < m_numLeds; i++) {
        uint16_t matiz = (deslocamentoMatiz + (i * 360 / m_numLeds)) % 360;
        m_pixels[i] = CorRGB::deHSV(matiz, 1.0f, 1.0f);
    }
}
