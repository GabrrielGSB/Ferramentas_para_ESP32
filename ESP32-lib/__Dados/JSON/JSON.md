# Documentação: Componente JSON (`__Dados/JSON`)

A classe `JSON` oferece uma camada de abstração em C++ moderna, intuitiva e segura sobre o `cJSON` nativo do ESP-IDF. Ela elimina a necessidade de gerenciamento manual de memória (`free`, `cJSON_Delete`), evitando vazamentos de memória (*memory leaks*) e simplificando tanto a criação de payloads quanto a leitura de respostas.

---

## Recursos Principais

- **Segurança de Memória (RAII):** Desaloca automaticamente os nós criados ao sair de escopo.
- **Sintaxe Fluente:** Encadeamento de chamadas (`json.set(...).set(...)`).
- **Sub-objetos e Arrays:** Criação facilitada de árvores aninhadas e listas.
- **Leitura Segura:** Métodos `getInt()`, `getString()`, etc., com valores *fallback* caso o campo não exista ou o tipo seja incompatível.
- **Serialização Direta:** `toString()` para payloads compactos (ex: MQTT/HTTP) ou formatados (debug serial).

---

## Exemplo 1: Criando um Payload JSON (Telemetria)

```cpp
#include "JSON.hpp"
#include <cstdio>

void enviarTelemetria() {
    JSON doc;
    doc.set("dispositivo", "ESP32-S3")
       .set("temperatura", 26.4)
       .set("ativo", true)
       .set("leituras", 150);

    // Adicionando um sub-objeto
    JSON rede;
    rede.set("ssid", "MinhaRedeWiFi")
        .set("rssi", -62);

    doc.set("rede", std::move(rede));

    // Serializa em string compacta para envio
    std::string payload = doc.toString();
    printf("Payload: %s\n", payload.c_str());
}
```

---

## Exemplo 2: Parseando e Lendo um JSON Recebido

```cpp
#include "JSON.hpp"
#include <cstdio>

void processarMensagem(const std::string& payloadRecebido) {
    JSON doc = JSON::parse(payloadRecebido);

    if (!doc.isValid()) {
        printf("Erro ao parsear JSON!\n");
        return;
    }

    // Leitura com valor padrão caso a chave não exista
    std::string comando = doc.getString("comando", "desconhecido");
    int delayMs         = doc.getInt("delay_ms", 1000);
    bool habilitado     = doc.getBool("habilitado", false);

    printf("Comando: %s | Delay: %d | Habilitado: %s\n", 
           comando.c_str(), delayMs, habilitado ? "SIM" : "NAO");

    // Lendo um sub-objeto
    if (doc.has("configuracoes")) {
        JSON config = doc.getObject("configuracoes");
        double sensibilidade = config.getDouble("sensibilidade", 1.0);
        printf("Sensibilidade: %.2f\n", sensibilidade);
    }
}
```

---

## Exemplo 3: Trabalhando com Arrays / Listas

```cpp
#include "JSON.hpp"
#include <cstdio>

void criarArrayJson() {
    JSON lista = JSON::createArray();
    lista.add("Item 1")
         .add(100)
         .add(25.5);

    printf("Array: %s\n", lista.toString().c_str());

    // Iterando sobre o array
    for (size_t i = 0; i < lista.size(); i++) {
        JSON item = lista.getAt(i);
        // Processa item...
    }
}
```

---

## Resumo dos Métodos

| Método | Descrição |
| :--- | :--- |
| `JSON()` | Construtor padrão (cria objeto `{}`) |
| `JSON::parse(str)` | Faz parse de uma string JSON para um objeto |
| `JSON::createArray()` | Cria um array JSON `[]` |
| `.set(chave, valor)` | Define/atualiza um campo (int, int64_t, double, bool, string, sub-objeto) |
| `.add(valor)` | Adiciona um item ao final do array |
| `.has(chave)` | Verifica se a chave existe |
| `.getInt(chave, def)` | Lê valor inteiro ou retorna `def` se inexistente |
| `.getDouble(chave, def)` | Lê valor decimal ou retorna `def` se inexistente |
| `.getBool(chave, def)` | Lê booleano ou retorna `def` se inexistente |
| `.getString(chave, def)` | Lê string ou retorna `def` se inexistente |
| `.getObject(chave)` | Retorna visualização de um sub-objeto/array |
| `.toString(formatado)` | Serializa para string (`false` para compacto, `true` para identado) |
| `.isValid()` | Retorna `true` se o JSON for válido |
| `.clear()` | Limpa e reseta a estrutura |
