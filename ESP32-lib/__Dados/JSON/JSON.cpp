#include "JSON.hpp"
#include <cstdlib>

// =========================================================================
// CONSTRUTORES E DESTRUTOR 
    JSON::JSON() : _raiz(cJSON_CreateObject()), _isProprietario(true) {
    }

    JSON::JSON(cJSON* rawNode, bool owner) : _raiz(rawNode), _isProprietario(owner) {
    }

    JSON::~JSON() {
        if (_isProprietario && _raiz != nullptr) {
            cJSON_Delete(_raiz);
            _raiz = nullptr;
        }
    }

    JSON::JSON(JSON&& other) noexcept : _raiz(other._raiz), _isProprietario(other._isProprietario) {
        other._raiz = nullptr;
        other._isProprietario = false;
    }

    JSON& JSON::operator=(JSON&& other) noexcept {
        if (this != &other) {
            if (_isProprietario && _raiz != nullptr) {
                cJSON_Delete(_raiz);
            }
            _raiz = other._raiz;
            _isProprietario = other._isProprietario;

            other._raiz = nullptr;
            other._isProprietario = false;
        }
        return *this;
    }
// =========================================================================

// =========================================================================
// MÉTODOS ESTÁTICOS / FACTORY
    JSON JSON::parse(const std::string& jsonString) {
        if (jsonString.empty()) {
            return JSON(nullptr, false);
        }
        cJSON* parsed = cJSON_Parse(jsonString.c_str());
        return JSON(parsed, true);
    }

    JSON JSON::criarLista() {
        return JSON(cJSON_CreateArray(), true);
    }
// =========================================================================

// =========================================================================
// MODIFICAÇÃO / ADIÇÃO DE CAMPOS (OBJETO)
    JSON& JSON::set(const std::string& chave, int valor) {
        if (!_raiz) return *this;
        cJSON_DeleteItemFromObjectCaseSensitive(_raiz, chave.c_str());
        cJSON_AddNumberToObject(_raiz, chave.c_str(), static_cast<double>(valor));
        return *this;
    }

    JSON& JSON::set(const std::string& chave, int64_t valor) {
        if (!_raiz) return *this;
        cJSON_DeleteItemFromObjectCaseSensitive(_raiz, chave.c_str());
        cJSON_AddNumberToObject(_raiz, chave.c_str(), static_cast<double>(valor));
        return *this;
    }

    JSON& JSON::set(const std::string& chave, double valor) {
        if (!_raiz) return *this;
        cJSON_DeleteItemFromObjectCaseSensitive(_raiz, chave.c_str());
        cJSON_AddNumberToObject(_raiz, chave.c_str(), valor);
        return *this;
    }

    JSON& JSON::set(const std::string& chave, bool valor) {
        if (!_raiz) return *this;
        cJSON_DeleteItemFromObjectCaseSensitive(_raiz, chave.c_str());
        cJSON_AddBoolToObject(_raiz, chave.c_str(), valor);
        return *this;
    }

    JSON& JSON::set(const std::string& chave, const char* valor) {
        if (!_raiz) return *this;
        cJSON_DeleteItemFromObjectCaseSensitive(_raiz, chave.c_str());
        if (valor) {
            cJSON_AddStringToObject(_raiz, chave.c_str(), valor);
        } else {
            cJSON_AddNullToObject(_raiz, chave.c_str());
        }
        return *this;
    }

    JSON& JSON::set(const std::string& chave, const std::string& valor) {
        return set(chave, valor.c_str());
    }

    JSON& JSON::set(const std::string& chave, JSON&& subObjeto) {
        if (!_raiz || !subObjeto._raiz) return *this;
        cJSON_DeleteItemFromObjectCaseSensitive(_raiz, chave.c_str());
        cJSON_AddItemToObject(_raiz, chave.c_str(), subObjeto.release());
        return *this;
    }
// =========================================================================

// =========================================================================
// MODIFICAÇÃO / ADIÇÃO DE ITENS (LISTA)
    JSON& JSON::add(int valor) {
        if (!_raiz) return *this;
        cJSON_AddItemToArray(_raiz, cJSON_CreateNumber(static_cast<double>(valor)));
        return *this;
    }

    JSON& JSON::add(int64_t valor) {
        if (!_raiz) return *this;
        cJSON_AddItemToArray(_raiz, cJSON_CreateNumber(static_cast<double>(valor)));
        return *this;
    }

    JSON& JSON::add(double valor) {
        if (!_raiz) return *this;
        cJSON_AddItemToArray(_raiz, cJSON_CreateNumber(valor));
        return *this;
    }

    JSON& JSON::add(bool valor) {
        if (!_raiz) return *this;
        cJSON_AddItemToArray(_raiz, cJSON_CreateBool(valor));
        return *this;
    }

    JSON& JSON::add(const char* valor) {
        if (!_raiz) return *this;
        if (valor) {
            cJSON_AddItemToArray(_raiz, cJSON_CreateString(valor));
        } else {
            cJSON_AddItemToArray(_raiz, cJSON_CreateNull());
        }
        return *this;
    }

    JSON& JSON::add(const std::string& valor) {
        return add(valor.c_str());
    }

    JSON& JSON::add(JSON&& subObjeto) {
        if (!_raiz || !subObjeto._raiz) return *this;
        cJSON_AddItemToArray(_raiz, subObjeto.release());
        return *this;
    }
// =========================================================================

// =========================================================================
// LEITURA E CONSULTA DE CAMPOS
    bool JSON::temChave(const std::string& chave) const {
        if (!_raiz) return false;
        return cJSON_HasObjectItem(_raiz, chave.c_str());
    }

    int JSON::getInt(const std::string& chave, int padrao) const {
        if (!_raiz) return padrao;
        cJSON* item = cJSON_GetObjectItemCaseSensitive(_raiz, chave.c_str());
        if (cJSON_IsNumber(item)) {
            return item->valueint;
        }
        return padrao;
    }

    int64_t JSON::getInt64(const std::string& chave, int64_t padrao) const {
        if (!_raiz) return padrao;
        cJSON* item = cJSON_GetObjectItemCaseSensitive(_raiz, chave.c_str());
        if (cJSON_IsNumber(item)) {
            return static_cast<int64_t>(item->valuedouble);
        }
        return padrao;
    }

    double JSON::getDouble(const std::string& chave, double padrao) const {
        if (!_raiz) return padrao;
        cJSON* item = cJSON_GetObjectItemCaseSensitive(_raiz, chave.c_str());
        if (cJSON_IsNumber(item)) {
            return item->valuedouble;
        }
        return padrao;
    }

    bool JSON::getBool(const std::string& chave, bool padrao) const {
        if (!_raiz) return padrao;
        cJSON* item = cJSON_GetObjectItemCaseSensitive(_raiz, chave.c_str());
        if (cJSON_IsBool(item)) {
            return cJSON_IsTrue(item);
        }
        return padrao;
    }

    std::string JSON::getString(const std::string& chave, const std::string& padrao) const {
        if (!_raiz) return padrao;
        cJSON* item = cJSON_GetObjectItemCaseSensitive(_raiz, chave.c_str());
        if (cJSON_IsString(item) && (item->valuestring != nullptr)) {
            return std::string(item->valuestring);
        }
        return padrao;
    }

    JSON JSON::getObject(const std::string& chave) const {
        if (!_raiz) return JSON(nullptr, false);
        cJSON* item = cJSON_GetObjectItemCaseSensitive(_raiz, chave.c_str());
        return JSON(item, false); // Não é o dono, evita deletar sub-nó isolado
    }
// =========================================================================

// =========================================================================
// OPERAÇÕES EM ARRAYS / LISTAS
    size_t JSON::tamanho() const {
        if (!_raiz) return 0;
        return static_cast<size_t>(cJSON_GetArraySize(_raiz));
    }

    JSON JSON::getItem(size_t index) const {
        if (!_raiz) return JSON(nullptr, false);
        cJSON* item = cJSON_GetArrayItem(_raiz, static_cast<int>(index));
        return JSON(item, false); // Não é o dono, é apenas uma referência
    }
// =========================================================================

// =========================================================================
// SERIALIZAÇÃO
    std::string JSON::toString(bool formatado) const {
    if (!_raiz) return "";

    char* render = formatado ? cJSON_Print(_raiz) : cJSON_PrintUnformatted(_raiz);
    if (!render) {
        return "";
    }

    std::string resultado(render);
    cJSON_free(render);
    return resultado;
    }
// =========================================================================

// =========================================================================
// UTILITÁRIOS E ESTADO
    bool JSON::isValida() const {
        return _raiz != nullptr;
    }

    bool JSON::isLista() const {
        return _raiz != nullptr && cJSON_IsArray(_raiz);
    }

    bool JSON::isObjeto() const {
        return _raiz != nullptr && cJSON_IsObject(_raiz);
    }

    void JSON::clear() {
        if (_isProprietario && _raiz != nullptr) {
            cJSON_Delete(_raiz);
        }
        _raiz = cJSON_CreateObject();
        _isProprietario = true;
    }

    cJSON* JSON::release() {
        cJSON* temp = _raiz;
        _raiz = nullptr;
        _isProprietario = false;
        return temp;
    }
// =========================================================================
