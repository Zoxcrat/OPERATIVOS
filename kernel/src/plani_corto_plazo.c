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

void* algoritmo_fifo(void* args)
{
    while(1)
    {
        pthread_mutex_lock(&mutex_cola_ready);
        if (!list_is_empty(cola_ready)) {
            TCB* hilo_a_ejecutar = list_remove(cola_ready, 0);
            pthread_mutex_unlock(&mutex_cola_ready);
            // Enviar el hilo a la CPU para su ejecución
            enviar_hilo_a_cpu(hilo_a_ejecutar);
        } else {
            pthread_mutex_unlock(&mutex_cola_ready);
        }
        sleep(1);
    }
    return NULL;
}

void* algoritmo_prioridades(void* args)
{
    while(1)
    {
        pthread_mutex_lock(&mutex_cola_ready);

        if (!list_is_empty(cola_ready)) {
            TCB* hilo_con_prioridad = list_get(cola_ready, 0);
            int index_con_prioridad = 0;

            // Buscar el hilo con mayor prioridad, desempate por FIFO
            for (int i = 1; i < list_size(cola_ready); i++) {
                TCB* hilo_actual = list_get(cola_ready, i);
                if (hilo_actual->prioridad < hilo_con_prioridad->prioridad) {
                    hilo_con_prioridad = hilo_actual;
                    index_con_prioridad = i;
                }
            }

            // Remover el hilo de la cola
            list_remove(cola_ready, index_con_prioridad);

            pthread_mutex_unlock(&mutex_cola_ready);

            // Ejecutar el hilo con la mayor prioridad
            enviar_hilo_a_cpu(hilo_con_prioridad);
        } else {
            pthread_mutex_unlock(&mutex_cola_ready);
        }

        sleep(1);
    }
    return NULL;
}

void* algoritmo_colas_multinivel(void* args)
{
    while(1)
    {
        pthread_mutex_lock(&mutex_colas_multinivel);
        if (!list_is_empty(cola_ready_multinivel)) {
            t_cola_multinivel* cola_mayor_prioridad = obtener_cola_con_mayor_prioridad();
            TCB* hilo_elegido = list_get(cola_mayor_prioridad->cola,0);
            list_remove_element(cola_mayor_prioridad->cola,hilo_elegido);

            pthread_mutex_unlock(&mutex_cola_ready);
            // Enviar el hilo a la CPU para su ejecución
            enviar_hilo_a_cpu(hilo_elegido);
        } else {
            pthread_mutex_unlock(&mutex_cola_ready);
        }
    }
    return NULL;
}

void enviar_interrupcion_a_cpu()
{
    int interrupcion = 1;
    send(fd_cpu_interrupt, &interrupcion, sizeof(int), 0);  // Enviar la interrupción
    log_info(logger, "Se envió una interrupción a la CPU.");
}

void enviar_hilo_a_cpu(TCB* hilo)
{
    // Cambiar el estado del hilo a EXEC
    hilo->estado = EXEC;
    list_add(cola_exec,hilo);

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

void procesar_motivo_devolucion(TCB* hilo, motivo_devolucion motivo)
{
    switch (motivo)
    {
        case MOTIVO_FINALIZACION:
            // El hilo ha finalizado su ejecución
            log_info(logger, "Hilo TID %d (PID %d) ha finalizado.", hilo->TID, hilo->PID);
            hilo->estado = EXIT;
            free(hilo);  // Liberar los recursos asociados al hilo
            break;

        case MOTIVO_IO:
            // El hilo necesita realizar una operación de E/S
            log_info(logger, "Hilo TID %d (PID %d) en espera de I/O.", hilo->TID, hilo->PID);
            hilo->estado = BLOCKED;
            // Mover el hilo a la cola de I/O (no mostrado aquí)
            //mover_a_cola_io(hilo);
            break;

        case MOTIVO_DESALOJO:
            // El hilo fue desalojado por una interrupción
            log_info(logger, "Hilo TID %d (PID %d) fue desalojado.", hilo->TID, hilo->PID);
            hilo->estado = READY;
            // Replanificar el hilo, devolviéndolo a la cola READY
            pthread_mutex_lock(&mutex_cola_ready);
            list_add(cola_ready, hilo);
            pthread_mutex_unlock(&mutex_cola_ready);
            break;

        default:
            log_error(logger, "Motivo de devolución desconocido: %d", motivo);
            break;
    }
}

void agregar_a_ready(TCB* tcb){
    switch (ALGORITMO_PLANIFICACION)
    {
    case FIFO:
        list_add(cola_ready, tcb);
        break;
    case PRIORIDADES:
        list_add(cola_ready, tcb);
        break;
    case COLAS_MULTINIVEL:
        t_cola_multinivel* cola_multinivel= obtener_o_crear_cola_con_prioridad(tcb->prioridad);
        list_add(cola_multinivel->cola, tcb);
    }
}

t_cola_multinivel* obtener_o_crear_cola_con_prioridad(int prioridad_buscada) {
    // Recorrer la lista de colas
    for (int i = 0; i < list_size(cola_ready_multinivel); i++) {
        t_cola_multinivel* cola = list_get(cola_ready_multinivel, i);

        if (cola->prioridad == prioridad_buscada) {
            return cola; // Retornar la cola encontrada
        }
    }

    // No se encontró ninguna cola con la prioridad buscada, así que creo una nueva
    t_cola_multinivel* nueva_cola = malloc(sizeof(t_cola_multinivel));
    nueva_cola->prioridad = prioridad_buscada;
    nueva_cola->cola = list_create(); 
    
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