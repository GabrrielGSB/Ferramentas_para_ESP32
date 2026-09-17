#include <cstdio>
#include "esp_system.h"
#include "JSON.hpp"
#include "Tempo.hpp"

extern "C" void app_main(void) {
    printf("==================================================\n");
    printf("          DEMONSTRAÇÃO DA CLASSE JSON            \n");
    printf("==================================================\n");

    // =========================================================================
    // 1. CRIANDO UM JSON DE TELEMETRIA
    // =========================================================================
    JSON telemetria;
    telemetria.set("dispositivo", "ESP32-S3-WROOM")
              .set("uptime_ms", (int64_t)millis())
              .set("temperatura", 27.8)
              .set("status_ok", true)
              .set("heap_livre", (int)esp_get_free_heap_size());

    // Criando sub-objeto de conectividade
    JSON wifi;
    wifi.set("ssid", "Rede_IoT_Producao")
        .set("ip", "192.168.1.105")
        .set("rssi", -58);

    telemetria.set("wifi", std::move(wifi));

    // Criando um array de alertas/sensores
    JSON sensores = JSON::createArray();
    sensores.add("BMP280")
            .add("AHT10")
            .add("WS2812");

    telemetria.set("sensores", std::move(sensores));

    // Serializando
    std::string compacto = telemetria.toString(false);
    std::string bonito   = telemetria.toString(true);

    printf("\n[JSON Compacto]:\n%s\n", compacto.c_str());
    printf("\n[JSON Formatado / Pretty]:\n%s\n", bonito.c_str());

    // =========================================================================
    // 2. PARSE E CONSULTA DE CAMPOS
    // =========================================================================
    printf("\n--- Testando Parsing e Leitura ---\n");
    const std::string payloadEntrada = "{\"comando\":\"ligar_rele\",\"canal\":1,\"potencia\":85.5,\"ativo\":true}";

    JSON docRecebido = JSON::parse(payloadEntrada);

    if (docRecebido.isValid()) {
        std::string comando = docRecebido.getString("comando");
        int canal           = docRecebido.getInt("canal");
        double potencia     = docRecebido.getDouble("potencia");
        bool ativo          = docRecebido.getBool("ativo");
        int timeout         = docRecebido.getInt("timeout_segundos", 30); // Usando valor default

        printf("Comando : %s\n", comando.c_str());
        printf("Canal   : %d\n", canal);
        printf("Potencia: %.1f%%\n", potencia);
        printf("Ativo   : %s\n", ativo ? "SIM" : "NAO");
        printf("Timeout : %d s (Padrao retornado)\n", timeout);
    } else {
        printf("Falha ao analisar payload JSON de entrada!\n");
    }

    printf("\n==================================================\n");
}
