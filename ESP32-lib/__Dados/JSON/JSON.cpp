#include "JSON.hpp"
#include <cstdlib>

// =========================================================================
// CONSTRUTORES E DESTRUTOR (RAII)
// =========================================================================

JSON::JSON() : _root(cJSON_CreateObject()), _isOwner(true) {
}

JSON::JSON(cJSON* rawNode, bool owner) : _root(rawNode), _isOwner(owner) {
}

JSON::~JSON() {
    if (_isOwner && _root != nullptr) {
        cJSON_Delete(_root);
        _root = nullptr;
    }
}

JSON::JSON(JSON&& other) noexcept : _root(other._root), _isOwner(other._isOwner) {
    other._root = nullptr;
    other._isOwner = false;
}

JSON& JSON::operator=(JSON&& other) noexcept {
    if (this != &other) {
        if (_isOwner && _root != nullptr) {
            cJSON_Delete(_root);
        }
        _root = other._root;
        _isOwner = other._isOwner;

        other._root = nullptr;
        other._isOwner = false;
    }
    return *this;
}

// =========================================================================
// MÉTODOS ESTÁTICOS / FACTORY
// =========================================================================

JSON JSON::parse(const std::string& jsonString) {
    if (jsonString.empty()) {
        return JSON(nullptr, false);
    }
    cJSON* parsed = cJSON_Parse(jsonString.c_str());
    return JSON(parsed, true);
}

JSON JSON::createArray() {
    return JSON(cJSON_CreateArray(), true);
}

// =========================================================================
// MODIFICAÇÃO / ADIÇÃO DE CAMPOS (OBJETO)
// =========================================================================

JSON& JSON::set(const std::string& chave, int valor) {
    if (!_root) return *this;
    cJSON_DeleteItemFromObjectCaseSensitive(_root, chave.c_str());
    cJSON_AddNumberToObject(_root, chave.c_str(), static_cast<double>(valor));
    return *this;
}

JSON& JSON::set(const std::string& chave, int64_t valor) {
    if (!_root) return *this;
    cJSON_DeleteItemFromObjectCaseSensitive(_root, chave.c_str());
    cJSON_AddNumberToObject(_root, chave.c_str(), static_cast<double>(valor));
    return *this;
}

JSON& JSON::set(const std::string& chave, double valor) {
    if (!_root) return *this;
    cJSON_DeleteItemFromObjectCaseSensitive(_root, chave.c_str());
    cJSON_AddNumberToObject(_root, chave.c_str(), valor);
    return *this;
}

JSON& JSON::set(const std::string& chave, bool valor) {
    if (!_root) return *this;
    cJSON_DeleteItemFromObjectCaseSensitive(_root, chave.c_str());
    cJSON_AddBoolToObject(_root, chave.c_str(), valor);
    return *this;
}

JSON& JSON::set(const std::string& chave, const char* valor) {
    if (!_root) return *this;
    cJSON_DeleteItemFromObjectCaseSensitive(_root, chave.c_str());
    if (valor) {
        cJSON_AddStringToObject(_root, chave.c_str(), valor);
    } else {
        cJSON_AddNullToObject(_root, chave.c_str());
    }
    return *this;
}

JSON& JSON::set(const std::string& chave, const std::string& valor) {
    return set(chave, valor.c_str());
}

JSON& JSON::set(const std::string& chave, JSON&& subObjeto) {
    if (!_root || !subObjeto._root) return *this;
    cJSON_DeleteItemFromObjectCaseSensitive(_root, chave.c_str());
    cJSON_AddItemToObject(_root, chave.c_str(), subObjeto.release());
    return *this;
}

// =========================================================================
// MODIFICAÇÃO / ADIÇÃO DE ITENS (ARRAY)
// =========================================================================

JSON& JSON::add(int valor) {
    if (!_root) return *this;
    cJSON_AddItemToArray(_root, cJSON_CreateNumber(static_cast<double>(valor)));
    return *this;
}

JSON& JSON::add(int64_t valor) {
    if (!_root) return *this;
    cJSON_AddItemToArray(_root, cJSON_CreateNumber(static_cast<double>(valor)));
    return *this;
}

JSON& JSON::add(double valor) {
    if (!_root) return *this;
    cJSON_AddItemToArray(_root, cJSON_CreateNumber(valor));
    return *this;
}

JSON& JSON::add(bool valor) {
    if (!_root) return *this;
    cJSON_AddItemToArray(_root, cJSON_CreateBool(valor));
    return *this;
}

JSON& JSON::add(const char* valor) {
    if (!_root) return *this;
    if (valor) {
        cJSON_AddItemToArray(_root, cJSON_CreateString(valor));
    } else {
        cJSON_AddItemToArray(_root, cJSON_CreateNull());
    }
    return *this;
}

JSON& JSON::add(const std::string& valor) {
    return add(valor.c_str());
}

JSON& JSON::add(JSON&& subObjeto) {
    if (!_root || !subObjeto._root) return *this;
    cJSON_AddItemToArray(_root, subObjeto.release());
    return *this;
}

// =========================================================================
// LEITURA E CONSULTA DE CAMPOS
// =========================================================================

bool JSON::has(const std::string& chave) const {
    if (!_root) return false;
    return cJSON_HasObjectItem(_root, chave.c_str());
}

int JSON::getInt(const std::string& chave, int padrao) const {
    if (!_root) return padrao;
    cJSON* item = cJSON_GetObjectItemCaseSensitive(_root, chave.c_str());
    if (cJSON_IsNumber(item)) {
        return item->valueint;
    }
    return padrao;
}

int64_t JSON::getInt64(const std::string& chave, int64_t padrao) const {
    if (!_root) return padrao;
    cJSON* item = cJSON_GetObjectItemCaseSensitive(_root, chave.c_str());
    if (cJSON_IsNumber(item)) {
        return static_cast<int64_t>(item->valuedouble);
    }
    return padrao;
}

double JSON::getDouble(const std::string& chave, double padrao) const {
    if (!_root) return padrao;
    cJSON* item = cJSON_GetObjectItemCaseSensitive(_root, chave.c_str());
    if (cJSON_IsNumber(item)) {
        return item->valuedouble;
    }
    return padrao;
}

bool JSON::getBool(const std::string& chave, bool padrao) const {
    if (!_root) return padrao;
    cJSON* item = cJSON_GetObjectItemCaseSensitive(_root, chave.c_str());
    if (cJSON_IsBool(item)) {
        return cJSON_IsTrue(item);
    }
    return padrao;
}

std::string JSON::getString(const std::string& chave, const std::string& padrao) const {
    if (!_root) return padrao;
    cJSON* item = cJSON_GetObjectItemCaseSensitive(_root, chave.c_str());
    if (cJSON_IsString(item) && (item->valuestring != nullptr)) {
        return std::string(item->valuestring);
    }
    return padrao;
}

JSON JSON::getObject(const std::string& chave) const {
    if (!_root) return JSON(nullptr, false);
    cJSON* item = cJSON_GetObjectItemCaseSensitive(_root, chave.c_str());
    return JSON(item, false); // Não é o dono, evita deletar sub-nó isolado
}

// =========================================================================
// OPERAÇÕES EM ARRAYS
// =========================================================================

size_t JSON::size() const {
    if (!_root) return 0;
    return static_cast<size_t>(cJSON_GetArraySize(_root));
}

JSON JSON::getAt(size_t index) const {
    if (!_root) return JSON(nullptr, false);
    cJSON* item = cJSON_GetArrayItem(_root, static_cast<int>(index));
    return JSON(item, false); // Não é o dono, é apenas uma referência
}

// =========================================================================
// SERIALIZAÇÃO
// =========================================================================

std::string JSON::toString(bool formatado) const {
    if (!_root) return "";

    char* render = formatado ? cJSON_Print(_root) : cJSON_PrintUnformatted(_root);
    if (!render) {
        return "";
    }

    std::string resultado(render);
    cJSON_free(render);
    return resultado;
}

// =========================================================================
// UTILITÁRIOS E ESTADO
// =========================================================================

bool JSON::isValid() const {
    return _root != nullptr;
}

bool JSON::isArray() const {
    return _root != nullptr && cJSON_IsArray(_root);
}

bool JSON::isObject() const {
    return _root != nullptr && cJSON_IsObject(_root);
}

void JSON::clear() {
    if (_isOwner && _root != nullptr) {
        cJSON_Delete(_root);
    }
    _root = cJSON_CreateObject();
    _isOwner = true;
}

cJSON* JSON::release() {
    cJSON* temp = _root;
    _root = nullptr;
    _isOwner = false;
    return temp;
}
