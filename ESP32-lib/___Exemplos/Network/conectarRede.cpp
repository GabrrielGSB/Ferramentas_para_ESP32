#include "Network.hpp"
#include "Tempo.hpp"

extern "C" void app_main(void) {
    Network rede;

    rede.conectar("myssid","21022002");

    while (true) {
        delay_s(5);
    }
}