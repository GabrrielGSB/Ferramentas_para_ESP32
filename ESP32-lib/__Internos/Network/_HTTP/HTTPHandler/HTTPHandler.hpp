#pragma once
#include "esp_http_client.h"

class HTTPHandler {
public:
    static esp_err_t httpEventHandler(esp_http_client_event_t* evt);
};
