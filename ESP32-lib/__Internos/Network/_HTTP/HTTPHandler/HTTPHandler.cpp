#include <string>

#include "HTTPHandler.hpp"
#include "esp_log.h"

namespace {
    constexpr const char* TAG = "HTTPHandler";
}

/**
 * @brief Callback estático do cliente HTTP para tratar eventos de resposta.
 * 
 * @param evt Estrutura contendo informações sobre o evento HTTP ocorrido.
 * @return esp_err_t Código de status da execução do callback (ESP_OK para sucesso).
 */
esp_err_t HTTPHandler::httpEventHandler(esp_http_client_event_t *evt) {
    switch (evt->event_id) {
        case HTTP_EVENT_ON_DATA: {
            auto* responseBuffer = static_cast<std::string*>(evt->user_data);
            if (responseBuffer != nullptr && evt->data != nullptr && evt->data_len > 0) {
                responseBuffer->append(static_cast<char*>(evt->data), evt->data_len);
            }
            break;
        }
        case HTTP_EVENT_ERROR:
            ESP_LOGE(TAG, "Erro durante a execução HTTP");
            break;
        default:
            break;
    }
    return ESP_OK;
}