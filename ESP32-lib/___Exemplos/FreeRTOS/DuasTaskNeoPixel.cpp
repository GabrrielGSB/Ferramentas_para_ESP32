#include <cstdio>
#include "esp_system.h"
#include "WS2812.hpp"
#include "Task.hpp"
#include "Tempo.hpp"

static WS2812 led(GPIO_NUM_48);

// =========================================================================
// 1. TAREFA VIA FUNÇÃO NORMAL (Executada no Core 0)
// =========================================================================
// Faz o monitoramento periódico do sistema e imprime telemetria no console
void rotinaMonitoramento() {
    uint32_t contador = 0;
    while (true) {
        contador++;
        uint32_t heapLivre = esp_get_free_heap_size();
        uint64_t tempoMs   = millis();

        printf("[Monitor Core 0] Ciclo: %lu | Uptime: %llums | Heap Livre: %lu bytes\n", 
               (unsigned long)contador, 
               (unsigned long long)tempoMs, 
               (unsigned long)heapLivre);

        delay_ms(2000); // Aguarda 2 segundos de forma não-bloqueante
    }
}

// =========================================================================
// FLUXO PRINCIPAL (app_main)
// =========================================================================
extern "C" void app_main(void) {
    printf("=== Inicializando Sistema com __FreeRTOS e WS2812 ===\n");

    // Inicializa o hardware do LED
    led.inicializar();
    led.setBrilho(40); // 15% de brilho para conforto visual

    // Cria e inicia a Tarefa 1: Função normal no Core 0
    static Task taskMonitor(rotinaMonitoramento, "TaskMonitor", 3072, 1, TaskCore::Core0);
    taskMonitor.iniciar();

    // Cria e inicia a Tarefa 2: Expressão Lambda no Core 1
    // Responsável pela animação suave de arco-íris a ~40 FPS sem interferir no Core 0
    static Task taskLed([]() {
        uint16_t matiz = 0;
        while (true) {
            led.efeitoArcoIris(matiz);
            led.atualizar();

            matiz = (matiz + 3) % 360; // Avança a cor no círculo cromático
            delay_ms(25);              // ~40 atualizações por segundo
        }
    }, "TaskLED", 3072, 2, TaskCore::Core1);
    taskLed.iniciar();

    // Verificação dos estados das tarefas
    if (taskMonitor.estaExecutando()) {
        printf("✓ TaskMonitor rodando no Core 0.\n");
    }
    if (taskLed.estaExecutando()) {
        printf("✓ TaskLED rodando no Core 1.\n");
    }

    printf("Sistema multitarefa ativo concorrentemente em ambos os núcleos!\n");
}
