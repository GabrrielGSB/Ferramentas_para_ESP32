# Internos -> Classe `Tempo`

Classe estática utilitária para temporização, delays e medição de intervalos no ESP32 com ESP-IDF. Fornece atrasos cooperativos e determinísticos baseados no FreeRTOS, delays de alta precisão em microssegundos via hardware e leitura contínua de contadores de tempo (`millis()` e `micros()`).

---

## 🚀 Recursos Principais

- **Classe 100% Estática (`<<utility>>`):** Sem necessidade de instanciar objetos nem consumo desnecessário de memória em pilha ou heap (`Tempo::delayMs(...)`).
- **Zero Overhead (`static inline`):** Todas as chamadas são resolvidas em tempo de compilação diretamente para as primitivas de baixo nível do ESP-IDF e FreeRTOS.
- **Temporização Segura com FreeRTOS:** Delays cooperativos (`delayMs`, `delayS`, `delayH`) utilizam `pdMS_TO_TICKS(...)`, evitando travamentos de CPU e cedendo tempo a outras tarefas (*Tasks*).
- **Sem Desvio de Tempo (*Timing Drift*):** Método `delayAte(...)` baseado em `vTaskDelayUntil` para garantir loops periódicos e taxas de amostragem perfeitamente constantes.
- **Alta Resolução:** `micros()` e `millis()` alimentados pelo hardware de timer do ESP-IDF (`esp_timer_get_time`).
- **Compatibilidade Retroativa:** Fornece aliases globais das funções antigas (`delay_ms`, `delay_s`, `delay_us`, etc.) para manter código existente totalmente funcional.

---

## 📦 Dependências

| Arquivo / Biblioteca | Papel no Módulo |
|---|---|
| `freertos/FreeRTOS.h` | Definições básicas e tipos do FreeRTOS (`TickType_t`, macros de tick) |
| `freertos/task.h` | Primitivas `vTaskDelay` e `vTaskDelayUntil` (delays cooperativos) |
| `esp_rom_sys.h` | `esp_rom_delay_us` — atraso de precisão em microssegundos via hardware ROM |
| `esp_timer.h` | `esp_timer_get_time` — contador de tempo de alta resolução desde o boot (µs) |

> Todas as funções são implementadas diretamente no header (`Tempo.hpp`) como `static inline`. Não é necessária compilação separada de arquivo `.cpp`.

---

## 📋 API da Classe `Tempo`

### Resumo dos Métodos

| Método Estático | Retorno | Descrição |
|---|---|---|
| `Tempo::delayMs(ms)` | `void` | Aguarda em milissegundos cedendo o processador ao FreeRTOS |
| `Tempo::delayS(s)` | `void` | Aguarda em segundos (conveniência para esperas longas) |
| `Tempo::delayH(h)` | `void` | Aguarda em horas |
| `Tempo::delayUs(us)` | `void` | Aguarda em microssegundos via *busy-wait* de alta precisão |
| `Tempo::delayAte(ultimoTempo, periodoMs)` | `void` | Delay periódico absoluto sem acúmulo de drift (`vTaskDelayUntil`) |
| `Tempo::millis()` | `uint64_t` | Retorna o tempo decorrido desde o boot em milissegundos |
| `Tempo::micros()` | `uint64_t` | Retorna o tempo decorrido desde o boot em microssegundos |

---

## 🔍 Detalhamento das Funções

### `delayMs(uint32_t ms)` — Aguarda em milissegundos

Pausa a task atual pelo tempo especificado, cedendo o processador a outras tarefas do FreeRTOS. É a forma recomendada para a maioria das pausas.

```cpp
Tempo::delayMs(500);  // aguarda 500 ms
Tempo::delayMs(1000); // aguarda 1 segundo
```

---

### `delayS(uint32_t s)` — Aguarda em segundos

Converte segundos em ticks do FreeRTOS e bloqueia cooperativamente a tarefa. Conveniência para esperas longas com código mais limpo.

```cpp
Tempo::delayS(5);  // aguarda 5 segundos
Tempo::delayS(60); // aguarda 1 minuto
```

---

### `delayH(uint32_t h)` — Aguarda em horas

Para esperas muito longas, como ciclos de suspensão ou monitoramento ambiental esporádico.

```cpp
Tempo::delayH(1); // aguarda 1 hora
Tempo::delayH(8); // aguarda 8 horas
```

> A macro `pdMS_TO_TICKS` recebe e calcula valores em `TickType_t` (geralmente `uint32_t`). Para intervalos extremamente longos de horas, atente-se para que a conversão não cause overflow de ticks. Caso precise de agendamento prolongado com economia de energia, considere modos de Deep Sleep ou divisões em loops periódicos.

---

### `delayUs(uint32_t us)` — Aguarda em microssegundos

Implementado com `esp_rom_delay_us` — executa uma espera ativa (**busy-wait**) sem ceder o processador ao FreeRTOS. Use exclusivamente para temporizações muito curtas em que a precisão de sinal/clock é essencial (ex: sensores One-Wire, barramentos manuais, pulsos de reset).

```cpp
Tempo::delayUs(10);  // aguarda 10 µs
Tempo::delayUs(500); // aguarda 500 µs
```

> Por ser busy-wait, esta função **mantém o núcleo da CPU ocupado a 100%** durante a espera e impede que outras tarefas de mesma prioridade executem naquele núcleo. Evite valores superiores a ~1 ms — para esperas maiores, utilize `Tempo::delayMs(...)`.

---

### `delayAte(TickType_t* ultimoTempoAcordado, uint32_t periodoMs)` — Delay Periódico Preciso

Baseado na primitiva `vTaskDelayUntil` do FreeRTOS. Executa um atraso absoluto em relação ao momento da última ativação da tarefa, compensando o tempo que a tarefa gastou executando processamentos. Isso evita desvio de tempo (*timing drift*) acumulado e garante taxas exatas de execução.

```cpp
TickType_t ultimoTempo = xTaskGetTickCount();

while (true) {
    Tempo::delayAte(&ultimoTempo, 100); // Garante execução periódica a exatamente 10 Hz
    lerSensores();
}
```

---

### `millis()` e `micros()` — Tempo decorrido desde o boot

Baseados no timer de hardware de 64 bits do ESP-IDF (`esp_timer_get_time`). São funções de alta resolução ideais para medir intervalos e executar tarefas sem bloquear loops.

```cpp
uint64_t ms = Tempo::millis(); // Ex: 3520 ms desde a inicialização
uint64_t us = Tempo::micros(); // Ex: 3520140 µs
```

---

## ⚖️ Comparativo entre os Tipos de Delay

| Método | Unidade | Mecanismo Base | Cede CPU ao FreeRTOS? | Uso Recomendado |
|---|---|---|:---:|---|
| `Tempo::delayMs` | Milissegundos (ms) | `vTaskDelay` | ✅ Sim | Esperas gerais e cooperativas em tarefas |
| `Tempo::delayS` | Segundos (s) | `vTaskDelay` | ✅ Sim | Pausas longas e legíveis |
| `Tempo::delayH` | Horas (h) | `vTaskDelay` | ✅ Sim | Ciclos lentos de telemetria/rotina |
| `Tempo::delayUs` | Microssegundos (µs) | `esp_rom_delay_us` (busy-wait) | ❌ Não | Pulsos de hardware, temporização em nível de protocolo |
| `Tempo::delayAte` | Milissegundos (ms) | `vTaskDelayUntil` | ✅ Sim | Amostragem de sensores com taxa fixa (sem drift) |

---

## 🛠️ Exemplos Práticos

### 1. Medição de Intervalos Não-Bloqueante com `millis()`

O padrão com `millis()` substitui `delayMs` quando a tarefa precisa continuar atendendo outros eventos no mesmo ciclo:

```cpp
#include "Tempo.hpp"
#include <cstdio>

extern "C" void app_main(void) {
    uint64_t ultimoTempo = Tempo::millis();

    while (true) {
        uint64_t agora = Tempo::millis();

        if ((agora - ultimoTempo) >= 1000) { // Dispara a cada 1000 ms (1 s)
            ultimoTempo = agora;
            printf("Evento periódico disparado a cada 1s!\n");
        }

        // O restante do loop continua executando sem bloqueio
        Tempo::delayMs(10); // Cede levemente a CPU para o FreeRTOS
    }
}
```

---

### 2. Piscando LED sem Atraso Bloqueante (com `GPIO`)

Exemplo combinando a classe `Tempo` com o módulo `GPIO`:

```cpp
#include "Tempo.hpp"
#include "GPIO.hpp"

extern "C" void app_main(void) {
    GPIO led(GPIO2, OUTPUT);
    uint64_t ultimoPisca = Tempo::millis();

    while (true) {
        if ((Tempo::millis() - ultimoPisca) >= 500) {
            ultimoPisca = Tempo::millis();
            led.inverter();
        }

        // Outras rotinas podem rodar aqui sem atraso perceptível
        Tempo::delayMs(1);
    }
}
```

---

### 3. Temporização de Alta Frequência / Protocolo com `delayUs()`

```cpp
#include "Tempo.hpp"
#include "GPIO.hpp"

void gerarPulsoTrigger(GPIO& pinoTrigger) {
    pinoTrigger.escrever(LOW);
    Tempo::delayUs(2);
    
    pinoTrigger.escrever(HIGH);
    Tempo::delayUs(10); // Pulso de 10 µs para iniciar sensor ultrassônico
    
    pinoTrigger.escrever(LOW);
}
```

---

## 🔄 Compatibilidade Retroativa

Para garantir que códigos ou exemplos mais antigos não quebrem, o cabeçalho ainda expõe aliases para as funções de escopo livre:

```cpp
delay_ms(500);                               // Equivalente a Tempo::delayMs(500)
delay_s(2);                                  // Equivalente a Tempo::delayS(2)
delay_us(10);                                // Equivalente a Tempo::delayUs(10)
delayAte(&ultimoTick, 100);                  // Equivalente a Tempo::delayAte(&ultimoTick, 100)
uint64_t t = millis();                       // Equivalente a Tempo::millis()
```
