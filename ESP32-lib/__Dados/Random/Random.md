# Dados -> Classe `Random`

Classe utilitária estática em C++ para geração simplificada e segura de números aleatórios e preenchimento de buffers com base no gerador de números aleatórios por hardware (Hardware RNG) do ESP32 (`esp_random.h`).

---

## 🚀 Recursos Principais

- **Hardware RNG Nativo:** Aproveita a entropia térmica e ruído de rádio do hardware do ESP32.
- **Ponto Flutuante em Faixa (`floatRange`):** Gera floats em um intervalo pré-determinado `[min, max]`.
- **Inteiros em Faixa (`intRange`):** Gera inteiros em um intervalo inclusivo `[min, max]`.
- **Preenchimento de Buffers (`bytes`):** Utiliza `esp_fill_random` para inicializar buffers criptográficos ou payloads de teste.
- **API Estática:** Não requer instanciação de objetos (métodos diretos no formato `Random::floatRange(...)`).

---

## 📦 Dependências

| Arquivo / Componente | Papel no Módulo |
|---|---|
| `esp_random.h` | Acesso ao periférico RNG de hardware do ESP32 |

---

## 📋 API da Classe `Random`

| Método | Retorno | Descrição |
|---|---|---|
| `Random::get()` | `uint32_t` | Retorna inteiro de 32 bits aleatório direto do hardware |
| `Random::floatRange(min, max)` | `float` | Retorna valor de ponto flutuante no intervalo `[min, max]` |
| `Random::intRange(min, max)` | `int32_t` | Retorna inteiro aleatório inclusivo no intervalo `[min, max]` |
| `Random::bytes(buf, len)` | `void` | Preenche o buffer apontado com bytes aleatórios |

---

## 🛠️ Exemplo de Uso

```cpp
#include "Random.hpp"

// Gerar número de ponto flutuante para simulação de sensores
float tensao = Random::floatRange(11.5f, 12.5f);
float corrente = Random::floatRange(0.5f, 2.0f);

// Gerar valor inteiro
int canal = Random::intRange(1, 13);
```
