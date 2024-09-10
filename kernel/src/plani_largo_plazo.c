#include "./plani_largo_plazo.h"

void inicializar_plani_largo_plazo()
{
    pthread_create(planificador_largo_plazo, NULL, planificar_largo_plazo, NULL);
    pthread_detach(*planificador_largo_plazo);
}

void* planificar_largo_plazo()
{
    while(1)
    {
        sem_wait(&verificar_cola_new);
        if (!list_is_empty(cola_new)){
            PCB* un_proceso = obtener_proceso_en_new();
            inicializar_proceso(un_proceso);
        }
        else{
            pthread_mutex_lock(&mutex_log);
            log_info(logger,"Cola new vacia, no hay procesos para inicializar");
            pthread_mutex_unlock(&mutex_log);
        }
    }
}

PCB* obtener_proceso_en_new()
{
    PCB *un_proceso;

    pthread_mutex_lock(&mutex_procesos_en_new);
    un_proceso = list_get(cola_new, 0);
    pthread_mutex_unlock(&mutex_procesos_en_new);

    return un_proceso;
}


