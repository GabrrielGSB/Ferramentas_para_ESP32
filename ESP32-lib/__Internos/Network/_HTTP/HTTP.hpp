#pragma once
#include <string>
#include "esp_event.h"
#include "esp_http_client.h"

class HTTP {
    public:
        std::string httpGet(const std::string& url);
};