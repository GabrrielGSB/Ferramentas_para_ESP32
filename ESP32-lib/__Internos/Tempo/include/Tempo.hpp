#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_rom_sys.h" 
#include "esp_timer.h"

/**
 * @brief Classe estática utilitária para controle de tempo e atrasos no ESP32.
 * Não pode ser instanciada. Utilize chamadas diretas como Tempo::delayMs(100).
 */
class Tempo {
private:
    Tempo() = delete; // Impede instanciação de objetos

public:
    /**
     * @brief Atraso em milissegundos não-bloqueante para a CPU (bloqueia apenas a FreeRTOS Task).
     * @param ms Tempo em milissegundos.
     */
    static inline void delayMs(uint32_t ms) {
        vTaskDelay(pdMS_TO_TICKS(ms));
    }

    /**
     * @brief Atraso em segundos.
     * @param s Tempo em segundos.
     */
    static inline void delayS(uint32_t s) {
        vTaskDelay(pdMS_TO_TICKS(s * 1000));
    }

    /**
     * @brief Atraso em horas.
     * @param h Tempo em horas.
     */
    static inline void delayH(uint32_t h) {
        vTaskDelay(pdMS_TO_TICKS(h * 3600000));
    }

    /**
     * @brief Atraso em microsegundos com busy-wait (ocupação direta da CPU).
     * @param us Tempo em microsegundos.
     */
    static inline void delayUs(uint32_t us) {
        esp_rom_delay_us(us);
    }

    /**
     * @brief Atraso periódico com precisão de taxa constante (vTaskDelayUntil).
     * @param ultimoTempoAcordado Ponteiro para o tick da última execução.
     * @param periodoMs Período desejado em milissegundos.
     */
    static inline void delayAte(TickType_t* ultimoTempoAcordado, uint32_t periodoMs) {
        vTaskDelayUntil(ultimoTempoAcordado, pdMS_TO_TICKS(periodoMs));
    }

    /**
     * @brief Retorna o tempo decorrido desde o boot em milissegundos.
     */
    static inline uint64_t millis() {
        return (esp_timer_get_time() / 1000ULL);
    }

    /**
     * @brief Retorna o tempo decorrido desde o boot em microsegundos.
     */
    static inline uint64_t micros() {
        return esp_timer_get_time();
    }
};

// Aliases globais para compatibilidade retroativa com códigos existentes
static inline void delay_ms(uint32_t ms) { Tempo::delayMs(ms); }
static inline void delay_s(uint32_t s)   { Tempo::delayS(s); }
static inline void delay_h(uint32_t h)   { Tempo::delayH(h); }
static inline void delay_us(uint32_t us) { Tempo::delayUs(us); }
static inline void delayAte(TickType_t* ultimoTempoAcordado, uint32_t periodoMs) { Tempo::delayAte(ultimoTempoAcordado, periodoMs); }
static inline uint64_t millis()          { return Tempo::millis(); }
static inline uint64_t micros()          { return Tempo::micros(); }
