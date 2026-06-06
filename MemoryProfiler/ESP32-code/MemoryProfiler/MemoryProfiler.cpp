#include "MemoryProfiler.hpp"
#include <stdio.h>
#include <string.h>
#include "esp_timer.h" 
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/*
IRAM = SRAM0
DRAM = SRAM1 e SRAM2
*/

// Símbolos do linker (opcionais)
extern const int _data_start;
extern const int _data_end;
extern const int _bss_start;
extern const int _bss_end;
extern const int _iram_start;
extern const int _iram_end;

// ---- Variáveis estáticas para a task periódica ----
static TaskHandle_t s_profiler_task_handle = nullptr;
static char s_prefix[32] = "snap";
static uint32_t s_intervalo_ms = 5000;

// Task que executa o profiling
static void profilerTask(void* param) {
    uint32_t contador = 1;
    while (true) {
        char tag[64];
        snprintf(tag, sizeof(tag), "%s_%lu", s_prefix, contador++);
        MemoryProfiler profiler;
        profiler.enviarSnapshotEstatico(tag);
        vTaskDelay(pdMS_TO_TICKS(s_intervalo_ms));
    }
}

// ---- Implementação da classe ----

MemoryProfiler::MemoryProfiler() {
    // Nada a inicializar
}

void MemoryProfiler::enviarSnapshotEstatico(const char* identificador_teste) {
    // Tempo em ms desde o boot
    uint64_t timestamp_ms = esp_timer_get_time() / 1000ULL;

    size_t sram_total  = heap_caps_get_total_size(MALLOC_CAP_INTERNAL);
    size_t sram_livre  = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    size_t dram_total  = heap_caps_get_total_size(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
    size_t dram_livre  = heap_caps_get_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
    size_t iram_total  = heap_caps_get_total_size(MALLOC_CAP_EXEC | MALLOC_CAP_INTERNAL);
    size_t iram_livre  = heap_caps_get_free_size(MALLOC_CAP_EXEC | MALLOC_CAP_INTERNAL);
    size_t maior_bloco_livre = heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT);

    printf("\n[MP_JSON_START]");
    printf("{"
           "\"teste\":\"%s\","
           "\"timestamp_ms\":%llu,"
           "\"sram_total\":%u,"
           "\"sram_livre\":%u,"
           "\"dram_total\":%u,"
           "\"dram_livre\":%u,"
           "\"iram_total\":%u,"
           "\"iram_livre\":%u,"
           "\"maior_bloco_livre\":%u"
           "}", 
           identificador_teste,
           timestamp_ms,
           sram_total,
           sram_livre,
           dram_total,
           dram_livre,
           iram_total,
           iram_livre,
           maior_bloco_livre);
    printf("[MP_JSON_END]\n");
}

// ---- Controle da task periódica ----

bool MemoryProfiler::iniciarAnalise(const char* prefixo, uint32_t intervalo_ms) {
    if (s_profiler_task_handle != nullptr) {
        return false;  // já está rodando
    }

    if (prefixo != nullptr) {
        strncpy(s_prefix, prefixo, sizeof(s_prefix) - 1);
        s_prefix[sizeof(s_prefix) - 1] = '\0';
    }
    s_intervalo_ms = intervalo_ms;

    BaseType_t ret = xTaskCreate(
        profilerTask,
        "memProfTask",
        4096,                // stack suficiente
        nullptr,
        tskIDLE_PRIORITY,    // prioridade baixa para não atrapalhar
        &s_profiler_task_handle
    );

    return (ret == pdPASS);
}

void MemoryProfiler::pararAnalise() {
    if (s_profiler_task_handle != nullptr) {
        vTaskDelete(s_profiler_task_handle);
        s_profiler_task_handle = nullptr;
    }
}