#include "HTTP.hpp"
#include "Network.hpp" 
#include "HTTPHandler.hpp" 

#include "esp_log.h"
#include "esp_crt_bundle.h"

namespace {
    constexpr const char* TAG = "HTTP";
}

/**
 * @brief Realiza uma requisição HTTP GET para a URL especificada e retorna o corpo da resposta.
 * 
 * @param url URL completa do recurso a ser acessado (ex.: "http://example.com/api/data").
 * @return std::string Corpo da resposta HTTP como string. Retorna string vazia em caso de erro.
 */
std::string HTTP::httpGet(const std::string& url) {
    std::string responseBody;

    esp_http_client_config_t config = {};
        config.url = url.c_str();
        config.event_handler = HTTPHandler::httpEventHandler; 
        config.user_data = &responseBody;
        config.timeout_ms = 8000;
        config.crt_bundle_attach = esp_crt_bundle_attach;

    esp_http_client_handle_t client = esp_http_client_init(&config);

    if (client == nullptr) {
        ESP_LOGE("Network", "Falha ao inicializar esp_http_client");
        return "";
    }

    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        int statusCode = esp_http_client_get_status_code(client);
        ESP_LOGI("Network", "HTTP GET finalizado com status: %d", statusCode);
        ESP_LOGI("Network", "Resposta: %s", responseBody.c_str());
    } 
    else {
        ESP_LOGE("Network", "Falha na requisição HTTP: %s", esp_err_to_name(err));
        responseBody.clear();
    }

    esp_http_client_cleanup(client);
    return responseBody;
}