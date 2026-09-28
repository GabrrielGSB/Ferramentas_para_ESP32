# Dados -> classe JSON 

A classe `JSON` oferece uma camada de abstração em C++ sobre o `cJSON` nativo do ESP-IDF. Ela elimina a necessidade de gerenciamento manual de memória (`free`, `cJSON_Delete`), evitando vazamentos de memória (*memory leaks*) e simplificando tanto a criação de payloads quanto a leitura de respostas.

---

## Dependências

| Componente / Biblioteca | Gerenciador / Origem | Descrição |
| :--- | :--- | :--- |
| `cJSON` (`espressif/cjson`) | ESP Component Registry / ESP-IDF | Biblioteca em C nativa para parsing e manipulação de JSON (`cJSON.h`) |

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

    if (!doc.isValida()) {
        printf("Erro ao parsear JSON!\n");
        return;
    }

    // Valida se o conteúdo recebido é de fato um objeto JSON {}
    if (!doc.isObjeto()) {
        printf("Esperado um objeto JSON, mas recebido outro tipo!\n");
        return;
    }

    // Leitura com valor padrão caso a chave não exista
    std::string comando = doc.getString("comando", "desconhecido");
    int delayMs         = doc.getInt("delay_ms", 1000);
    int64_t timestamp   = doc.getInt64("timestamp", 0);
    bool habilitado     = doc.getBool("habilitado", false);

    printf("Comando: %s | Delay: %d | Timestamp: %lld | Habilitado: %s\n", 
           comando.c_str(), delayMs, static_cast<long long>(timestamp), habilitado ? "SIM" : "NAO");

    // Lendo um sub-objeto
    if (doc.temChave("configuracoes")) {
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
    JSON lista = JSON::criarLista();

    // Verificando o tipo da estrutura
    if (lista.isLista()) {
        lista.add("Item 1")
             .add(100)
             .add(25.5);
    }

    // Serialização com formatação legível (pretty-print) para debug serial
    printf("Array Formatado:\n%s\n", lista.toString(true).c_str());

    // Iterando sobre o array
    for (size_t i = 0; i < lista.tamanho(); i++) {
        JSON item = lista.getItem(i);
        // Processa item...
    }
}
```

---

## Exemplo 4: Reutilização, Interoperabilidade com ESP-IDF (`raw`/`release`)

```cpp
#include "JSON.hpp"
#include "cJSON.h"
#include <cstdio>

void integracaoComEspIdf() {
    JSON doc;
    doc.set("status", "ok")
       .set("codigo", 200);

    // 1. Acesso ao ponteiro bruto cJSON* para APIs nativas (sem transferir posse)
    cJSON* rawNode = doc.raw();
    if (rawNode) {
        printf("Tipo cJSON: %d\n", rawNode->type);
    }

    // 2. Limpar e reutilizar o mesmo objeto para uma nova montagem
    doc.clear();
    doc.set("novo_status", "resetado");

    // 3. Desvincular a posse de memória (útil ao repassar ponteiro para filas/APIs que deletarão o nó)
    cJSON* noDesvinculado = doc.release();

    // Como doc.release() foi chamado, doc não é mais dona do ponteiro.
    // O gerenciamento agora é manual via cJSON nativo:
    cJSON_Delete(noDesvinculado);
}
```

---

## Resumo dos Métodos

| Método | Descrição |
| :--- | :--- |
| `JSON()` | Construtor padrão (cria objeto `{}`) |
| `JSON(rawNode, owner)` | Construtor que envolve ponteiro `cJSON*` existente (controle de *ownership*) |
| `JSON::parse(str)` | Faz parse de uma string JSON para um objeto |
| `JSON::criarLista()` | Cria um array JSON `[]` |
| `.set(chave, valor)` | Define/atualiza um campo (`int`, `int64_t`, `double`, `bool`, `const char*`, `std::string`, sub-objeto) |
| `.add(valor)` | Adiciona um item ao final do array (`int`, `int64_t`, `double`, `bool`, `const char*`, `std::string`, sub-objeto) |
| `.temChave(chave)` | Verifica se a chave existe |
| `.getInt(chave, def)` | Lê valor inteiro ou retorna `def` se inexistente |
| `.getInt64(chave, def)` | Lê valor inteiro de 64 bits ou retorna `def` se inexistente |
| `.getDouble(chave, def)` | Lê valor decimal ou retorna `def` se inexistente |
| `.getBool(chave, def)` | Lê booleano ou retorna `def` se inexistente |
| `.getString(chave, def)` | Lê string ou retorna `def` se inexistente |
| `.getObject(chave)` | Retorna visualização de um sub-objeto/array |
| `.tamanho()` | Retorna o tamanho do array/objeto |
| `.getItem(index)` | Obtém um item do array pelo índice numérico |
| `.toString(formatado)` | Serializa para string (`false` para compacto, `true` para identado) |
| `.isValida()` | Retorna `true` se a estrutura for válida |
| `.isLista()` | Retorna `true` se o nó for um array/lista |
| `.isObjeto()` | Retorna `true` se o nó for um objeto |
| `.clear()` | Limpa a estrutura e reinicializa um objeto vazio `{}` |
| `.raw()` | Retorna o ponteiro bruto `cJSON*` subjacente |
| `.release()` | Desvincula a posse do nó `cJSON*`, impedindo sua destruição no destrutor |
