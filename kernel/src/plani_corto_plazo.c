#include "./plani_corto_plazo.h"

void inicializar_plani_corto_plazo()
{
    switch (ALGORITMO_PLANIFICACION)
    {
    case FIFO:
        lanzar_hilo_plani_corto_plazo_con(algoritmo_fifo);
        break;
    case PRIORIDADES:
        lanzar_hilo_plani_corto_plazo_con(algoritmo_prioridades);
        break;
    case COLAS_MULTINIVEL:
        lanzar_hilo_plani_corto_plazo_con(algoritmo_colas_multinivel);
        break;
    }
}

void lanzar_hilo_plani_corto_plazo_con(void* (*algoritmo_plani)(void*))
{
    pthread_create(planificador_corto_plazo, NULL, algoritmo_plani, NULL);
    pthread_detach(*planificador_corto_plazo);
}

void* algoritmo_fifo(void* args) {
    while(1) {
        sem_wait(&hay_hilos_en_ready);

        if(!list_is_empty(cola_ready) && hilo_en_exec == NULL){
            pthread_mutex_lock(&mutex_cola_ready);
            TCB* hilo_a_ejecutar = list_remove(cola_ready, 0);
            pthread_mutex_unlock(&mutex_cola_ready);

            pthread_mutex_lock(&mutex_socket_dispatch);
            enviar_hilo_a_cpu(hilo_a_ejecutar);
            pthread_mutex_unlock(&mutex_socket_dispatch);

            pthread_mutex_lock(&mutex_log);
            log_warning(logger, "Se pasa a EXEC el hilo %d del proceso %d", hilo_a_ejecutar->TID, hilo_a_ejecutar->PID);
            pthread_mutex_unlock(&mutex_log);
        }
    }
    return NULL;
}

void* algoritmo_prioridades(void* args) {
    while(1) {
        sem_wait(&hay_hilos_en_ready);

        if(!list_is_empty(cola_ready) && hilo_en_exec == NULL){
        pthread_mutex_lock(&mutex_cola_ready);
        TCB* hilo_con_prioridad = list_get(cola_ready, 0);
        int index_con_prioridad = 0;
        for (int i = 1; i < list_size(cola_ready); i++) {
            TCB* hilo_actual = list_get(cola_ready, i);
            if (hilo_actual->prioridad < hilo_con_prioridad->prioridad) {
                hilo_con_prioridad = hilo_actual;
                index_con_prioridad = i;
            }
        }
        hilo_con_prioridad = list_remove(cola_ready, index_con_prioridad);
        pthread_mutex_unlock(&mutex_cola_ready);

        pthread_mutex_lock(&mutex_socket_dispatch);
        enviar_hilo_a_cpu(hilo_con_prioridad);
        pthread_mutex_unlock(&mutex_socket_dispatch);

        pthread_mutex_lock(&mutex_log);
        log_warning(logger, "Se pasa a EXEC el hilo %d del proceso %d", hilo_con_prioridad->TID, hilo_con_prioridad->PID);
        pthread_mutex_unlock(&mutex_log);
        }
    }
    return NULL;
}

void* algoritmo_colas_multinivel(void* args) {
    while (1) {
        sem_wait(&hay_hilos_en_ready);  // Esperar hasta que haya hilos listos

        if (hilo_en_exec == NULL) {
            pthread_mutex_lock(&mutex_colas_multinivel);
            t_cola_multinivel* cola_mayor_prioridad = obtener_cola_con_mayor_prioridad();
            TCB* hilo_elegido = list_get(cola_mayor_prioridad->cola, 0);
            list_remove_element(cola_mayor_prioridad->cola, hilo_elegido);
            pthread_mutex_unlock(&mutex_colas_multinivel);

            struct timespec tiempo_inicio, tiempo_actual;
            clock_gettime(CLOCK_MONOTONIC, &tiempo_inicio);  // Obtener tiempo de inicio

            pthread_mutex_lock(&mutex_socket_dispatch);
            enviar_hilo_a_cpu(hilo_elegido);  // Enviar el hilo a la CPU para que sea ejecutado
            pthread_mutex_unlock(&mutex_socket_dispatch);

            pthread_mutex_lock(&mutex_log);
            log_warning(logger, "Se pasa a EXEC el hilo %d del proceso %d", hilo_elegido->TID, hilo_elegido->PID);
            pthread_mutex_unlock(&mutex_log);

            while (hilo_elegido->estado == EXEC) {
                // Control del quantum
                clock_gettime(CLOCK_MONOTONIC, &tiempo_actual);  // Obtener el tiempo actual
                double tiempo_transcurrido = (tiempo_actual.tv_sec - tiempo_inicio.tv_sec) * 1000 +
                                             (tiempo_actual.tv_nsec - tiempo_inicio.tv_nsec) / 1000000.0;

                if (tiempo_transcurrido >= QUANTUM) {
                    // Se cumplió el quantum, se debe desalojar el hilo
                    interrupcion = 1;
                    sem_post(&mandar_interrupcion);  // Mandar interrupción por fin de quantum

                    agregar_a_ready(hilo_elegido);
                    
                    pthread_mutex_lock(&mutex_hilo_exec);
                    hilo_en_exec = NULL;  // Limpiar el hilo en ejecución
                    pthread_mutex_unlock(&mutex_hilo_exec);

                    sem_post(&hay_hilos_en_ready);

                    pthread_mutex_lock(&mutex_log);
                    log_info(logger, "El hilo %d del proceso %d fue desalojado por fin de quantum.", hilo_elegido->TID, hilo_elegido->PID);
                    pthread_mutex_unlock(&mutex_log);
                    break;
                }

                usleep(1000);  // Esperar 1 ms antes de la próxima verificación
            }
        }
    }
    return NULL;
}

void enviar_hilo_a_cpu(TCB* hilo)
{
    // Cambiar el estado del hilo a EXEC
    hilo->estado = EXEC;

    pthread_mutex_lock(&mutex_hilo_exec);
    hilo_en_exec = hilo;
    pthread_mutex_lock(&mutex_hilo_exec);

    // Crear un paquete para enviar el TID y PID al módulo de CPU (dispatch)
    t_paquete* paquete = crear_paquete();
    agregar_a_paquete(paquete, &(hilo->PID), sizeof(int));
    agregar_a_paquete(paquete, &(hilo->TID), sizeof(int));
    enviar_peticion(paquete, fd_cpu_dispatch, EJECUTAR_HILO);
    eliminar_paquete(paquete);
}

void agregar_a_ready(TCB* tcb){
    tcb->estado = READY;
    switch (ALGORITMO_PLANIFICACION) {
        case FIFO:
            pthread_mutex_lock(&mutex_cola_ready);
            list_add(cola_ready, tcb);
            pthread_mutex_unlock(&mutex_cola_ready);
            break;
        case PRIORIDADES:
            pthread_mutex_lock(&mutex_cola_ready);
            list_add(cola_ready, tcb);
            pthread_mutex_unlock(&mutex_cola_ready);
            break;
        case COLAS_MULTINIVEL:
            pthread_mutex_lock(&mutex_colas_multinivel);
            t_cola_multinivel* cola_multinivel = obtener_o_crear_cola_con_prioridad(tcb->prioridad);
            list_add(cola_multinivel->cola, tcb);
            pthread_mutex_unlock(&mutex_colas_multinivel);
            break;
    }
    sem_post(&hay_hilos_en_ready);
}

void manejar_thread_join(int tid_a_unir) {
    TCB* hilo_esperador = hilo_en_exec;
    hilo_esperador->estado = BLOCKED_TJ;

    // Añadir relación pid <-> tid_esperador <-> tid_esperado a la lista de joins
    t_join_wait* nueva_relacion = malloc(sizeof(t_join_wait));
    nueva_relacion->pid = hilo_esperador->PID;
    nueva_relacion->tid_esperador = hilo_esperador;
    nueva_relacion->tid_esperado = tid_a_unir;

    pthread_mutex_lock(&mutex_cola_join_wait);
    list_add(cola_joins, nueva_relacion);
    pthread_mutex_lock(&mutex_cola_join_wait);

    pthread_mutex_lock(&mutex_log);
    log_info(logger_obligatorio, "## (%d:%d) - Bloqueado por: PTHREAD_JOIN", hilo_en_exec->PID,hilo_en_exec->TID);
    pthread_mutex_unlock(&mutex_log);

    hilo_desalojado = true;

    pthread_mutex_lock(&mutex_hilo_exec);
    hilo_en_exec = NULL;
    pthread_mutex_unlock(&mutex_hilo_exec);
}
/// FUNCIONES PARA ALROGIRMTO COLAS MULTINIVEL

t_cola_multinivel* obtener_o_crear_cola_con_prioridad(int prioridad_buscada) {
    // Buscar una cola existente con la prioridad buscada
    for (int i = 0; i < list_size(cola_ready_multinivel); i++) {
        t_cola_multinivel* cola = list_get(cola_ready_multinivel, i);
        if (cola->prioridad == prioridad_buscada) {
            return cola;
        }
    }

    // Si no se encontró ninguna, crear una nueva cola
    t_cola_multinivel* nueva_cola = malloc(sizeof(t_cola_multinivel));
    nueva_cola->cola = list_create();  // Crear la lista para almacenar hilos
    nueva_cola->prioridad = prioridad_buscada;

    // Agregar la nueva cola a la lista de colas multinivel
    list_add(cola_ready_multinivel, nueva_cola);

    return nueva_cola;
}

void verificar_eliminacion_cola_multinivel(int prioridad_buscada) {
    // Recorrer la lista de colas
    for (int i = 0; i < list_size(cola_ready_multinivel); i++) {
        t_cola_multinivel* cola = list_get(cola_ready_multinivel, i);
        if (cola->prioridad == prioridad_buscada) {
            if (list_is_empty(cola->cola)){
                // Eliminar la cola de la lista
                list_remove(cola_ready_multinivel, i);
                // Liberar la memoria de la cola
                list_destroy(cola->cola); 
                free(cola); // Liberar la memoria de la cola
            }
        }
    }
}

t_cola_multinivel* obtener_cola_con_mayor_prioridad() {
    // Si la lista de colas multinivel está vacía, devolver NULL
    if (list_is_empty(cola_ready_multinivel)) {
        pthread_mutex_unlock(&mutex_colas_multinivel); // Desbloquear el mutex antes de devolver
        return NULL;
    }
    // Asumir que la primera cola tiene la mayor prioridad inicialmente
    t_cola_multinivel* cola_con_mayor_prioridad = list_get(cola_ready_multinivel, 0);
    // Recorrer el resto de las colas y buscar la que tenga la mayor prioridad (valor más bajo)
    for (int i = 1; i < list_size(cola_ready_multinivel); i++) {
        t_cola_multinivel* cola_actual = list_get(cola_ready_multinivel, i);

        // Si la prioridad de la cola actual es mayor (valor numérico más bajo), actualizamos
        if (cola_actual->prioridad < cola_con_mayor_prioridad->prioridad) {
            cola_con_mayor_prioridad = cola_actual;
        }
    }
    return cola_con_mayor_prioridad;  // Devolver la cola con mayor prioridad
}