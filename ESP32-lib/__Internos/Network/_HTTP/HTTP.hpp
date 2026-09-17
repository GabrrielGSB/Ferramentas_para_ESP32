#pragma once
#include <string>
#include "esp_event.h"
#include "esp_http_client.h"

class HTTP {
    public:
        std::string get(const std::string& url, int* outStatusCode = nullptr);
        std::string post(const std::string& url, 
                             const std::string& payload, 
                             const std::string& contentType = "application/json", 
                             int* outStatusCode = nullptr);

        std::string patch(const std::string& url, 
                              const std::string& payload, 
                              const std::string& contentType = "application/json", 
                              int* outStatusCode = nullptr);

        std::string put(const std::string& url, 
                        const std::string& payload, 
                        const std::string& contentType = "application/json", 
                        int* outStatusCode = nullptr);

    private:
        std::string executar(esp_http_client_method_t metodo,
                             const std::string& url,
                             const std::string& payload = "",
                             const std::string& contentType = "",
                             int* outStatusCode = nullptr);
};