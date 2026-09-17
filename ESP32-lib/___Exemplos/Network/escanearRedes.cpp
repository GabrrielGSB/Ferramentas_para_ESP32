#include "Network.hpp"
#include "Tempo.hpp"

extern "C" void app_main(void) {
    Network rede; 

    while (true) {
        rede.escanear();

        ESP_LOGI(TAG, "Aguardando 5 segundos para a próxima varredura...");
        delay_s(5);
    }
}