# Internos -> Classe `Network`

Classe em C++ para gerenciamento simplificado e robusto de conectividade de rede no ESP32, abstraindo os drivers Wi-Fi (`esp_wifi.h`), pilha TCP/IP (`esp_netif.h`), loop de eventos (`esp_event.h`) e cliente HTTP integrado (`HTTP.hpp`) do ESP-IDF.

---

## 🚀 Recursos Principais

- **Múltiplos Modos de Operação:** Suporte nativo a Estação (`STA`), Ponto de Acesso (`AP`) e Híbrido (`AP_STA`).
- **Cliente HTTP Integrado:** Acesso direto via membro `rede.http` com suporte completo a verbos `GET`, `POST`, `PUT` e `PATCH`.
- **Certificados SSL/TLS Nativos:** Suporte a HTTPS via bundle de certificados do ESP-IDF (`esp_crt_bundle`) e certificados embutidos para APIs seguras e Google Cloud / Firebase Realtime Database.
- **Reconexão com Backoff Exponencial:** Gerenciamento inteligente de reconexões com temporizador `esp_timer`, evitando saturação do processador ou do rádio.
- **Varredura de Redes (Scan):** Mapeamento detalhado das redes disponíveis (SSID, RSSI, canal, BSSID e tipo de autenticação).
- **Métricas e Diagnóstico:** Métodos auxiliares para consulta de IP, Máscara de sub-rede, Gateway, endereço MAC e potência de sinal (RSSI).
- **Controle de Identificação:** Configuração simples de Hostname para identificação na rede local.

---

## 📦 Dependências

| Arquivo / Biblioteca | Papel no Módulo |
|---|---|
| `esp_wifi.h` | Driver nativo de rádio Wi-Fi do ESP-IDF |
| `esp_netif.h` | Camada de abstração de interface de rede e pilha TCP/IP LwIP |
| `esp_event.h` | Loop de eventos padrão para notificações de Wi-Fi e IP |
| `nvs_flash.h` | Armazenamento de dados de calibração e configuração RF |
| `esp_timer.h` | Temporizador de alta precisão para agendamento de reconexão |
| `esp_http_client.h` | Driver cliente de requisições HTTP/HTTPS |
| `esp_crt_bundle.h` | Repositório de CAs para validação SSL/TLS de conexões seguras |

> Inicializa e recupera automaticamente partições NVS corrompidas ou sem páginas livres durante a primeira inicialização.

---

## 📋 API da Classe `Network`

### Enum `ConfigRede`

```cpp
enum class ConfigRede {
    STA,    // Modo Estação: conecta-se a um roteador ou AP existente
    AP,     // Modo Access Point: cria sua própria rede Wi-Fi para outros dispositivos
    AP_STA  // Modo Híbrido: atua como cliente e ponto de acesso simultaneamente
};
```

---

### Construtor e Inicialização

| Método | Descrição |
|---|---|
| `Network(ConfigRede modo = ConfigRede::STA)` | Construtor configurando o modo de operação |
| `~Network()` | Destrutor: desconecta, para o rádio, desregistra eventos e limpa recursos |
| `esp_err_t estaInicializado() const` | Retorna o último código de erro ou `ESP_OK` se o hardware iniciou corretamente |

---

### Métodos de Conexão e Ponto de Acesso

| Método | Descrição |
|---|---|
| `esp_err_t conectar(ssid, senha, maxTentativas = 5)` | Configura e inicia o processo de conexão a uma rede Wi-Fi |
| `esp_err_t iniciarAP(ssid, senha = "", canal = 1, maxConexoes = 4)` | Cria uma rede própria (AP) aberta ou protegida por WPA2 |
| `esp_err_t iniciarAPSTA(apSsid, apSenha, staSsid, staSenha, maxTentativas = 5)` | Inicializa ambas as interfaces em modo híbrido simultâneo |
| `esp_err_t aguardarConexao(timeoutMs = 10000)` | Bloqueia a task de forma cooperativa até obter IP ou estourar o timeout |
| `bool estaConectado() const` | Retorna `true` se estiver autenticado e com IP obtido |
| `esp_err_t escanear()` | Realiza varredura das redes Wi-Fi locais e imprime tabela formatada |
| `ConfigRede modoAtual() const` | Retorna o modo em operação (`ConfigRede::STA`, `AP` ou `AP_STA`) |

---

### Diagnóstico de Rede e Configurações

| Método | Retorno | Descrição |
|---|---|---|
| `obterIP()` | `std::string` | Endereço IPv4 atribuído (ex: `"192.168.1.100"`) |
| `obterMascara()` | `std::string` | Máscara de sub-rede (ex: `"255.255.255.0"`) |
| `obterGateway()` | `std::string` | Endereço do gateway padrão (ex: `"192.168.1.1"`) |
| `obterMAC()` | `std::string` | Endereço físico MAC formatado (`"XX:XX:XX:XX:XX:XX"`) |
| `obterRSSI()` | `int8_t` | Potência de recepção do sinal em dBm (ou 0 se desconectado) |
| `definirHostname(nome)` | `bool` | Define o nome de host do ESP32 na rede local |

---

### Submódulo HTTP (`rede.http`)

Membro público acessível para efetuar chamadas REST/HTTP de forma síncrona:

| Método | Assinatura | Descrição |
|---|---|---|
| `get` | `get(url, outStatusCode = nullptr)` | Efetua requisição GET e retorna o corpo como `std::string` |
| `post` | `post(url, payload, contentType, outStatusCode)` | Envia dados via POST |
| `put` | `put(url, payload, contentType, outStatusCode)` | Envia substituição de recurso via PUT |
| `patch` | `patch(url, payload, contentType, outStatusCode)` | Envia modificação parcial via PATCH |

---

## 🛠️ Exemplos de Uso

### 1. Conexão Básica no Modo Estação (STA)

```cpp
#include "Network.hpp"
#include "Tempo.hpp"
#include "esp_log.h"

static const char* TAG = "ExemploWiFi";

extern "C" void app_main(void) {
    Network rede; // Modo STA padrão

    rede.definirHostname("meu-esp32-iot");
    rede.conectar("MinhaRedeWiFi", "senha123456");

    if (rede.aguardarConexao(10000) == ESP_OK) {
        ESP_LOGI(TAG, "Conectado com sucesso!");
        ESP_LOGI(TAG, "IP:      %s", rede.obterIP().c_str());
        ESP_LOGI(TAG, "Gateway: %s", rede.obterGateway().c_str());
        ESP_LOGI(TAG, "MAC:     %s", rede.obterMAC().c_str());
        ESP_LOGI(TAG, "Sinal:   %d dBm", rede.obterRSSI());
    } else {
        ESP_LOGE(TAG, "Falha ou timeout ao conectar.");
    }

    while (true) {
        Tempo::delayS(10);
    }
}
```

---

### 2. Escaneamento de Redes Disponíveis

```cpp
#include "Network.hpp"
#include "Tempo.hpp"

extern "C" void app_main(void) {
    Network rede;

    while (true) {
        // Realiza o scan e imprime lista formatada com SSIDs, RSSI e Auth
        rede.escanear();

        Tempo::delayS(10);
    }
}
```

---

### 3. Criando um Ponto de Acesso Próprio (AP)

```cpp
#include "Network.hpp"
#include "Tempo.hpp"
#include "esp_log.h"

extern "C" void app_main(void) {
    // Inicializa explicitamente em modo Access Point
    Network ap(ConfigRede::AP);

    // Cria rede protegida por senha com até 4 conexões simultâneas
    ap.iniciarAP("ESP32-Configuracao", "admin1234", 1, 4);

    ESP_LOGI("AP", "Ponto de acesso ativo! IP padrão: %s", ap.obterIP().c_str());

    while (true) {
        Tempo::delayS(5);
    }
}
```

---

### 4. Requisições REST/HTTP com JSON

Integração com a classe [JSON] para envio e consulta de APIs web:

```cpp
#include "Network.hpp"
#include "JSON.hpp"
#include "Tempo.hpp"
#include "esp_log.h"

static const char* TAG = "ExemploHTTP";

extern "C" void app_main(void) {
    static Network rede;
    rede.conectar("MinhaRedeWiFi", "senha123456");

    if (rede.aguardarConexao(15000) != ESP_OK) {
        ESP_LOGE(TAG, "Não foi possível conectar ao Wi-Fi.");
        return;
    }

    // 1. Requisição GET simples
    int status = 0;
    std::string respostaGet = rede.http.get("https://httpbin.org/get", &status);
    ESP_LOGI(TAG, "GET Status: %d | Resposta: %s", status, respostaGet.c_str());

    // 2. Requisição POST com JSON gerado dinamicamente
    JSON doc;
    doc.set("dispositivo", "ESP32-S3")
       .set("temperatura", 27.5)
       .set("ativo", true);

    int statusPost = 0;
    std::string respostaPost = rede.http.post(
        "https://httpbin.org/post",
        doc.toString(),
        "application/json",
        &statusPost
    );

    ESP_LOGI(TAG, "POST Status: %d | Tamanho: %d bytes", statusPost, (int)respostaPost.size());
}
```

---

## 🔒 Boas Práticas e Recomendações

> Para efetuar requisições HTTPS com validação rigorosa de certificados (ex: Firebase Realtime Database, AWS IoT, Google APIs), lembre-se de sincronizar o relógio do ESP32 via SNTP/NTP (`esp_netif_sntp.h`), pois datas desalinhadas causam rejeição imediata do handshake TLS.

> A senha de um Access Point (`iniciarAP`) em modo WPA2 deve possuir no mínimo 8 caracteres. Caso passada vazia, a rede será inicializada em modo aberto (`WIFI_AUTH_OPEN`).
