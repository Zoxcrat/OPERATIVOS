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
            PCB* un_proceso = list_get(cola_new, 0);
            inicializar_proceso(un_proceso);
        } 
        else{
            log_info(logger,"cola new vacia, no se inicializa nada.");
        }
    }
}