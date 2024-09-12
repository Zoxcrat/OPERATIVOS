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
    printf(sem_getvalue(&verificar_cola_new,NULL));
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
    PCB* proceso = buscar_proceso_por_id(proceso_id);
    pthread_mutex_unlock(&mutex_procesos_sistema);
    if (proceso!=NULL){
        if (respuesta == 1) {
            liberar_PCB(proceso);

            sem_post(&verificar_cola_new);

            pthread_mutex_lock(&mutex_procesos_sistema);
            list_remove_element(procesos_sistema, proceso);
            pthread_mutex_unlock(&mutex_procesos_sistema);
            
            pthread_mutex_lock(&mutex_log);
            log_info(logger, "Proceso %d finalizado correctamente", proceso_id);
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
    TCB* hilo = buscar_hilo_por_id_y_proceso(pid,hilo_id);
    if (respuesta == 1) {
        
        liberar_TCB(hilo);
        //DESBLOQUEAR HILOS BLOQUEADOS POR THREAD_JOIN / por mutex tomados por el hilo finalizado 
        mover_hilos_bloqueados_por(hilo);

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
    // Recorrer la lista de TCBs asociados al proceso y finalizar cada hilo
    for (int i = 0; i < list_size(proceso->TIDs); i++) {
        int tid = list_get(proceso->TIDs, i);
        TCB* hilo = buscar_hilo_por_id_y_proceso(proceso->PID, tid);
        
        if (hilo != NULL) {
            // FALTARIA REMOVER EL HILO DE LA LISTA EN LA QUE SE ENCUENTRA
            liberar_TCB(hilo);

            pthread_mutex_lock(&mutex_log);
            log_info(logger, "Hilo %d del proceso %d movido a la cola de EXIT", hilo->TID, hilo->PID);
            pthread_mutex_unlock(&mutex_log);
        } else {
            pthread_mutex_lock(&mutex_log);
            log_error(logger, "No se encontró el hilo %d del proceso %d", tid, proceso->PID);
            pthread_mutex_unlock(&mutex_log);
        }
    }

    list_destroy(proceso->TIDs);
    list_destroy(proceso->mutexs);
    free(proceso->archivo_pseudocodigo_principal);
    free(proceso);

    pthread_mutex_lock(&mutex_log);
    log_info(logger, "Proceso %d y sus hilos han sido liberados correctamente", proceso->PID);
    pthread_mutex_unlock(&mutex_log);
}

void liberar_TCB(TCB* hilo) {
    free(hilo->archivo_pseudocodigo);
    free(hilo);
}

void transicionar_hilo_actual_a_exit()
{
    free(hilo_en_exec->archivo_pseudocodigo);
    free(hilo_en_exec);
    hilo_en_exec = NULL;
}

// FALTAN IMPLEMENTAR

void mover_hilos_bloqueados_por(TCB* hilo){
    //falta implementar
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
PCB* buscar_proceso_por_id(int proceso_id) {
    for (int i = 0; i < list_size(procesos_sistema); i++) {
        PCB* pcb = (PCB*) list_get(procesos_sistema, i);
        if (pcb->PID == proceso_id) {
            return pcb; 
        }
    }
    return NULL; 
}

TCB* buscar_hilo_por_id_y_proceso(int proceso_id, int hilo_id) {
    TCB* hilo_encontrado = NULL;
    // Verificar en la cola de ready (si no es multinivel)
    if (ALGORITMO_PLANIFICACION != COLAS_MULTINIVEL) {
        pthread_mutex_lock(&mutex_cola_ready);
        for (int i = 0; i < list_size(cola_ready); i++) {
            TCB* hilo = list_get(cola_ready, i);
            if (hilo->PID == proceso_id && hilo->TID == hilo_id) {
                hilo_encontrado = hilo;
                break;
            }
        }
        pthread_mutex_unlock(&mutex_cola_ready);
    } else {
        // Verificar en la cola multinivel
        pthread_mutex_lock(&mutex_colas_multinivel);
        for (int i = 0; i < list_size(cola_ready_multinivel); i++) {
            t_cola_multinivel* cola_multinivel = list_get(cola_ready_multinivel, i);
            for (int j = 0; j < list_size(cola_multinivel->cola); j++) {
                TCB* hilo = list_get(cola_multinivel->cola, j);
                if (hilo->PID == proceso_id && hilo->TID == hilo_id) {
                    hilo_encontrado = hilo;
                    break;
                }
            }
            if (hilo_encontrado != NULL) break;
        }
        pthread_mutex_unlock(&mutex_colas_multinivel);
    }
    // Si no lo encontró en ready, buscar en blocked
    if (hilo_encontrado == NULL) {
        pthread_mutex_lock(&mutex_cola_blocked);
        for (int i = 0; i < list_size(cola_blocked); i++) {
            TCB* hilo = list_get(cola_blocked, i);
            if (hilo->PID == proceso_id && hilo->TID == hilo_id) {
                hilo_encontrado = hilo;
                break;
            }
        }
        pthread_mutex_unlock(&mutex_cola_blocked);
    }

    return hilo_encontrado;
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