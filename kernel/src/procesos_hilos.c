#include "./procesos_hilos.h"

PCB* crear_proceso(char *archivo_pseudocodigo, int tamanio_proceso, int prioridad_hilo_0) {
    pid_global++;
    PCB* nuevo_proceso = malloc(sizeof(PCB));
    nuevo_proceso->PID= pid_global;  // Generar un identificador único para el proceso
    nuevo_proceso->TIDs = list_create();  
    nuevo_proceso->mutexs = list_create(); 
    nuevo_proceso->tamanio = tamanio_proceso;
    nuevo_proceso->archivo_pseudocodigo_principal = strdup(archivo_pseudocodigo);
    nuevo_proceso->prioridad_hilo_0 = prioridad_hilo_0; // Lista de mutex asociados al proceso

    pthread_mutex_lock(&mutex_log);
    log_info(logger,"Proceso %d creado correctamente",nuevo_proceso->PID);
    pthread_mutex_unlock(&mutex_log);
    
    list_add(cola_new,nuevo_proceso);
    sem_post(&verificar_cola_new);
    return nuevo_proceso;
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
        //crea el hilo principal
        crear_hilo(proceso, proceso->prioridad_hilo_0, proceso->archivo_pseudocodigo_principal);
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

    PCB* proceso = buscar_proceso_por_id(proceso_id);
    if (proceso!=NULL){
        if (respuesta == 1) {
                liberar_PCB(proceso);
                sem_post(&verificar_cola_new);
                
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
    int respuesta = informar_creacion_hilo_a_memoria(proceso->PID, list_size(proceso->TIDs), archivo_pseudocodigo);
    if (respuesta == 1){
        list_add(proceso->TIDs, list_size(proceso->TIDs));
        TCB *nuevo_tcb = malloc(sizeof(TCB));
        nuevo_tcb->PID = proceso->PID;
        nuevo_tcb->TID = list_size(proceso->TIDs);
        nuevo_tcb->prioridad = prioridad;
        nuevo_tcb->archivo_pseudocodigo = archivo_pseudocodigo;
        nuevo_tcb->estado = READY;
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

void finalizar_hilo(PCB* proceso,int hilo_id) {
    int respuesta = informar_finalizacion_hilo_a_memoria(proceso->PID,hilo_id);
    // Procesar la respuesta de la memoria
    TCB* hilo = buscar_hilo_por_id_y_proceso(proceso,hilo_id);
    if (respuesta == 1) {
        
        liberar_TCB(hilo);
        mover_hilos_bloqueados_por(hilo);

        pthread_mutex_lock(&mutex_log);
        log_info(logger, "Hilo %d finalizado correctamente del proceso %d finalizado correctamente", hilo_id, proceso->PID);
        pthread_mutex_unlock(&mutex_log);
    } else {
        pthread_mutex_lock(&mutex_log);
        log_error(logger, "No se pudo finalizar el hilo %d", hilo_id);
        pthread_mutex_unlock(&mutex_log);
        
    }
}

void liberar_PCB(PCB* proceso) {
    // falta implementar, aca buscaria todos los TCBs y los liberaria asociados y luego haria un free al pcb

    list_destroy(proceso->TIDs);
    list_destroy(proceso->mutexs);
    free(proceso->archivo_pseudocodigo_principal);
}

void liberar_TCB(TCB* hilo) {
    PCB* proceso_asociado = buscar_proceso_por_id(hilo->PID);
    if (proceso_asociado != NULL) {
        // Buscar el índice del TID en la lista de TIDs
        int index_to_remove = -1;
        for (int i = 0; i < list_size(proceso_asociado->TIDs); i++) {
            int* tid = (int*) list_get(proceso_asociado->TIDs, i);
            if (*tid == hilo->TID) {
                index_to_remove = i;
                break;
            }
        }
        // Si se encontró el TID, removerlo de la lista
        if (index_to_remove != -1) {
            int* tid_removido = list_remove(proceso_asociado->TIDs, index_to_remove);
            free(tid_removido);  // Liberar la memoria del TID removido
        } else {

            pthread_mutex_lock(&mutex_log);
            log_error(logger, "No se encontró el TID %d en el proceso %d", hilo->TID, hilo->PID);
            pthread_mutex_unlock(&mutex_log);
            
        }
    } else {
        pthread_mutex_lock(&mutex_log);
        log_error(logger, "No se encontró el proceso con PID %d", hilo->PID);
        pthread_mutex_unlock(&mutex_log);
    }
    
    transicionar_hilo_a_exit(hilo);
}

void transicionar_hilo_a_exit(TCB* hilo)
{
    free(hilo->archivo_pseudocodigo);
    free(hilo);
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

TCB* buscar_hilo_por_id_y_proceso(PCB* proceso, int hilo_id){
    //FALTA IMPLEMENTAR
}