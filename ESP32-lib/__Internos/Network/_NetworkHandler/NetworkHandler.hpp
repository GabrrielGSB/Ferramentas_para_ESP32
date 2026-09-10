#pragma once
#include "esp_event.h"
class Network; // Forward declaration

class NetworkHandler {
public:
    static void eventHandler(void* arg, esp_event_base_t eventBase, int32_t eventId, void* eventData);
};