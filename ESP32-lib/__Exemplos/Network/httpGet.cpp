#include "Network.hpp"
#include "Tempo.hpp"

void tarefa_http(void* pvParameters) {
    auto* rede = static_cast<Network*>(pvParameters);

    while (true) {
        std::string resposta = rede->httpGet("https://timeapi.io/api/Time/current/zone?timeZone=America/Sao_Paulo");
        delay_s(10);
    }
}

extern "C" void app_main(void) {
    static Network rede;
    rede.conectar("myssid", "21022002");

    if (rede.aguardarConexao(10000)) {
        xTaskCreate(
            tarefa_http,
            "tarefa_http",
            8192,
            &rede,
            5,
            nullptr
        );
    } else {
        ESP_LOGE("MAIN", "Nao foi possivel conectar a rede Wi-Fi a tempo.");
    }
}