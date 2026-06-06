#include <stdio.h>
#include <stdlib.h>            // malloc, free
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "MemoryProfiler.hpp"

// ===== PARÂMETROS DA ALOCAÇÃO (ajuste aqui) =====
#define ALLOC_CHUNK_SIZE    (10 * 1024)   // 10 KB por alocação
#define MAX_ALLOC_COUNT     20            // número máximo de blocos alocados
#define ALLOC_DELAY_MS      5000          // intervalo entre alocações
// =================================================

static void *allocated_blocks[MAX_ALLOC_COUNT] = { nullptr };
static int alloc_counter = 0;

// Task que aloca memória periodicamente até atingir o limite
void memory_alloc_task(void *param) {
    while (alloc_counter < MAX_ALLOC_COUNT) {
        void *ptr = malloc(ALLOC_CHUNK_SIZE);
        if (ptr == nullptr) {
            printf("[ALOCADOR] Falha ao alocar %d bytes (bloco %d). Parando.\n",
                   ALLOC_CHUNK_SIZE, alloc_counter + 1);
            break;
        }
        allocated_blocks[alloc_counter++] = ptr;
        printf("[ALOCADOR] Bloco %d alocado: %d bytes. (total alocado: %d KB)\n",
               alloc_counter, ALLOC_CHUNK_SIZE, (alloc_counter * ALLOC_CHUNK_SIZE) / 1024);
        vTaskDelay(pdMS_TO_TICKS(ALLOC_DELAY_MS));
    }
    printf("[ALOCADOR] Limite de %d alocações atingido. Task finalizada.\n", MAX_ALLOC_COUNT);
    vTaskDelete(nullptr);
}

extern "C" void app_main(void) {
    MemoryProfiler profiler;

    // Inicia a task de alocação (executa em paralelo)
    xTaskCreate(
        memory_alloc_task,   // função
        "alloc_task",        // nome
        4096,                // stack size (suficiente para nossa lógica simples)
        nullptr,
        1,                   // prioridade (baixa, não atrapalha a main)
        nullptr
    );

    uint32_t contador_ciclo = 1;

    while (true) {
        char tag_teste[20];
        snprintf(tag_teste, sizeof(tag_teste), "%lu", contador_ciclo++);
        profiler.enviarSnapshotEstatico(tag_teste);

        // Opcional: imprime quantos blocos já foram alocados
        printf("[MAIN] Snapshots enviados: %lu | Blocos alocados até agora: %d\n",
               contador_ciclo - 1, alloc_counter);

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}