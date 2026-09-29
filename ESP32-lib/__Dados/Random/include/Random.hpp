#pragma once

#ifndef RANDOM_HPP
#define RANDOM_HPP

#include <cstdint>
#include "esp_random.h"

/**
 * @file Random.hpp
 * @brief Classe utilitária estática para geração de números pseudoaleatórios e aleatórios por hardware no ESP32.
 */
class Random {
public:
    // Impede instanciação de classe utilitária
    Random() = delete;

    /**
     * @brief Retorna um número inteiro de 32 bits gerado diretamente pelo gerador de hardware (RNG) do ESP32.
     * @return uint32_t Valor aleatório entre 0 e UINT32_MAX.
     */
    static inline uint32_t get() {
        return esp_random();
    }

    /**
     * @brief Retorna um número de ponto flutuante no intervalo [min, max].
     * 
     * @param min Valor mínimo do intervalo.
     * @param max Valor máximo do intervalo.
     * @return float Número aleatório entre min e max.
     */
    static float floatRange(float min, float max);

    /**
     * @brief Retorna um número inteiro dentro do intervalo [min, max] (inclusivo).
     * 
     * @param min Valor mínimo inclusivo.
     * @param max Valor máximo inclusivo.
     * @return int32_t Número inteiro aleatório.
     */
    static int32_t intRange(int32_t min, int32_t max);

    /**
     * @brief Preenche um buffer de bytes com valores aleatórios obtidos via hardware RNG.
     * 
     * @param buf Ponteiro para a área de memória.
     * @param len Tamanho em bytes a ser preenchido.
     */
    static void bytes(void* buf, size_t len);
};

#endif // RANDOM_HPP
