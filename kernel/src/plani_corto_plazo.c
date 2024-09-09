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
    return NULL;
}

void* algoritmo_prioridades(void* args) {
    while(1) {
        sem_wait(&hay_hilos_en_ready);

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
        list_remove(cola_ready, index_con_prioridad);
        pthread_mutex_unlock(&mutex_cola_ready);

        pthread_mutex_lock(&mutex_socket_dispatch);
        enviar_hilo_a_cpu(hilo_con_prioridad);
        pthread_mutex_unlock(&mutex_socket_dispatch);

        pthread_mutex_lock(&mutex_log);
        log_warning(logger, "Se pasa a EXEC el hilo %d del proceso %d", hilo_con_prioridad->TID, hilo_con_prioridad->PID);
        pthread_mutex_unlock(&mutex_log);
    }
    return NULL;
}

void* algoritmo_colas_multinivel(void* args) {
    while(1) {
        sem_wait(&hay_hilos_en_ready);

        pthread_mutex_lock(&mutex_colas_multinivel);
        t_cola_multinivel* cola_mayor_prioridad = obtener_cola_con_mayor_prioridad();
        TCB* hilo_elegido = list_get(cola_mayor_prioridad->cola, 0);
        list_remove_element(cola_mayor_prioridad->cola, hilo_elegido);
        pthread_mutex_unlock(&mutex_colas_multinivel);

        time_t tiempo_inicio = time(NULL);

        pthread_mutex_lock(&mutex_socket_dispatch);
        enviar_hilo_a_cpu(hilo_elegido);
        pthread_mutex_unlock(&mutex_socket_dispatch);

        pthread_mutex_lock(&mutex_log);
        log_warning(logger, "Se pasa a EXEC el hilo %d del proceso %d", hilo_elegido->TID, hilo_elegido->PID);
        pthread_mutex_unlock(&mutex_log);

        while (hilo_elegido->estado == EXEC) {
            time_t tiempo_actual = time(NULL);
            double tiempo_transcurrido = difftime(tiempo_actual, tiempo_inicio) * 1000;
            if (tiempo_transcurrido >= QUANTUM) {
                enviar_interrupcion_a_cpu();
                break;
            }
            usleep(1000); // 1 ms
        }
        if (hilo_elegido->estado == EXEC) {
            pthread_mutex_lock(&mutex_colas_multinivel);
            agregar_a_ready(hilo_elegido);
            pthread_mutex_unlock(&mutex_colas_multinivel);
        }
    }
    return NULL;
}

void enviar_interrupcion_a_cpu()
{
    int interrupcion = 1;
    send(fd_cpu_interrupt, &interrupcion, sizeof(int), 0);  // Enviar la interrupción
    pthread_mutex_lock(&mutex_log);
    log_info(logger, "Se envió una interrupción a la CPU.");
    pthread_mutex_unlock(&mutex_log);
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
    agregar_a_paquete(paquete, &(hilo->TID), sizeof(int));
    agregar_a_paquete(paquete, &(hilo->PID), sizeof(int));
    enviar_peticion(paquete, fd_cpu_dispatch, ENVIAR_HILO);
    eliminar_paquete(paquete);

    // Esperar la respuesta de la CPU (TID devuelto y motivo)
    int motivo;
    recv(fd_cpu_dispatch, &motivo, sizeof(motivo_devolucion), 0);
    procesar_motivo_devolucion(hilo, motivo);
}

void procesar_motivo_devolucion(TCB* hilo, motivo_devolucion motivo) {
    switch(motivo) {
        case DESALOJO:
            pthread_mutex_lock(&mutex_log);
            log_info(logger, "Volvió el hilo %d del proceso %d por DESALOJO.", hilo->TID, hilo->PID);
            pthread_mutex_unlock(&mutex_log);

            pthread_mutex_lock(&mutex_hilo_exec);
            agregar_a_ready(hilo_en_exec);
            hilo_en_exec = NULL;
            pthread_mutex_unlock(&mutex_hilo_exec);

            liberar_TCB(hilo);
            break;
        case BLOQUEO_IO:
            pthread_mutex_lock(&mutex_log);
            log_info(logger, "Volvió hilo %d del proceso %d para BLOQUEAR.", hilo->TID, hilo->PID);
            pthread_mutex_unlock(&mutex_log);

            pthread_mutex_lock(&mutex_hilo_exec);
            pthread_mutex_lock(&mutex_cola_blocked);
            list_add(cola_blocked, hilo_en_exec);
            hilo_en_exec = NULL;

            //falta el gestor IO
            pthread_mutex_unlock(&mutex_cola_blocked);
            pthread_mutex_unlock(&mutex_hilo_exec);

            break;
        case HILO_TERMINADO:
            pthread_mutex_lock(&mutex_log);
            log_info(logger, "Volvió el hilo %d del proceso %d para finalizar.", hilo->TID, hilo->PID);
            pthread_mutex_unlock(&mutex_log);

            pthread_mutex_lock(&mutex_hilo_exec);
            hilo_en_exec = NULL;
            pthread_mutex_unlock(&mutex_hilo_exec);

            liberar_TCB(hilo);
            break;
    }
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

void *gestor_io(void) {
    while (1) {
        // Esperar a que haya hilos en la cola de BLOCKED (FIFO)
        sem_wait(&hay_hilos_en_blocked);

        // Esperar hasta que no haya otro hilo en I/O
        sem_wait(&sem_io_mutex);

        // Obtener el primer hilo en la cola de BLOCKED (FIFO)
        pthread_mutex_lock(&mutex_cola_blocked);
        TCB* hilo_bloqueado = list_get(cola_blocked, 0); // Obtener el primer hilo
        pthread_mutex_unlock(&mutex_cola_blocked);

        // Simular operación de I/O: obtener el tiempo a bloquear
        int tiempo_a_bloquear = hilo_bloqueado->tiempo_bloqueo_io * 1000; // Convertir a microsegundos

        pthread_mutex_lock(&mutex_log);
        printf("El hilo %d del proceso %d inicia su I/O de %d milisegundos\n", 
               hilo_bloqueado->TID, hilo_bloqueado->PID, hilo_bloqueado->tiempo_bloqueo_io);
        pthread_mutex_unlock(&mutex_log);

        // Simular el bloqueo (I/O) por el tiempo indicado
        usleep(tiempo_a_bloquear);

        pthread_mutex_lock(&mutex_log);
        printf("El hilo %d del proceso %d finaliza su I/O de %d milisegundos\n", 
               hilo_bloqueado->TID, hilo_bloqueado->PID, hilo_bloqueado->tiempo_bloqueo_io);
        pthread_mutex_unlock(&mutex_log);

        // Remover el hilo de la cola de BLOCKED
        pthread_mutex_lock(&mutex_cola_blocked);
        hilo_bloqueado = list_remove(cola_blocked, 0); // Remover el hilo que ya terminó su I/O
        pthread_mutex_unlock(&mutex_cola_blocked);

        // Mover el hilo a la cola de READY
        pthread_mutex_lock(&mutex_cola_ready);
        hilo_bloqueado->estado = READY;
        agregar_a_ready(hilo_bloqueado);
        pthread_mutex_unlock(&mutex_cola_ready);

        // Señalar que hay hilos en la cola de READY
        sem_post(&hay_hilos_en_ready);
        // Liberar el semáforo de I/O para permitir que el siguiente hilo maneje I/O
        sem_post(&sem_io_mutex);
    }
}
//manejar este problema: si hay un hilo haciendo i/o que todavia no termina y llegan 2 hilos nuevos a blocked, 
//ambos hilos habilitaran el semaforo hay_hilos_en_ready (en realidad solo el primero que llegue lo habilitara),
// pero cuando el segundo hilo que llegue tambien quiera habilitarlo ya va a estar habilitado entonces el while 
//solo se ejecutara 1 sola vez (la del primer hilo que llego)

t_cola_multinivel* obtener_o_crear_cola_con_prioridad(int prioridad_buscada) {
    for (int i = 0; i < list_size(cola_ready_multinivel); i++) {
        t_cola_multinivel* cola = list_get(cola_ready_multinivel, i);
        if (cola->prioridad == prioridad_buscada) {
            pthread_mutex_unlock(&mutex_colas_multinivel);
            return cola;
        }
    }
    return NULL;
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
    pthread_mutex_lock(&mutex_colas_multinivel); // Bloquear el mutex para proteger el acceso a la lista
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
    pthread_mutex_unlock(&mutex_colas_multinivel); // Desbloquear el mutex antes de devolver
    return cola_con_mayor_prioridad;  // Devolver la cola con mayor prioridad
}