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
            log_info(logger, "estoy dentro el semaforo de &verificar_cola_new)");
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

void mandar_hilo_a_exit(int pid, int tid) {
    TCB* hilo = buscar_hilo_por_pid_tid(pid, tid);
    // Dependiendo del estado del hilo, buscamos en la cola correspondiente
    switch (hilo->estado) {
        case EXEC:
            pthread_mutex_unlock(&mutex_hilos_sistema);
            list_remove_element(hilos_sistema,hilo);
            pthread_mutex_unlock(&mutex_hilos_sistema);

            hilo_en_exec = NULL;
            liberar_TCB(hilo);
            break;
        case READY:
            if (ALGORITMO_PLANIFICACION == COLAS_MULTINIVEL) {
                pthread_mutex_lock(&mutex_colas_multinivel);
                for (int i = 0; i < list_size(cola_ready_multinivel); i++) {
                    t_cola_multinivel* cola_actual = list_get(cola_ready_multinivel, i);

                    bool encontrado = list_remove_element(cola_actual->cola,hilo);
                    if (encontrado){
                        pthread_mutex_unlock(&mutex_hilos_sistema);
                        list_remove_element(hilos_sistema,hilo);
                        pthread_mutex_unlock(&mutex_hilos_sistema);

                        liberar_TCB(hilo);
                        break;
                    }
                }
                pthread_mutex_unlock(&mutex_colas_multinivel);
            } else {
                pthread_mutex_lock(&mutex_cola_ready);
                list_remove_element(cola_ready, hilo);
                pthread_mutex_unlock(&mutex_cola_ready);\

                pthread_mutex_unlock(&mutex_hilos_sistema);
                list_remove_element(hilos_sistema,hilo);
                pthread_mutex_unlock(&mutex_hilos_sistema);

                liberar_TCB(hilo);
            }
            break;

        case BLOCKED_IO:
            pthread_mutex_lock(&mutex_cola_io);
            list_remove_element(cola_io, hilo);
            pthread_mutex_unlock(&mutex_cola_io);
            
            pthread_mutex_unlock(&mutex_hilos_sistema);
            list_remove_element(hilos_sistema,hilo);
            pthread_mutex_unlock(&mutex_hilos_sistema);

            liberar_TCB(hilo);
            break;

        case BLOCKED_TJ:
            // Buscar en la lista de joins (hilos bloqueados esperando otro hilo)
            pthread_mutex_lock(&mutex_cola_join_wait);
            for (int i = 0; i < list_size(cola_joins); i++) {
                t_join_wait* join_wait = list_get(cola_joins, i);
                if (join_wait->pid == pid && join_wait->tid_esperador->TID == tid) {
                    
                    pthread_mutex_unlock(&mutex_hilos_sistema);
                    list_remove_element(hilos_sistema,hilo);
                    pthread_mutex_unlock(&mutex_hilos_sistema);
                    
                    list_remove(cola_joins,i);

                    liberar_TCB(hilo);
                    break;
                }
            }
            pthread_mutex_unlock(&mutex_cola_join_wait);
            break;

        case BLOCKED_MUTEX:
            pthread_mutex_lock(&mutex_procesos_sistema);
            PCB* proceso_asociado = obtener_proceso_por_pid(pid);
            // Recorrer todos los mutex y buscar en sus colas de bloqueados
            for (int j = 0; j < list_size(proceso_asociado->mutexs); j++) {
                t_mutex* mutex = list_get(proceso_asociado->mutexs, j);

                bool encontrado = list_remove_element(mutex->cola_bloqueados,hilo);
                if (encontrado){
                    pthread_mutex_unlock(&mutex_hilos_sistema);
                    list_remove_element(hilos_sistema,hilo);
                    pthread_mutex_unlock(&mutex_hilos_sistema);

                    liberar_TCB(hilo);
                    break;
                }
            }
            pthread_mutex_unlock(&mutex_procesos_sistema);
            break;
        default:
            break;
    }
}


