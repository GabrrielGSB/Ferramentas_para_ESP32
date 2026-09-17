# Internos -> Classe `GPIO`

Biblioteca orientada a objetos para controle e abstração de pinos de Entrada e Saída de Uso Geral (**GPIO**) no ESP32, utilizando as APIs nativas do ESP-IDF (`driver/gpio.h`).

---

## Dependências

| Arquivo / Biblioteca | Papel |
|----------------------|-------|
| `driver/gpio.h`      | Driver nativo de GPIO do ESP-IDF para configuração e leitura/escrita |

---

## Compatibilidade

A classe detecta automaticamente o target selecionado pelo ESP-IDF via macros de build e disponibiliza constantes legíveis para os pinos:

| Modelo | Pinos mapeados | Constantes disponíveis |
|---|---|---|
| **ESP32** | GPIO0 – GPIO39 | `GPIO0` a `GPIO36`, `GPIO39` |
| **ESP32-S3** | GPIO0 – GPIO48 | `GPIO0` a `GPIO21`, `GPIO26` a `GPIO36`, `GPIO38` a `GPIO48` |
| **ESP32-C3** | GPIO0 – GPIO21 | `GPIO0` a `GPIO21` |

> [!WARNING]
> Verifique sempre a pinagem e as restrições de hardware:
> - No **ESP32 clássico**, os pinos **GPIO34 a GPIO39** são **exclusivamente entradas** (GPI) e não possuem resistores de pull-up/pull-down internos por hardware.
> - Evite usar pinos de *strapping* ou dedicados à memória Flash/PSRAM integrada.

---

## Modos de operação e resistores

Para simplificar a configuração em relação aos tipos verbosos do ESP-IDF, são fornecidos aliases diretos:

### Direção do pino (`gpio_mode_t`)

| Macro | Valor ESP-IDF correspondente | Significado |
|---|---|---|
| `OUTPUT` | `GPIO_MODE_OUTPUT` | Saída digital push-pull |
| `INPUT` | `GPIO_MODE_INPUT` | Entrada digital de alta impedância |
| `INPUT_OUTPUT` | `GPIO_MODE_INPUT_OUTPUT` | Entrada e saída simultâneas (permite ler o estado de saída) |
| `OPEN_DRAIN` | `GPIO_MODE_OUTPUT_OD` | Saída em dreno aberto (open-drain) |

### Resistores internos de pull (`gpio_pull_mode_t`)

Os microcontroladores ESP32 contam com resistores internos comutáveis de elevação (*pull-up*) e descida (*pull-down*), úteis para garantir níveis lógicos definidos sem componentes externos:

| Macro | Valor ESP-IDF correspondente | Comportamento |
|---|---|---|
| `NOPULL` | `GPIO_FLOATING` | Flutuante (sem resistores internos ativados) — *Padrão* |
| `PULLUP` | `GPIO_PULLUP_ONLY` | Ativa o resistor interno de pull-up |
| `PULLDOWN` | `GPIO_PULLDOWN_ONLY` | Ativa o resistor interno de pull-down |

> [!TIP]
> Para botões ou chaves táteis conectadas ao GND, recomenda-se configurar com `PULLUP`. Caso deseje detecção de toque curto/longo ou debounce automático via software, considere utilizar a classe especializada `__Externos/Botao`.

---

## Interface da classe

```cpp
class GPIO {
protected:
    gpio_num_t  pino;           // Número do pino no hardware
    gpio_mode_t modo;           // Modo configurado (INPUT, OUTPUT, etc.)
    uint8_t     estado_atual;   // Cache do último estado lógico aplicado

public:
    // Construtor: inicializa, reseta e configura o pino automaticamente
    GPIO(gpio_num_t numPino, gpio_mode_t modoPino, gpio_pull_mode_t modoPull = NOPULL);

    // Controle de saídas digitais
    void ligar();                               // Define nível lógico alto (1)
    void desligar();                            // Define nível lógico baixo (0)
    void inverter();                            // Inverte/alterna o nível atual

    // Leitura digital
    int  ler();                                 // Retorna o nível lógico atual (0 ou 1)

    // Reconfiguração em tempo de execução
    void configPull(gpio_pull_mode_t modoPull); // Altera o modo de resistor pull-up/pull-down
};
```

---

## Como usar

### 1. Incluir o header

```cpp
#include "GPIO.hpp"
```

### 2. Criar uma instância

O construtor já efetua o reset do pino (`gpio_reset_pin`), define sua direção e aplica a configuração de pull-up/pull-down solicitada. Não é preciso chamar funções de setup separadas:

```cpp
// Pino 2 como saída digital push-pull
GPIO led(GPIO2, OUTPUT);

// Pino 4 como entrada com resistor pull-up interno ativo
GPIO botao(GPIO4, INPUT, PULLUP);

// Pino 5 com resistor pull-down interno
GPIO sensor(GPIO5, INPUT, PULLDOWN);
```

### 3. Controlar saídas

```cpp
led.ligar();     // Pino vai para nível alto (3.3V / 1)
led.desligar();  // Pino vai para nível baixo (0V / 0)
led.inverter();  // Alterna o estado: se estava em 1 vai para 0, e vice-versa
```

### 4. Ler entradas

```cpp
int nivel = botao.ler(); // Retorna 0 ou 1

if (nivel == 0) {
    // Botão pressionado (com PULLUP, GND = nível baixo = ativo)
}
```

### 5. Reconfigurar resistor de pull

A função `configPull` permite alterar dinamicamente o resistor de pull de um pino já instanciado:

```cpp
botao.configPull(NOPULL); // Remove o resistor interno
```

---

## Métodos em detalhes

### `ligar()` — Coloca o pino em nível alto

```cpp
void ligar();
```

Aplica nível lógico 1 (`1` / 3.3V) ao pino associado. Só executa se o pino tiver sido configurado com `OUTPUT` ou `INPUT_OUTPUT`. Em modos exclusivamente de entrada, a chamada é ignorada de forma segura.

---

### `desligar()` — Coloca o pino em nível baixo

```cpp
void desligar();
```

Aplica nível lógico 0 (`0` / 0V) ao pino associado. Só executa se o pino tiver sido configurado com `OUTPUT` ou `INPUT_OUTPUT`.

---

### `inverter()` — Alterna o estado lógico da saída

```cpp
void inverter();
```

Inverte o valor atual da saída digital baseado no controle interno de estado (se estava em `0`, vai para `1`; se estava em `1`, vai para `0`). Ideal para geradores de clock manuais e rotinas de pisca-pisca de sinalizadores (*blink*).

---

### `ler()` — Obtém o nível lógico do pino

```cpp
int ler();
```

Consulta diretamente os registradores de entrada do hardware (`gpio_get_level`) e retorna `1` ou `0`. Funciona para qualquer modo de pino (`INPUT`, `OUTPUT` e `INPUT_OUTPUT`).

---

### `configPull(modoPull)` — Ajusta os resistores internos

```cpp
void configPull(gpio_pull_mode_t modoPull);
```

Reconfigura os resistores de pull do pino em tempo de execução via `gpio_set_pull_mode`.
- Parâmetros aceitos: `PULLUP`, `PULLDOWN` ou `NOPULL`.
- Só tem efeito prático em pinos configurados como `INPUT` ou `INPUT_OUTPUT`.

---

## Resumo dos métodos

| Método | Retorno | Bloqueante | Modos suportados | Descrição |
|---|---|---|---|---|
| `ligar()` | `void` | Não | `OUTPUT`, `INPUT_OUTPUT` | Coloca o pino no estado alto (`1`) |
| `desligar()` | `void` | Não | `OUTPUT`, `INPUT_OUTPUT` | Coloca o pino no estado baixo (`0`) |
| `inverter()` | `void` | Não | `OUTPUT`, `INPUT_OUTPUT` | Alterna o estado lógico da saída |
| `ler()` | `int` | Não | Todos (`INPUT`, `OUTPUT`, etc.) | Retorna o nível lógico atual (`0` ou `1`) |
| `configPull(modoPull)` | `void` | Não | `INPUT`, `INPUT_OUTPUT` | Altera dinamicamente os resistores de pull |

---

## Exemplos completos

### 1. Piscar LED (Blink)

Exemplo clássico utilizando a biblioteca auxiliar de temporização não bloqueante [`Tempo.hpp`]:

```cpp
#include "GPIO.hpp"
#include "Tempo.hpp"

extern "C" void app_main() {
    GPIO led(GPIO2, OUTPUT);

    while (true) {
        led.inverter();
        delay_ms(500); // Aguarda 500 ms de forma cooperativa com o FreeRTOS
    }
}
```

### 2. Botão controlando LED (Entrada com Pull-Up)

Leitura contínua de um botão com debounce simples:

```cpp
#include "GPIO.hpp"
#include "Tempo.hpp"

extern "C" void app_main() {
    GPIO led(GPIO2, OUTPUT);
    GPIO botao(GPIO4, INPUT, PULLUP); // Botão com pull-up interno ligado ao GND

    while (true) {
        if (botao.ler() == 0) { // Botão pressionado (ativo em nível lógico baixo)
            led.ligar();
        } else {
            led.desligar();
        }
        delay_ms(10); // Debounce por amostragem
    }
}
```

> [!NOTE]
> Para funcionalidades avançadas com botões (tais como clique curto, clique longo, eventos assíncronos e debounce em hardware/software), utilize a classe `__Externos/Botao`.

### 3. LED piscando sem atraso bloqueante (`millis()`)

Exemplo demonstrando a integração com `millis()` para controle temporal sem interromper o fluxo do laço principal:

```cpp
#include "GPIO.hpp"
#include "Tempo.hpp"

extern "C" void app_main() {
    GPIO led(GPIO2, OUTPUT);
    uint64_t ultimo_pisca = millis();

    while (true) {
        if ((millis() - ultimo_pisca) >= 500) {
            ultimo_pisca = millis();
            led.inverter();
        }

        // Outras tarefas podem ser executadas aqui em paralelo sem atraso
    }
}
```
