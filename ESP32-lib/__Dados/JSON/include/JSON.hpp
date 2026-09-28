#pragma once

#ifndef JSON_HPP
#define JSON_HPP

#include <string>
#include <vector>
#include <cstdint>
#include "cJSON.h"

/**
 * @brief Classe para manipulação simplificada, segura e moderna de JSON no ESP32.
 * 
 * Encapsula a biblioteca cJSON do ESP-IDF garantindo gerenciamento automático de memória (RAII),
 * métodos fluentes para encadeamento, sobrecargas de tipos e tratamento seguro de erros.
 */
class JSON {
    public:
        // =========================================================================
        // CONSTRUTORES E DESTRUTOR 
            /**
            * @brief Cria um novo objeto JSON vazio `{}`
            */
            JSON();

            /**
            * @brief Envolve um ponteiro cJSON existente.
            * @param rawNode Ponteiro bruto do cJSON.
            * @param owner Se true, a instância JSON será responsável por deletar o nó no destrutor.
            */
            explicit JSON(cJSON* rawNode, bool owner = true);

            /**
            * @brief Destrutor. Libera a memória associada se for a dona (owner) da árvore.
            */
            ~JSON();

            // Desabilita cópia rasa para evitar double-free de ponteiros cJSON
            JSON(const JSON&) = delete;
            JSON& operator=(const JSON&) = delete;

            // Habilita movimentação (Move Semantics)
            JSON(JSON&& other) noexcept;
            JSON& operator=(JSON&& other) noexcept;
        // =========================================================================
        
        // =========================================================================
        // MÉTODOS ESTÁTICOS / FACTORY
            /**
            * @brief Faz o parse de uma string JSON para um objeto JSON.
            * @param jsonString String contendo o JSON válido.
            * @return Instância JSON com os dados parseados (ou inválida se houver erro).
            */
            static JSON parse(const std::string& jsonString);

            /**
            * @brief Cria uma nova lista/array JSON vazia `[]`.
            * @return Instância JSON representando um array.
            */
            static JSON criarLista();
        // =========================================================================
        
        // =========================================================================
        // MODIFICAÇÃO / ADIÇÃO DE CAMPOS (OBJETO)
            JSON& set(const std::string& chave, int                valor);
            JSON& set(const std::string& chave, int64_t            valor);
            JSON& set(const std::string& chave, double             valor);
            JSON& set(const std::string& chave, bool               valor);
            JSON& set(const std::string& chave, const char*        valor);
            JSON& set(const std::string& chave, const std::string& valor);
            JSON& set(const std::string& chave, JSON&&             subObjeto);
        // =========================================================================

        // =========================================================================
        // MODIFICAÇÃO / ADIÇÃO DE ITENS (LISTA)
            JSON& add(int                valor);
            JSON& add(int64_t            valor);
            JSON& add(double             valor);
            JSON& add(bool               valor);
            JSON& add(const char*        valor);
            JSON& add(const std::string& valor);
            JSON& add(JSON&&             subObjeto);
        // =========================================================================

        // =========================================================================
        // LEITURA E CONSULTA DE CAMPOS
            /**
            * @brief Verifica se uma chave existe no objeto atual.
            */
            bool temChave(const std::string& chave) const;

            /**
            * @brief Obtém o valor inteiro de uma chave.
            */
            int getInt(const std::string& chave, int padrao = 0) const;

            /**
            * @brief Obtém o valor numérico de 64 bits de uma chave.
            */
            int64_t getInt64(const std::string& chave, int64_t padrao = 0) const;

            /**
            * @brief Obtém o valor de ponto flutuante de uma chave.
            */
            double getDouble(const std::string& chave, double padrao = 0.0) const;

            /**
            * @brief Obtém o valor booleano de uma chave.
            */
            bool getBool(const std::string& chave, bool padrao = false) const;

            /**
            * @brief Obtém o valor textual de uma chave.
            */
            std::string getString(const std::string& chave, const std::string& padrao = "") const;

            /**
            * @brief Obtém um sub-objeto ou array como uma nova visualização JSON (não é dona do ponteiro).
            */
            JSON getObject(const std::string& chave) const;
        // =========================================================================

        // =========================================================================
        // OPERAÇÕES EM ARRAYS
            /**
            * @brief Retorna o tamanho do array JSON (ou número de campos se for objeto).
            */
            size_t tamanho() const;

            /**
            * @brief Obtém um item do array pelo índice numérico.
            */
            JSON getItem(size_t index) const;
        // =========================================================================

        // =========================================================================
        // SERIALIZAÇÃO
            /**
            * @brief Converte a estrutura JSON para uma string formatada ou compacta.
            * @param formatado Se true, inclui quebras de linha e tabulações (pretty-print).
            * @return std::string com o JSON serializado.
            */
            std::string toString(bool formatado = false) const;
        // =========================================================================

        // =========================================================================
        // UTILITÁRIOS E ESTADO
            /**
            * @brief Verifica se a estrutura é válida (ponteiro cJSON não nulo).
            */
            bool isValida() const;

            /**
            * @brief Verifica se o nó atual é um Array `[]`.
            */
            bool isLista() const;

            /**
            * @brief Verifica se o nó atual é um Objeto `{}`.
            */
            bool isObjeto() const;

            /**
            * @brief Limpa e reseta a estrutura para um novo objeto vazio.
            */
            void clear();

            /**
            * @brief Acesso ao ponteiro bruto cJSON para integrações com APIs que exigem cJSON*.
            */
            cJSON* raw() const { return _raiz; }

            /**
            * @brief Desvincula a posse do ponteiro cJSON da classe, evitando que ele seja deletado.
            */
            cJSON* release();
        // =========================================================================
    
    private:
        cJSON* _raiz;
        bool   _isProprietario;
};

#endif // JSON_HPP
