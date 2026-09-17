# Componente FreeRTOS - Classe Task

Wrapper orientado a objetos em C++ moderno para gerenciamento de **Tarefas (Tasks)** do FreeRTOS no ESP32, operando via funções e expressões Lambda (`std::function<void()>`).

---

## 🚀 Recursos

- **RAII (Resource Acquisition Is Initialization):** Deleta a tarefa do FreeRTOS automaticamente via `vTaskDelete` caso o objeto seja destruído.
- **Não Copiável:** Construtor de cópia e operador de atribuição deletados (`= delete`) para evitar problemas de concorrência com o `TaskHandle_t`.
- **Baseado em Funções / Lambdas:** Basta fornecer um `std::function<void()>`.
- **Captura de Contexto:** Lambdas com captura (`[&]` ou `[this]`) facilitam o acesso a variáveis locais e membros de classes sem a necessidade de structs auxiliares.
- **Afinidade de Núcleo (Multi-Core):** Enum `TaskCore` explícito (`Core0`, `Core1`, `QualquerUm` mapeando para `tskNO_AFFINITY`).
- **Finalização Segura:** Padrão trampolim (`taskTrampoline`) com desalocação e limpeza de estado automáticas caso a rotina retorne sem loop infinito.
- **Integração com Tempo:** Inclui `Tempo.hpp` para manipulação de delays precisos (`delay_ms`, `delayAte`, `millis`).
- **Cedência Cooperativa:** Método estático `Task::ceder()` (`taskYIELD()`).

---

## 📋 API da Classe

### Enum `TaskCore`
```cpp
enum class TaskCore {
    Core0 = 0,
    Core1 = 1,
    QualquerUm = tskNO_AFFINITY
};
```

### Construtor e Destrutor
- `Task(TaskFunction func, const string& nome = "Task", uint32_t tamanhoPilha = 4096, UBaseType_t prioridade = 1, TaskCore core = TaskCore::QualquerUm)`
- `~Task()`: Chama `parar()`, removendo a tarefa do FreeRTOS se estiver ativa.

### Controle do Ciclo de Vida
- `bool iniciar()`: Instancia e agenda a tarefa via `xTaskCreatePinnedToCore`. Retorna `true` em caso de sucesso.
- `void parar()`: Exclui a tarefa (`vTaskDelete`) e reseta o handle e estado.
- `void suspender()`: Pausa a execução da tarefa (`vTaskSuspend`).
- `void retomar()`: Retoma uma tarefa suspensa (`vTaskResume`).
- `static inline void ceder()`: Cede o processamento voluntariamente (`taskYIELD()`).

### Getters e Setters
- `bool estaExecutando() const`: Indica se a tarefa está criada e em execução.
- `TaskHandle_t getHandle() const`: Retorna o handle nativo do FreeRTOS.
- `const string& getNome() const`: Retorna o nome atribuído à tarefa.
- `UBaseType_t getPrioridade() const`: Retorna a prioridade atual da tarefa.
- `TaskCore getCore() const`: Retorna o núcleo configurado.
- `void setPrioridade(UBaseType_t novaPrioridade)`: Atualiza a prioridade local e, caso em execução, no FreeRTOS (`vTaskPrioritySet`).

---

## 🛠️ Exemplos de Uso

### 1. Criando com Função Normal (Livre)

```cpp
#include "Task.hpp"
#include <cstdio>

// Função normal comum (void sem parâmetros)
void rotinaSensor() {
    while (true) {
        printf("Lendo sensores...\n");
        delay_ms(2000);
    }
}

// Cria a tarefa apontando para a função normal
Task tarefaSensor(rotinaSensor, "SensorTask", 2048, 1, TaskCore::Core1);

tarefaSensor.iniciar();
```

---

### 2. Criando com Lambda Simples

```cpp
#include "Task.hpp"
#include <cstdio>

// Cria a tarefa com função lambda
Task tarefaPisca([]() {
    while (true) {
        printf("Tick executando a cada 500ms!\n");
        delay_ms(500); // Função utilitária de Tempo.hpp
    }
}, "PiscaTask", 2048, 1, TaskCore::Core0);

// Inicia a execução no FreeRTOS
tarefaPisca.iniciar();
```

---

### 3. Captura de Contexto / Variáveis com Lambda

```cpp
#include "Task.hpp"
#include <cstdio>

int contador = 0;

Task tarefaContador([&contador]() {
    while (true) {
        contador++;
        printf("Contagem: %d\n", contador);
        delay_ms(1000);
    }
}, "ContadorTask", 2048, 1, TaskCore::QualquerUm);

tarefaContador.iniciar();
```

---

### 4. Usando Método de Classe (via std::bind)

Como `TaskFunction` é um `std::function<void()>`, você também pode vincular métodos normais de uma classe:

```cpp
#include "Task.hpp"
#include <cstdio>
#include <functional>

class ServicoTelemetria {
public:
    void loopTelemetria() {
        while (true) {
            printf("Enviando telemetria...\n");
            delay_ms(3000);
        }
    }
};

ServicoTelemetria servico;

// Vincula o método normal da classe à instância usando std::bind
Task tarefaTelemetria(
    std::bind(&ServicoTelemetria::loopTelemetria, &servico),
    "TelemetriaTask",
    3072,
    1,
    TaskCore::Core1
);

tarefaTelemetria.iniciar();
```

---

### 5. Controle de Ciclo de Vida e Prioridade

```cpp
// Pausa temporariamente
tarefaPisca.suspender();

// Verifica status
if (tarefaPisca.estaExecutando()) {
    // Altera prioridade em tempo de execução
    tarefaPisca.setPrioridade(2);
}

// Retoma a execução
tarefaPisca.retomar();

// Cede processamento de forma cooperativa
Task::ceder();

// Encerra e desaloca a tarefa manualmente
tarefaPisca.parar();
```
