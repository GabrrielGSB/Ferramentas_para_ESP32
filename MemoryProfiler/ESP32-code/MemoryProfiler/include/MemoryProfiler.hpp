#pragma once
#ifndef MEMORY_PROFILER_HPP
#define MEMORY_PROFILER_HPP

#include <stddef.h>
#include <stdint.h>

/**
 * @brief Classe Profiler de Memória para ESP32 (ESP-IDF)
 * Captura dados estáticos do mapa de compilação e consumos dinâmicos do Heap.
 * Inclui suporte a envio periódico via task dedicada.
 */
class MemoryProfiler {
public:
    MemoryProfiler();

    /**
     * @brief Coleta métricas e envia JSON estruturado pela Serial.
     * @param identificador_teste Tag para rotular este snapshot.
     */
    void enviarSnapshotEstatico(const char* identificador_teste);

    /**
     * @brief Inicia uma task do FreeRTOS que envia snapshots periodicamente.
     * @param prefixo Prefixo para o nome dos snapshots (ex: "meu_app").
     * @param intervalo_ms Intervalo entre envios, em milissegundos.
     * @return true se a task foi criada com sucesso, false se já estiver rodando.
     */
    static bool iniciarAnalise(const char* prefixo, uint32_t intervalo_ms);

    /**
     * @brief Para a task de profiling periódico, se estiver rodando.
     */
    static void pararAnalise();
};

#endif