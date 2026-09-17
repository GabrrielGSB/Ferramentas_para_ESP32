#pragma once

#ifndef TASK_HPP
#define TASK_HPP

#include <string>
#include <functional>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "Tempo.hpp"

using namespace std;

enum class TaskCore {
    Core0 = 0,
    Core1 = 1,
    QualquerUm = tskNO_AFFINITY
};

class Task {
    public:
        // Alias para o tipo da função da task instanciada 
        using TaskFunction = std::function<void()>;

        Task(
            TaskFunction  func,
            const string& nome          = "Task", 
            uint32_t      tamanhoPilha  = 4096, 
            UBaseType_t   prioridade    = 1, 
            TaskCore      core          = TaskCore::QualquerUm
        );
      
        ~Task();

        // Impede cópias acidentais para preservar a integridade do TaskHandle_t
        Task(const Task&) = delete;
        Task& operator=(const Task&) = delete;

        // ==========================================
        // MÉTODOS DE CONTROLE DA TAREFA
            bool iniciar();
            void parar();
            void suspender();
            void retomar();
            static inline void ceder() { taskYIELD(); }
        // ==========================================

        // ==========================================
        // GETTERS E SETTERS DE ESTADO
            bool          estaExecutando() const { return m_executando; }
            TaskHandle_t  getHandle()      const { return m_handle; }
            const string& getNome()        const { return m_nome; }
            UBaseType_t   getPrioridade()  const { return m_prioridade; }
            TaskCore      getCore()        const { return m_core; }
            void          setPrioridade(UBaseType_t novaPrioridade);
        // ==========================================

    private:
        string       m_nome;
        uint32_t     m_tamanhoPilha;
        UBaseType_t  m_prioridade;
        TaskCore     m_core;
        TaskFunction m_func;
        TaskHandle_t m_handle;
        bool         m_executando;

    /**
     * @brief Padrão de trampolim estático para ligar a chamada estilo C do FreeRTOS à função/lambda.
     */
    static void taskTrampoline(void* arg);
};

#endif // TASK_HPP
