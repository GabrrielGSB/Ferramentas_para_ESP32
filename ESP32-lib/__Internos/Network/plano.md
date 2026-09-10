### **Fase 1: Fundação Sólida, Desacoplamento e Robustez do Wi-Fi**

> **Objetivo:** Deixar a camada de Wi-Fi à prova de falhas, sem travamentos repentinos (`abort`), com métricas legíveis e eventos limpos para a aplicação.
> 

#### **Passo 1.1: Eliminar a dependência de `Handler` (Desacoplamento)**

- **Problema atual:** `Handler.cpp` é amigo (`friend`) de `Network` e mistura callbacks de Wi-Fi e HTTP.
- **Solução:**
    1. Tornar o callback de eventos Wi-Fi um método estático privado da própria classe `Network` (ex: `Network::wifiEventHandler`).
    2. Passar a instância `this` no registro do `esp_event_handler_instance_register`.
    3. Deixar `HTTP` gerenciar seus próprios eventos internamente, eliminando `Handler.hpp` e `Handler.cpp`.

#### **Passo 1.2: Substituir `ESP_ERROR_CHECK` por Retorno Controlado**

- **Problema atual:** Se a NVS falhar ou o rádio não subir, o ESP32 reinicia em loop.
- **Solução:**
    1. Fazer `init()` retornar um `bool` ou `esp_err_t`.
    2. Tratar falhas parciais (ex: registrar log e setar um flag de erro interno `m_statusOk = false`).
    3. Permitir que métodos como `conectar()` e `iniciarAP()` retornem `bool` indicando se os parâmetros foram aceitos.

#### **Passo 1.3: Adicionar Getters de Rede e Configuração de Hostname**

- **Ações:**
    - Implementar `std::string obterIP() const;` usando `esp_netif_get_ip_info()`.
    - Implementar `std::string obterMascara() const;` e `std::string obterGateway() const;`.
    - Implementar `std::string obterMAC() const;` usando `esp_wifi_get_mac()`.
    - Implementar `int8_t obterRSSI() const;` usando `esp_wifi_sta_get_ap_info()`.
    - Implementar `bool definirHostname(const std::string& nome);` usando `esp_netif_set_hostname()`.

### **Passo 1.4: Configuração de IP Estático vs. DHCP**

- **Ações:**
    - Adicionar método `configurarIPEstato(ip, mascara, gateway, dns1, dns2)` que desativa o cliente DHCP da netif (`esp_netif_dhcpc_stop`) e aplica o IP fixo antes de conectar.
    - Adicionar método para reativar DHCP se desejado (`usarDHCP()`).

### **Passo 1.5: Reconexão com Backoff Exponencial**

- **Ações:**
    - No evento `WIFI_EVENT_STA_DISCONNECTED`, em vez de reconectar imediatamente com `esp_wifi_connect()`:
        - Calcular o atraso: $tempo = \min(t_{base} \times 2^{tentativa}, t_{max})$.
        - Utilizar um timer FreeRTOS (`esp_timer`) para disparar a reconexão sem travar a thread de eventos.

### **Passo 1.6: Callbacks C++ Modernos para a Aplicação**

- **Ações:**
    - Permitir que a aplicação registre closures com `std::function`:
        
        ```
        cpp
        net.onConectado([](conststd::string& ip) { ... });
        net.onDesconectado([](int motivo) { ... });
        net.onClienteAPConectado([](constuint8_t mac[6]) { ... });
        ```
        
    - Disparar esses callbacks a partir do manipulador de eventos central.

---

## **Fase 2: Serviços de Base, Persistência e Sincronização**

> **Objetivo:** Permitir que o dispositivo guarde suas configurações, sincronize a hora e se identifique na rede.
> 

### **Passo 2.1: Persistência de Credenciais em NVS**

- **Ações:**
    - Criar um helper ou submódulo de armazenamento (`ConfigStorage` ou `NVSManager`).
    - Criar métodos: `salvarCredenciais(ssid, senha)`, `carregarCredenciais(ssid, senha)` e `apagarCredenciais()`.

### **Passo 2.2: Sincronização de Relógio via SNTP (NTP)**

- **Ações:**
    - Criar a classe/módulo `NTP` utilizando a API nativa `esp_sntp.h`.
    - Configurar servidores padrão (`pool.ntp.org`, `time.google.com`).
    - Método `sincronizar(fusoHorario, timeoutMs)` e verificação de sincronismo `estaSincronizado()`.
    - **Importante:** Este passo deve anteceder requisições HTTPS e atualizações OTA para validação de certificados SSL.

### **Passo 2.3: Descoberta de Serviço via mDNS**

- **Ações:**
    - Integrar o componente `mdns` nativo do ESP-IDF.
    - Criar método `iniciarMDNS(const std::string& hostname, const std::string& nomeServico)`.
    - Permite acessar o dispositivo na mesma sub-rede por `http://<hostname>.local`.

---

## **Fase 3: Camada Web Completa (Cliente, Servidor e Portal Cativo)**

> **Objetivo:** Comunicação HTTP robusta e provisionamento fácil sem reprogramação.
> 

### **Passo 3.1: Modernização do Cliente HTTP**

- **Ações:**
    - Criar as estruturas de dados:
        - `struct HttpResponse { int statusCode; std::string body; std::map<std::string, std::string> headers; };`
    - Suportar requisições genéricas:
        - `HttpResponse get(url, headers);`
        - `HttpResponse post(url, body, contentType, headers);`
        - `HttpResponse put(...)`, `HttpResponse del(...)`.
    - Adicionar versão com callback para streaming de arquivos ou respostas gigantes.

### **Passo 3.2: Servidor HTTP Embarcado (`esp_http_server`)**

- **Ações:**
    - Criar uma classe `HttpServer` com sintaxe orientada a rotas:
        
        ```
        cpp
        server.on("/api/status", HTTP_GET, [](HttpRequest& req,HttpResponse& res) {
        res.send("{\"status\":\"ok\"}","application/json");
        });
        ```
        
    - Iniciar e parar o servidor em background via FreeRTOS.

### **Passo 3.3: Servidor DNS Cativo & Fluxo WiFiManager**

- **Ações:**
    - Criar uma task leve com socket UDP na porta 53 (`DnsServer`) que responde a qualquer consulta com o IP local do AP (192.168.4.1).
    - Criar uma página HTML servida pelo `HttpServer` contendo:
        - Lista de redes escaneadas.
        - Campos de texto para seleção de SSID e senha.
        - Endpoint `POST /salvar` para armazenar na NVS e reiniciar a conexão em modo STA.

---

## **Fase 4: Protocolos IoT Industriais (MQTT, WebSockets e OTA)**

> **Objetivo:** Tornar a biblioteca pronta para soluções de nuvem e monitoramento remoto.
> 

### **Passo 4.1: Cliente MQTT / MQTTS**

- **Ações:**
    - Criar a classe `MqttClient` encapsulando `esp_mqtt_client`.
    - Métodos intuitivos:
        - `conectar(brokerUrl, usuario, senha);`
        - `publicar(topico, mensagem, qos, retain);`
        - `inscrever(topico, callbackMensagem);`
    - Suporte a reconexão automática e mensagens de testamento (*Last Will and Testament*).

### **Passo 4.2: Atualização de Firmware Remota (OTA)**

- **Ações:**
    - Criar a classe `OtaUpdater` utilizando `esp_https_ota.h`.
    - Método `iniciarAtualizacao(urlFirmware, hashEsperado)`.
    - Progresso via callback percentual (`onProgresso(int porcentagem)`).
    - Rollback automático se o novo firmware falhar ao inicializar.

---

## **Fase 5: Segurança Avançada e Otimização de Recursos**

> **Objetivo:** Blindagem de segurança e eficiência energética.
> 

### **Passo 5.1: Gestão de Certificados Privados (mTLS / Pinning)**

- **Ações:**
    - Permitir passar certificados CA personalizados (strings PEM) além do bundle Mozilla padrão.
    - Suporte a autenticação mútua (certificado do cliente + chave privada).

### **Passo 5.2: Gestão de Energia do Wi-Fi**

- **Ações:**
    - Criar métodos para alternar entre `WIFI_PS_NONE`, `WIFI_PS_MIN_MODEM` e `WIFI_PS_MAX_MODEM`.
    - Permitir desligar o rádio completamente (`desconectarEDesligar()`) para economia em deep sleep.