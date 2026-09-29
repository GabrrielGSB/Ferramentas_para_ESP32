#include "Random.hpp"

float Random::floatRange(float min, float max) {
    if (min >= max) {
        return min;
    }
    uint32_t r = esp_random();
    return min + (static_cast<float>(r) / static_cast<float>(UINT32_MAX)) * (max - min);
}

int32_t Random::intRange(int32_t min, int32_t max) {
    if (min >= max) {
        return min;
    }
    uint32_t diff = static_cast<uint32_t>(max - min + 1);
    return min + static_cast<int32_t>(esp_random() % diff);
}

void Random::bytes(void* buf, size_t len) {
    if (buf != nullptr && len > 0) {
        esp_fill_random(buf, len);
    }
}
