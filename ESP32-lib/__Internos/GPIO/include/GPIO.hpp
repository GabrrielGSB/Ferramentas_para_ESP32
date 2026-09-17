#pragma once

#ifndef GPIO_HPP
#define GPIO_HPP

#include "driver/gpio.h"

#pragma region MODES
    #define OUTPUT       GPIO_MODE_OUTPUT 
    #define INPUT        GPIO_MODE_INPUT
    #define INPUT_OUTPUT GPIO_MODE_INPUT_OUTPUT
    #define PULLUP       GPIO_PULLUP_ONLY
    #define PULLDOWN     GPIO_PULLDOWN_ONLY
    #define NOPULL       GPIO_FLOATING
    #define OPEN_DRAIN   GPIO_MODE_OUTPUT_OD
#pragma endregion

#pragma region PINS
    #if   CONFIG_IDF_TARGET_ESP32
        #define GPIO0 GPIO_NUM_0
        #define GPIO1 GPIO_NUM_1
        #define GPIO2 GPIO_NUM_2
        #define GPIO3 GPIO_NUM_3
        #define GPIO4 GPIO_NUM_4
        #define GPIO5 GPIO_NUM_5
        #define GPIO6 GPIO_NUM_6
        #define GPIO7 GPIO_NUM_7
        #define GPIO8 GPIO_NUM_8
        #define GPIO9 GPIO_NUM_9
        #define GPIO10 GPIO_NUM_10
        #define GPIO11 GPIO_NUM_11
        #define GPIO12 GPIO_NUM_12
        #define GPIO13 GPIO_NUM_13
        #define GPIO14 GPIO_NUM_14
        #define GPIO15 GPIO_NUM_15
        #define GPIO16 GPIO_NUM_16
        #define GPIO17 GPIO_NUM_17
        #define GPIO18 GPIO_NUM_18
        #define GPIO19 GPIO_NUM_19
        #define GPIO20 GPIO_NUM_20
        #define GPIO21 GPIO_NUM_21
        #define GPIO22 GPIO_NUM_22
        #define GPIO23 GPIO_NUM_23
        #define GPIO24 GPIO_NUM_24
        #define GPIO25 GPIO_NUM_25
        #define GPIO26 GPIO_NUM_26
        #define GPIO27 GPIO_NUM_27
        #define GPIO28 GPIO_NUM_28
        #define GPIO29 GPIO_NUM_29
        #define GPIO30 GPIO_NUM_30
        #define GPIO31 GPIO_NUM_31
        #define GPIO32 GPIO_NUM_32
        #define GPIO33 GPIO_NUM_33
        #define GPIO34 GPIO_NUM_34
        #define GPIO35 GPIO_NUM_35
        #define GPIO36 GPIO_NUM_36
        #define GPIO39 GPIO_NUM_39
    #elif CONFIG_IDF_TARGET_ESP32S3
        #define GPIO0 GPIO_NUM_0
        #define GPIO1 GPIO_NUM_1
        #define GPIO2 GPIO_NUM_2
        #define GPIO3 GPIO_NUM_3
        #define GPIO4 GPIO_NUM_4
        #define GPIO5 GPIO_NUM_5
        #define GPIO6 GPIO_NUM_6
        #define GPIO7 GPIO_NUM_7
        #define GPIO8 GPIO_NUM_8
        #define GPIO9 GPIO_NUM_9
        #define GPIO10 GPIO_NUM_10
        #define GPIO11 GPIO_NUM_11
        #define GPIO12 GPIO_NUM_12
        #define GPIO13 GPIO_NUM_13
        #define GPIO14 GPIO_NUM_14
        #define GPIO15 GPIO_NUM_15
        #define GPIO16 GPIO_NUM_16
        #define GPIO17 GPIO_NUM_17
        #define GPIO18 GPIO_NUM_18
        #define GPIO19 GPIO_NUM_19
        #define GPIO20 GPIO_NUM_20
        #define GPIO21 GPIO_NUM_21
        #define GPIO26 GPIO_NUM_26
        #define GPIO27 GPIO_NUM_27
        #define GPIO28 GPIO_NUM_28
        #define GPIO29 GPIO_NUM_29
        #define GPIO30 GPIO_NUM_30
        #define GPIO31 GPIO_NUM_31
        #define GPIO32 GPIO_NUM_32
        #define GPIO33 GPIO_NUM_33
        #define GPIO34 GPIO_NUM_34
        #define GPIO35 GPIO_NUM_35
        #define GPIO36 GPIO_NUM_36
        #define GPIO38 GPIO_NUM_38
        #define GPIO39 GPIO_NUM_39
        #define GPIO40 GPIO_NUM_40
        #define GPIO41 GPIO_NUM_41
        #define GPIO42 GPIO_NUM_42
        #define GPIO43 GPIO_NUM_43
        #define GPIO44 GPIO_NUM_44
        #define GPIO45 GPIO_NUM_45
        #define GPIO46 GPIO_NUM_46
        #define GPIO47 GPIO_NUM_47
        #define GPIO48 GPIO_NUM_48

    #elif CONFIG_IDF_TARGET_ESP32C3
        #define GPIO0 GPIO_NUM_0
        #define GPIO1 GPIO_NUM_1
        #define GPIO2 GPIO_NUM_2
        #define GPIO3 GPIO_NUM_3
        #define GPIO4 GPIO_NUM_4
        #define GPIO5 GPIO_NUM_5
        #define GPIO6 GPIO_NUM_6
        #define GPIO7 GPIO_NUM_7
        #define GPIO8 GPIO_NUM_8
        #define GPIO9 GPIO_NUM_9
        #define GPIO10 GPIO_NUM_10
        #define GPIO11 GPIO_NUM_11
        #define GPIO12 GPIO_NUM_12
        #define GPIO13 GPIO_NUM_13
        #define GPIO14 GPIO_NUM_14
        #define GPIO15 GPIO_NUM_15
        #define GPIO16 GPIO_NUM_16
        #define GPIO17 GPIO_NUM_17
        #define GPIO18 GPIO_NUM_18
        #define GPIO19 GPIO_NUM_19
        #define GPIO20 GPIO_NUM_20
        #define GPIO21 GPIO_NUM_21

    #else
        #error "Modelo de ESP32 não suportado pela biblioteca de GPIO!"
    #endif
#pragma endregion

class GPIO {
protected:
    gpio_num_t pino;           
    gpio_mode_t modo;          
    uint8_t estado_atual;      

public:
    /**
     * @brief Construtor da classe GPIO. Reseta o pino, define sua direção e configura resistores de pull.
     * @param numPino Número do pino GPIO (ex: GPIO2, GPIO4).
     * @param modoPino Direção de operação (OUTPUT, INPUT, INPUT_OUTPUT, OPEN_DRAIN).
     * @param modoPull Configuração do resistor interno (NOPULL, PULLUP, PULLDOWN). Padrão: NOPULL.
     */
    GPIO(gpio_num_t numPino, gpio_mode_t modoPino, gpio_pull_mode_t modoPull = NOPULL);

    /**
     * @brief Define o nível lógico alto (1 / 3.3V) no pino.
     * @note Válido apenas para pinos configurados como OUTPUT ou INPUT_OUTPUT.
     */
    void ligar();

    /**
     * @brief Define o nível lógico baixo (0 / 0V) no pino.
     * @note Válido apenas para pinos configurados como OUTPUT ou INPUT_OUTPUT.
     */
    void desligar();

    /**
     * @brief Inverte o estado lógico atual da saída digital.
     * @note Válido apenas para pinos configurados como OUTPUT ou INPUT_OUTPUT.
     */
    void inverter();

    /**
     * @brief Realiza a leitura do nível lógico atual presente no pino.
     * @return int Nível lógico lido do hardware (1 ou 0).
     */
    bool ler();

    /**
     * @brief Altera dinamicamente a configuração do resistor de pull interno.
     * @param modoPull Novo modo do resistor de pull (NOPULL, PULLUP, PULLDOWN).
     * @note Tem efeito apenas em pinos configurados como INPUT ou INPUT_OUTPUT.
     */
    void configPull(gpio_pull_mode_t modoPull);
};

#endif