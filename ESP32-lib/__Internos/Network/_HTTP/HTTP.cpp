#include "HTTP.hpp"
#include "Network.hpp" 
#include "HTTPHandler.hpp" 

#include "esp_log.h"
#include "esp_crt_bundle.h"

namespace {
    constexpr const char* TAG = "HTTP";

    // Certificado raiz oficial do Google Trust Services (GTS Root R1)
    // Extraído do pacote de CAs oficial da Mozilla / ESP-IDF.
    // Abrange Firebase Realtime Database, Firestore, Cloud Functions e APIs Google.
    constexpr const char* GOOGLE_CA_PEM = 
        "-----BEGIN CERTIFICATE-----\n"
        "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\n"
        "gm3c\n"
        "-----END CERTIFICATE-----\n";
}

/**
 * @brief Realiza uma requisição HTTP GET para a URL especificada e retorna o corpo da resposta.
 * 
 * @param url URL completa do recurso a ser acessado (ex.: "http://example.com/api/data").
 * @return std::string Corpo da resposta HTTP como string. Retorna string vazia em caso de erro.
 */
std::string HTTP::get(const std::string& url, int* outStatusCode) {
    return executar(HTTP_METHOD_GET, url, "", "", outStatusCode);
}

std::string HTTP::post(const std::string& url, 
                           const std::string& payload, 
                           const std::string& contentType, 
                           int* outStatusCode) {
    return executar(HTTP_METHOD_POST, url, payload, contentType, outStatusCode);
}

std::string HTTP::patch(const std::string& url, 
                            const std::string& payload, 
                            const std::string& contentType, 
                            int* outStatusCode) {
    return executar(HTTP_METHOD_PATCH, url, payload, contentType, outStatusCode);
}

std::string HTTP::put(const std::string& url, 
                          const std::string& payload, 
                          const std::string& contentType, 
                          int* outStatusCode) {
    return executar(HTTP_METHOD_PUT, url, payload, contentType, outStatusCode);
}

std::string HTTP::executar(esp_http_client_method_t metodo,
                           const std::string& url,
                           const std::string& payload,
                           const std::string& contentType,
                           int* outStatusCode) {
    std::string responseBody;

    if (outStatusCode != nullptr) {
        *outStatusCode = -1;
    }

    esp_http_client_config_t config = {};
    config.url = url.c_str();
    config.event_handler = HTTPHandler::httpEventHandler; 
    config.user_data = &responseBody;
    config.timeout_ms = 10000;
    config.max_redirection_count = 5;

    // Se a requisição for para o Firebase ou Google, utiliza diretamente o certificado raiz do Google Trust Services
    if (url.find("firebase") != std::string::npos || url.find("google") != std::string::npos) {
        config.cert_pem = GOOGLE_CA_PEM;
    } else {
        config.crt_bundle_attach = esp_crt_bundle_attach;
    }

    esp_http_client_handle_t client = esp_http_client_init(&config);

    if (client == nullptr) {
        ESP_LOGE(TAG, "Falha ao inicializar esp_http_client");
        return "";
    }

    esp_http_client_set_method(client, metodo);

    // Se for POST, PATCH ou PUT, configura cabeçalho e payload
    if (metodo == HTTP_METHOD_POST || metodo == HTTP_METHOD_PATCH || metodo == HTTP_METHOD_PUT) {
        if (!contentType.empty()) {
            esp_http_client_set_header(client, "Content-Type", contentType.c_str());
        }
        if (!payload.empty()) {
            esp_http_client_set_post_field(client, payload.c_str(), static_cast<int>(payload.length()));
        }
    }

    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        int statusCode = esp_http_client_get_status_code(client);
        if (outStatusCode != nullptr) {
            *outStatusCode = statusCode;
        }
        ESP_LOGI(TAG, "HTTP perform finalizado com status: %d", statusCode);
        ESP_LOGD(TAG, "Resposta recebida: %s", responseBody.c_str());
    } else {
        ESP_LOGE(TAG, "Falha na requisição HTTP: %s", esp_err_to_name(err));
        responseBody.clear();
    }

    esp_http_client_cleanup(client);
    return responseBody;
}