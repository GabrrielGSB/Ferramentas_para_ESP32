#include "Task.hpp"

Task::Task(TaskFunction func, const string& nome, uint32_t tamanhoPilha, UBaseType_t prioridade, TaskCore core)
    : m_nome        (nome),
      m_tamanhoPilha(tamanhoPilha),
      m_prioridade  (prioridade),
      m_core        (core),
      m_func        (std::move(func)),
      m_handle      (nullptr),
      m_executando  (false) {}

Task::~Task() {
    parar();
}

bool Task::iniciar() {
    // Não inicia sem uma função válida definida
    if (!m_func) {
        return false;
    }

    // Evita criar múltiplas instâncias da mesma tarefa se já estiver rodando
    if (m_executando && m_handle != nullptr) {
        return true;
    }

    BaseType_t resultado = xTaskCreatePinnedToCore(
        taskTrampoline,
        m_nome.c_str(),
        m_tamanhoPilha,
        this,
        m_prioridade,
        &m_handle,
        static_cast<BaseType_t>(m_core)
    );

    if (resultado == pdPASS) {
        m_executando = true;
        return true;
    }

    m_handle     = nullptr;
    m_executando = false;
    return false;
}

void Task::parar() {
    if (m_handle != nullptr) {
        TaskHandle_t tempHandle = m_handle;
        m_handle = nullptr;
        m_executando = false;
        vTaskDelete(tempHandle);
    }
}

void Task::suspender() {
    if (m_handle != nullptr && m_executando) {
        vTaskSuspend(m_handle);
    }
}

void Task::retomar() {
    if (m_handle != nullptr && m_executando) {
        vTaskResume(m_handle);
    }
}

void Task::setPrioridade(UBaseType_t novaPrioridade) {
    m_prioridade = novaPrioridade;
    if (m_handle != nullptr) {
        vTaskPrioritySet(m_handle, novaPrioridade);
    }
}

void Task::taskTrampoline(void* arg) {
    Task* taskInstancia = static_cast<Task*>(arg);

    if (taskInstancia != nullptr && taskInstancia->m_func) {
        // Executa a função ou lambda
        taskInstancia->m_func();

        // Se a função/lambda terminar sem loop infinito,
        // limpa o estado e desaloca a tarefa com segurança no FreeRTOS
        taskInstancia->m_executando = false;
        TaskHandle_t tempHandle = taskInstancia->m_handle;
        taskInstancia->m_handle = nullptr;
        vTaskDelete(tempHandle);
    } else {
        vTaskDelete(NULL);
    }
}
