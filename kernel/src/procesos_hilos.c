#include "./procesos_hilos.h"

void crear_proceso(char *archivo_pseudocodigo, int tamanio_proceso, int prioridad_hilo_0) {
    pid_global++;
    PCB* nuevo_proceso = malloc(sizeof(PCB));
    nuevo_proceso->PID= pid_global;  // Generar un identificador único para el proceso
    nuevo_proceso->TIDs = list_create();  
    nuevo_proceso->mutexs = list_create(); 
    nuevo_proceso->tamanio = tamanio_proceso;
    nuevo_proceso->archivo_pseudocodigo_principal = archivo_pseudocodigo;
    nuevo_proceso->prioridad_hilo_0 = prioridad_hilo_0; // lo pongo en el pcb por si todavia no se puede inicializar el proceso en memoria

    pthread_mutex_lock(&mutex_log);
    log_info(logger,"Proceso %d creado correctamente",nuevo_proceso->PID);
    pthread_mutex_unlock(&mutex_log);
    
    pthread_mutex_lock(&mutex_procesos_en_new);
    list_add(cola_new,nuevo_proceso);
    pthread_mutex_unlock(&mutex_procesos_en_new);
    
    sem_post(&verificar_cola_new);
    printf(sem_getvalue(&verificar_cola_new,NULL)); // no se por que ayuda poner esto
}

void inicializar_proceso(PCB* proceso){
    int respuesta = informar_inicializacion_proceso_a_memoria(proceso->PID, proceso->tamanio);
    if (respuesta == 1){
        pthread_mutex_lock(&mutex_log);
        log_info(logger, "Proceso %d inicializado correctamente",proceso->PID);
        pthread_mutex_unlock(&mutex_log);
        // saca el proceso de la cola new
        pthread_mutex_lock(&mutex_procesos_en_new);
        list_remove_element(cola_new, proceso);
        pthread_mutex_unlock(&mutex_procesos_en_new);

        pthread_mutex_lock(&mutex_procesos_en_new);
        list_add(procesos_sistema, proceso);
        pthread_mutex_unlock(&mutex_procesos_en_new);
        //crea el hilo principal
        crear_hilo(proceso, proceso->prioridad_hilo_0, proceso->archivo_pseudocodigo_principal);

        free(proceso->archivo_pseudocodigo_principal);
    }
    else{
        pthread_mutex_lock(&mutex_log);
        log_info(logger, "Proceso %d no creado, Memoria llena",proceso->PID);
        pthread_mutex_unlock(&mutex_log);
    }
}

void finalizar_proceso(int proceso_id){
    // Crear conexión efimera a la memoria 
    int respuesta = informar_finalizacion_proceso_a_memoria(proceso_id);

    pthread_mutex_lock(&mutex_procesos_sistema);
    PCB* proceso = obtener_proceso_por_pid(proceso_id);
    pthread_mutex_unlock(&mutex_procesos_sistema);
    if (proceso!=NULL){
        if (respuesta == 1) {
            eliminar_hilos_asociados(proceso);
            liberar_PCB(proceso);

            sem_post(&verificar_cola_new);
            
            pthread_mutex_lock(&mutex_log);
            log_info(logger, "Proceso %d y sus hilos han sido liberados correctamente", proceso->PID);
            pthread_mutex_unlock(&mutex_log);
        }else {
            pthread_mutex_lock(&mutex_log);
            log_error(logger, "No se pudo finalizar el proceso %d.", proceso_id);
            pthread_mutex_unlock(&mutex_log);
        }
    }
    else{
        pthread_mutex_lock(&mutex_log);
        log_error(logger, "no se pudo encontrar el proceso %d", proceso_id);
        pthread_mutex_unlock(&mutex_log);
    }
}

void crear_hilo(PCB* proceso, int prioridad, char* archivo_pseudocodigo) {
    // Informar a Memoria
    int id_proximo_hilo = obtener_numero_proximo_hilo(proceso);
    int respuesta = informar_creacion_hilo_a_memoria(proceso->PID, id_proximo_hilo, archivo_pseudocodigo);
    if (respuesta == 1){
        list_add(proceso->TIDs, id_proximo_hilo);
        TCB *nuevo_tcb = malloc(sizeof(TCB));
        nuevo_tcb->PID = proceso->PID;
        nuevo_tcb->TID = id_proximo_hilo;
        nuevo_tcb->prioridad = prioridad;
        nuevo_tcb->archivo_pseudocodigo = archivo_pseudocodigo;
        // Enviar hilo a cola READY dependiendo del algoritmo de planificacion
        agregar_a_ready(nuevo_tcb);

        pthread_mutex_lock(&mutex_log);
        log_info(logger, "Hilo %d para el proceso %d creado correctamente",nuevo_tcb->TID, proceso->PID);
        pthread_mutex_unlock(&mutex_log);
    }
    else{
        pthread_mutex_lock(&mutex_log);
        log_error(logger, "Hilo %d para el proceso %d no iniciado",list_size(proceso->TIDs), proceso->PID);
        pthread_mutex_unlock(&mutex_log);
    }
}

void finalizar_hilo(int pid,int hilo_id) {
    int respuesta = informar_finalizacion_hilo_a_memoria(pid,hilo_id);
    // Procesar la respuesta de la memoria
    TCB* hilo = buscar_hilo_por_pid_tid(pid,hilo_id);
    if (respuesta == 1) {
        liberar_TCB(hilo);
        //DESBLOQUEAR HILOS BLOQUEADOS POR THREAD_JOIN / por mutex tomados por el hilo finalizado 
        mover_hilos_bloqueados_por_thread_join(hilo);

        pthread_mutex_lock(&mutex_log);
        log_info(logger, "Hilo %d finalizado correctamente del proceso %d finalizado correctamente", hilo_id, pid);
        pthread_mutex_unlock(&mutex_log);
    } else {
        pthread_mutex_lock(&mutex_log);
        log_error(logger, "No se pudo finalizar el hilo %d", hilo_id);
        pthread_mutex_unlock(&mutex_log);
    }
}

void liberar_PCB(PCB* proceso) {
    list_destroy(proceso->TIDs);
    list_destroy(proceso->mutexs);
    free(proceso->archivo_pseudocodigo_principal);

    pthread_mutex_lock(&mutex_procesos_sistema);
    list_remove_element(procesos_sistema, proceso);
    pthread_mutex_unlock(&mutex_procesos_sistema);

    free(proceso);
}

void liberar_TCB(TCB* hilo) {
    switch(hilo->estado) {
        case READY:
            // Si está en cola_ready o cola_ready_multinivel
            if (ALGORITMO_PLANIFICACION == COLAS_MULTINIVEL) {
                pthread_mutex_lock(&mutex_colas_multinivel);
                t_cola_multinivel* cola_encontrada = NULL;
                for (int i = 0; i < list_size(cola_ready_multinivel); i++) {
                    t_cola_multinivel* cola_actual = list_get(cola_ready_multinivel, i);
                    if (cola_actual->prioridad == hilo->prioridad) {
                        cola_encontrada = cola_actual;
                        break;
                    }
                }
                // Si encontramos la cola con la prioridad correcta, removemos el hilo de esa cola
                if (cola_encontrada != NULL) {
                    list_remove_element(cola_encontrada->cola, hilo);
                }
                pthread_mutex_unlock(&mutex_colas_multinivel);
            } else {
                pthread_mutex_lock(&mutex_cola_ready);
                list_remove_element(cola_ready,hilo);
                pthread_mutex_unlock(&mutex_cola_ready);
            }
            break;
        case BLOCKED:
            pthread_mutex_lock(&mutex_cola_blocked);
            list_remove_element(cola_blocked,hilo);
            pthread_mutex_unlock(&mutex_cola_blocked);

            pthread_mutex_lock(&mutex_cola_io);
            list_remove_element(cola_io,hilo);
            pthread_mutex_unlock(&mutex_cola_io);
            break;
    }
    free(hilo->archivo_pseudocodigo);
    free(hilo);
}

void mover_hilos_bloqueados_por_thread_join(TCB* hilo){
    for (int i = 0; i < list_size(lista_joins); i++) {
        t_join_wait* relacion = list_get(lista_joins, i);
        if (relacion->pid == hilo->PID && relacion->tid_esperado == hilo->TID) {
            // Encontramos un hilo que espera por este hilo
            TCB* hilo_esperador = buscar_hilo_por_pid_tid(relacion->pid, relacion->tid_esperador);

            if (hilo_esperador != NULL) {
                hilo_esperador->estado = READY;
                agregar_a_ready(hilo_esperador);
            }

            // Eliminamos la relación ya que el hilo esperado ha terminado
            list_remove_and_destroy_element(lista_joins, i, free);
            i--;
        }
    }
}

// PEDIDOS A MEMORIA

int informar_inicializacion_proceso_a_memoria(int pid, int tamanio){
    // Crear conexión efímera a la memoria
    conectar_memoria();
	t_paquete* paquete = crear_paquete();
	agregar_a_paquete(paquete, &pid, sizeof(int));
	agregar_a_paquete(paquete, &tamanio, sizeof(int));
    enviar_peticion(paquete,fd_memoria,INICIALIZAR_PROCESO);
	eliminar_paquete(paquete);
    int respuesta; 
    recv(fd_memoria, &respuesta, sizeof(int), 0);
    close(fd_memoria);  // Cerrar la conexión con memoria
    return respuesta;
}

int informar_finalizacion_proceso_a_memoria(int pid){
    // Crear conexión efímera a la memoria
    conectar_memoria();
	t_paquete* paquete = crear_paquete();
	agregar_a_paquete(paquete, &pid, sizeof(int));
    enviar_peticion(paquete,fd_memoria,FINALIZACION_PROCESO);
	eliminar_paquete(paquete);
    int respuesta; 
    recv(fd_memoria, &respuesta, sizeof(int), 0);
    close(fd_memoria);  // Cerrar la conexión con memoria
    return respuesta;
}

int informar_creacion_hilo_a_memoria(int pid, int tid, char* archivo_pseudocodigo){
    // Crear conexión efímera a la memoria
    conectar_memoria();
	t_paquete* paquete = crear_paquete();
	agregar_a_paquete(paquete, &pid, sizeof(int));
	agregar_a_paquete(paquete, &tid, sizeof(int));
	agregar_a_paquete(paquete, archivo_pseudocodigo, strlen(archivo_pseudocodigo) + 1);
    enviar_peticion(paquete,fd_memoria,CREACION_HILO);
	eliminar_paquete(paquete);
    int respuesta;
    recv(fd_memoria, &respuesta, sizeof(int), 0);
    close(fd_memoria);  // Cerrar la conexión con memoria
    return respuesta;
}

int informar_finalizacion_hilo_a_memoria(int pid, int tid){
    // Crear conexión efímera a la memoria
    conectar_memoria();
	t_paquete* paquete = crear_paquete();
	agregar_a_paquete(paquete, &pid, sizeof(int));
	agregar_a_paquete(paquete, &tid, sizeof(int));
    enviar_peticion(paquete,fd_memoria,FINALIZACION_HILO);
	eliminar_paquete(paquete);
    int respuesta; 
    recv(fd_memoria, &respuesta, sizeof(int), 0);
    close(fd_memoria);  // Cerrar la conexión con memoria
    return respuesta;
}

/// FUNCIONES AUXILIARES ///
bool buscar_por_pid(void* proceso) {
    PCB* un_proceso = (PCB*) proceso;
    return un_proceso->PID == pid_a_buscar;
}

PCB* obtener_proceso_por_pid(int pid) {
    pid_a_buscar = pid;
    return (PCB*) list_find(procesos_sistema, buscar_por_pid);
}

bool es_hilo_del_proceso(void* hilo, void* pid_proceso) {
    TCB* un_hilo = (TCB*) hilo;
    int pid = *(int*) pid_proceso;
    return un_hilo->PID == pid;
}

void eliminar_hilos_asociados(PCB* proceso) {
    // Iteramos sobre la lista de TIDs de los hilos asociados al proceso
    for (int i = 0; i < list_size(proceso->TIDs); i++) {
        int tid_a_buscar = list_get(proceso->TIDs, i);
        int pid_a_buscar = proceso->PID; 
        TCB* hilo;
        // Buscar el hilo en cola_ready_multinivel
        if (ALGORITMO_PLANIFICACION == COLAS_MULTINIVEL) {
            pthread_mutex_lock(&mutex_colas_multinivel);
            for (int j = 0; j < list_size(cola_ready_multinivel); j++) {
                t_cola_multinivel* cola_actual = list_get(cola_ready_multinivel, j);
                hilo = buscar_hilo_en_cola_por_pid_tid(cola_actual->cola, pid_a_buscar, tid_a_buscar);
                if (hilo != NULL) {
                    liberar_TCB(hilo);
                    break;
                }
            }
            pthread_mutex_unlock(&mutex_colas_multinivel);
            if (hilo != NULL) continue;
        }else{
            pthread_mutex_lock(&mutex_cola_ready);
            TCB* hilo = buscar_hilo_en_cola_por_pid_tid(cola_ready, pid_a_buscar, tid_a_buscar);
            if (hilo != NULL) {
                liberar_TCB(hilo);
                continue;
            }
            pthread_mutex_unlock(&mutex_cola_ready);
        }

        // Buscar el hilo en cola_blocked
        pthread_mutex_lock(&mutex_cola_blocked);
        hilo = buscar_hilo_en_cola_por_pid_tid(cola_blocked, pid_a_buscar, tid_a_buscar);
        if (hilo != NULL) {
            liberar_TCB(hilo);
            continue;
        }
        pthread_mutex_unlock(&mutex_cola_blocked);

        // Buscar el hilo en cola_io
        pthread_mutex_lock(&mutex_cola_io);
        hilo = buscar_hilo_en_cola_por_pid_tid(cola_io, pid_a_buscar, tid_a_buscar);
        if (hilo != NULL) {
            liberar_TCB(hilo);
            continue;
        }
        pthread_mutex_unlock(&mutex_cola_io);
    }
}
//MODIFICAR EN BASE A ESTADO
TCB* buscar_hilo_por_pid_tid(int pid_a_buscar, int tid_a_buscar) {
    TCB* hilo = NULL;
    // Dependiendo del estado del hilo, buscamos en la cola correspondiente
    switch (hilo->estado) {
        case READY:
            if (ALGORITMO_PLANIFICACION == COLAS_MULTINIVEL){
                pthread_mutex_lock(&mutex_colas_multinivel);
                for (int i = 0; i < list_size(cola_ready_multinivel); i++) {
                    t_cola_multinivel* cola_actual = list_get(cola_ready_multinivel, i);
                    hilo = buscar_en_cola(cola_actual->cola, pid_a_buscar, tid_a_buscar);
                    if (hilo != NULL) {
                        pthread_mutex_unlock(&mutex_colas_multinivel);
                        return hilo;
                    }
                }
                pthread_mutex_unlock(&mutex_colas_multinivel);
                break;
            }else{
                pthread_mutex_lock(&mutex_cola_ready);
                hilo = buscar_en_cola(cola_ready, pid_a_buscar, tid_a_buscar);
                pthread_mutex_unlock(&mutex_cola_ready);
                return hilo;
            }
        case BLOCKED:
            // Primero buscar en cola_blocked
            pthread_mutex_lock(&mutex_cola_blocked);
            hilo = buscar_en_cola(cola_blocked, pid_a_buscar, tid_a_buscar);
            pthread_mutex_unlock(&mutex_cola_blocked);
            if (hilo != NULL) {
                return hilo;
            }
        default:
            return NULL; // Si el estado no es reconocido o no se encuentra el hilo
    }
    return NULL; // Si no se encuentra el hilo en ninguna cola
}

TCB* buscar_en_cola(t_list* cola, int pid_a_buscar, int tid_a_buscar) {
    for (int i = 0; i < list_size(cola); i++) {
        TCB* hilo = list_get(cola, i);
        if (hilo->PID == pid_a_buscar && hilo->TID == tid_a_buscar) {
            return hilo;
        }
    }
    return NULL;
}

TCB* buscar_hilo_en_cola_por_pid_tid(t_list* cola, int pid_a_buscar, int tid_a_buscar) {
    for (int i = 0; i < list_size(cola); i++) {
        TCB* hilo = list_get(cola, i);
        if (hilo->PID == pid_a_buscar && hilo->TID == tid_a_buscar) {
            return hilo;
        }
    }
    return NULL;  // Si no se encontró el hilo
}

int obtener_numero_proximo_hilo(PCB* proceso) {
    int proximo_hilo = -1;

    for (int i = 0; i < list_size(proceso->TIDs); i++) {
        int tid_actual = list_get(proceso->TIDs, i);
        if (tid_actual > proximo_hilo) {
            proximo_hilo = tid_actual;
        }
    }
    return proximo_hilo + 1;
}