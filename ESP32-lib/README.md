# Estrutura e Organização da Biblioteca `ESP32-lib`

Este documento descreve a arquitetura de pastas, convenções de nomenclatura e organização dos módulos da biblioteca **ESP32-lib**, projetada para desenvolvimento modular e orientado a objetos com o **ESP-IDF**.

---

## 📌 Visão Geral da Arquitetura

A estrutura foi desenhada para separar claramente o que são **periféricos internos do microcontrolador**, **dispositivos externos**, **sensores**, **displays**, **gerenciamento de dados/RTOS** e **exemplos práticos**.

```
ESP32-lib/
├── __Internos/       # Periféricos e módulos integrados ao silício do ESP32
├── __Externos/       # Componentes externos discretos de uso geral (atuadores, botões, etc.)
├── _Sensores/        # Drivers para sensores de grandezas físicas
├── _Displays/        # Drivers e controladores para telas e mostradores
├── _Cameras/         # Módulos de captura e transmissão de vídeo/imagem
├── __FreeRTOS/       # Wrappers e abstrações para tarefas e sincronização RTOS
├── __Tasks/          # Tarefas prontas e orquestradores de alto nível
├── __Dados/          # Manipulação, parsing e serialização de dados (JSON, etc.)
└── ___Exemplos/      # Aplicações de teste e demonstração de uso dos módulos
```

---

## 📂 Descrição dos Diretórios

### 1. `__Internos/` — Periféricos Nativos do ESP32
Reúne bibliotecas que encapsulam registradores e drivers de baixo nível fornecidos pelo ESP-IDF, fornecendo interfaces limpas em C++:
- **`GPIO/`**: Abstração de pinos digitais de entrada e saída, modos de pull-up/pull-down e controle de nível lógico.
- **`PWM/`**: Geração de sinais PWM com base no periférico de hardware **LEDC** (controle de motores, iluminação, fade suave e clock para câmeras).
- **`ADC/`**: Leitura e calibração de conversores analógico-digitais internos.
- **`I2C/`**: Gerenciador do barramento I2C Master com suporte a endereçamento automático de dispositivos em 8 e 16 bits.
- **`Tempo/`**: Utilitários de temporização (`delay_ms`, `delay_us`, `delay_s`, `delay_ate`) e contagem com base em timers de alta resolução (`millis()`).
- **`Network/`**: Pilha de conectividade (Wi-Fi, HTTP client, WebSockets e Firebase).

---

### 2. `__Externos/` — Componentes de Interface Externa
Componentes de entrada e saída que se conectam ao ESP32 via GPIOs ou protocolos simples:
- **`Botao/`**: Abstração avançada para botões tácteis com suporte a debounce automático, detecção de clique simples, clique duplo e clique longo.
- **`WS2812/`**: Controle de LEDs endereçáveis (NeoPixel) via RMT ou barramento de pulsos.

---

### 3. `_Sensores/` — Sensoriamento de Grandezas Físicas
Contém bibliotecas dedicadas à comunicação com transdutores e sensores específicos:
- **`VL53L0X/`**: Sensor de distância a laser por tempo de voo (*Time-of-Flight* - ToF) operando sobre o barramento I2C.

---

### 4. `_Displays/` — Telas e Mostradores
Drivers gráficos e controladores de display:
- **`SSD1306/`**: Controlador para displays OLED monocromáticos (128x64 / 128x32) com suporte a primitivas gráficas e texto via I2C.
- **`_Exemplos/`**: Demonstrações visuais para telas.

---

### 5. `_Cameras/` — Módulos de Visão e Câmera
Integração com sensores de imagem via interface DVP/SCCB:
- **`Camera_OV5640/`**: Configuração, inicialização do sensor OV5640 e captura de quadros (framebuffers).

---

### 6. `__FreeRTOS/` — Abstração de Sistema Operacional
Camadas em C++ sobre a API nativa do FreeRTOS:
- Abstração orientada a objetos para criação e controle de tarefas (`Task`), filas (*Queues*), semáforos e timers de software.

---

### 7. `__Tasks/` — Tarefas de Fundo e Serviços Contínuos
Implementação de loops de execução e serviços assíncronos prontos para o FreeRTOS:
- **`Network/`**: Gerenciadores de reconexão de rede em background e sincronização periódica de dados na nuvem.

---

### 8. `__Dados/` — Formatação e Armazenamento
Módulos responsáveis por serialização, decodificação e processamento de fluxos de dados:
- **`JSON/`**: Wrapper para manipulação ergonômica de documentos JSON (criação, parsing e leitura de campos).

---

### 9. `___Exemplos/` — Projetos e Demonstrações de Uso
Programas de teste independentes divididos por módulo (`GPIO/`, `PWM/`, `Botao/`, `Network/`, `FreeRTOS/`, `Dados/`), contendo funções `app_main()` prontas para validação rápida em bancada.

---

## 🏷️ Padrão Interno dos Módulos

Cada pasta de módulo segue uma estrutura uniforme:

```
NomeDoModulo/
├── include/
│   └── NomeDoModulo.hpp   # Declaração da classe, macros e documentação Doxygen
├── NomeDoModulo.cpp       # Implementação dos métodos
└── NomeDoModulo.md        # Documentação completa em Markdown, exemplos e pinagens
```

### Regras de Documentação no Código
- **Headers (`.hpp`)**: Contêm tags Doxygen (`@brief`, `@param`, `@return`, `@note`) em todos os métodos públicos.
- **Documentos (`.md`)**: Contêm a tabela de dependências, tabela de compatibilidade com modelos de ESP32 (quando aplicável), descrição detalhada de cada método e exemplos de código completos.
