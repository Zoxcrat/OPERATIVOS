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
    log_info(logger, "Proceso %d creado correctamente",nuevo_proceso->PID);

    list_add(cola_new,nuevo_proceso);
    sem_post(&verificar_cola_new);
    return nuevo_proceso;
}

void inicializar_proceso(PCB* proceso){
    int respuesta = informar_inicializacion_proceso_a_memoria(proceso->PID, proceso->tamanio);
    if (respuesta == 1){
        log_info(logger, "Proceso %d inicializado correctamente",proceso->PID);
        crear_hilo(proceso, proceso->prioridad_hilo_0, proceso->archivo_pseudocodigo_principal);
    }
    else{
        log_info(logger, "Proceso %d no creado, Memoria llena",proceso->PID);
    }
}

void finalizar_proceso(int proceso_id){
    // Crear conexión efimera a la memoria 
    int respuesta = informar_finalizacion_proceso_a_memoria(proceso_id);

    if (respuesta == 1) {
            log_info(logger, "Proceso %d finalizado correctamente", proceso_id);
            liberar_PCB(procesos_sistema,proceso_id);
            sem_post(&verificar_cola_new);
    }else {
        log_error(logger, "No se pudo finalizar el proceso %d.", proceso_id);
    }
}

void crear_hilo(PCB* proceso, int prioridad, char* archivo_pseudocodigo) {
    // Informar a Memoria
    int respuesta = informar_creacion_hilo_a_memoria(proceso->PID, list_size(proceso->TIDs));
    if (respuesta == 1){
        list_add(proceso->TIDs, list_size(proceso->TIDs));
        TCB *nuevo_tcb = malloc(sizeof(TCB));
        nuevo_tcb->TID = list_size(proceso->TIDs);
        nuevo_tcb->prioridad = prioridad;
        nuevo_tcb->PID = proceso->PID;
        log_info(logger, "Hilo %d para el proceso %d creado correctamente",nuevo_tcb->TID, proceso->PID);
        // Enviar hilo a cola READY
        agregar_a_ready(nuevo_tcb);
    }
    else{
        log_error(logger, "Hilo %d para el proceso %d no iniciado",list_size(proceso->TIDs), proceso->PID);
    }
}

void finalizar_hilo(int hilo_id, int pid) {
    int respuesta = informar_finalizacion_hilo_a_memoria(hilo_id, pid);
    // Procesar la respuesta de la memoria
    if (respuesta == 1) {
        log_info(logger, "Hilo %d finalizado correctamente del proceso %d finalizado correctamente", hilo_id, pid);
        TCB* hilo = list_get(cola_exec, 0);
        free(hilo);
        mover_hilos_bloqueados_por(hilo);
    } else {
        log_error(logger, "No se pudo finalizar el hilo %d", hilo_id);
    }
}

void liberar_PCB(t_list* lista_pcbs, int pid) {
    for (int i = 0; i < list_size(lista_pcbs); i++) {
        PCB* pcb = list_get(lista_pcbs, i);  // Obtén el PCB en la posición i
        if (pcb->PID == pid) {
            list_remove(lista_pcbs, i);
            free(pcb);
            break;
        }
    }
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

int informar_creacion_hilo_a_memoria(int pid, int tid){
    // Crear conexión efímera a la memoria
    conectar_memoria();
	t_paquete* paquete = crear_paquete();
	agregar_a_paquete(paquete, &pid, sizeof(int));
	agregar_a_paquete(paquete, &tid, sizeof(int));
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

