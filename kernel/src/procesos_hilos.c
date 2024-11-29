#include "./procesos_hilos.h"

void crear_proceso(char *archivo_pseudocodigo, int tamanio_proceso, int prioridad_hilo_0)
{
    pid_global++;
    PCB *nuevo_proceso = malloc(sizeof(PCB));
    nuevo_proceso->PID = pid_global; // Generar un identificador único para el proceso
    nuevo_proceso->TIDs = list_create();
    nuevo_proceso->mutexs = list_create();
    nuevo_proceso->tamanio = tamanio_proceso;
    nuevo_proceso->archivo_pseudocodigo_principal = archivo_pseudocodigo;
    nuevo_proceso->prioridad_hilo_0 = prioridad_hilo_0; // lo pongo en el pcb por si todavia no se puede inicializar el proceso en memoria

    pthread_mutex_lock(&mutex_log);
    log_info(logger_obligatorio, "## (%d:0) Se crea el proceso - Estado: NEW", nuevo_proceso->PID);
    pthread_mutex_unlock(&mutex_log);

    pthread_mutex_lock(&mutex_procesos_en_new);
    list_add(cola_new, nuevo_proceso);
    pthread_mutex_unlock(&mutex_procesos_en_new);

    sem_post(&verificar_cola_new);
}

void inicializar_proceso(PCB *proceso)
{
    int respuesta = informar_inicializacion_proceso_a_memoria(proceso->PID, proceso->tamanio);
    if (respuesta == 1)
    {
        pthread_mutex_lock(&mutex_log);
        log_info(logger, "Proceso %d inicializado correctamente", proceso->PID);
        pthread_mutex_unlock(&mutex_log);
        // saca el proceso de la cola new
        pthread_mutex_lock(&mutex_procesos_en_new);
        list_remove_element(cola_new, proceso);
        pthread_mutex_unlock(&mutex_procesos_en_new);

        pthread_mutex_lock(&mutex_procesos_sistema);
        list_add(procesos_sistema, proceso);
        pthread_mutex_unlock(&mutex_procesos_sistema);
        // crea el hilo principal
        crear_hilo(proceso, proceso->prioridad_hilo_0, proceso->archivo_pseudocodigo_principal);

        free(proceso->archivo_pseudocodigo_principal);
    }
    else
    {
        pthread_mutex_lock(&mutex_log);
        log_info(logger, "Proceso %d no inicializado, Memoria llena. Se mantiene en NEW", proceso->PID);
        pthread_mutex_unlock(&mutex_log);
    }
}

void finalizar_proceso(int proceso_id)
{
    // Crear conexión efimera a la memoria
    int respuesta = informar_finalizacion_proceso_a_memoria(proceso_id);

    if (respuesta == 1)
    {
        pthread_mutex_lock(&mutex_procesos_sistema);
        PCB *proceso = obtener_proceso_por_pid(proceso_id);
        pthread_mutex_unlock(&mutex_procesos_sistema);

        if (proceso != NULL)
        {
            eliminar_hilos_asociados(proceso);
            liberar_PCB(proceso);

            pthread_mutex_lock(&mutex_log);
            log_info(logger_obligatorio, "## Finaliza el proceso %d", hilo_en_exec->PID);
            pthread_mutex_unlock(&mutex_log);

            sem_post(&verificar_cola_new);
        }
        else
        {
            pthread_mutex_lock(&mutex_log);
            log_error(logger, "El proceso %d no existe.", proceso_id);
            pthread_mutex_unlock(&mutex_log);
        }
    }
    else
    {
        pthread_mutex_lock(&mutex_log);
        log_error(logger, "No se pudo finalizar el proceso %d.", proceso_id);
        pthread_mutex_unlock(&mutex_log);
    }
}

void crear_hilo(PCB *proceso, int prioridad, char *archivo_pseudocodigo)
{
    // Informar a Memoria
    int id_proximo_hilo = obtener_numero_proximo_hilo(proceso);
    int respuesta = informar_creacion_hilo_a_memoria(proceso->PID, id_proximo_hilo, archivo_pseudocodigo);
    if (respuesta == 1)
    {
        list_add(proceso->TIDs, id_proximo_hilo);
        TCB *nuevo_tcb = malloc(sizeof(TCB));
        nuevo_tcb->PID = proceso->PID;
        nuevo_tcb->TID = id_proximo_hilo;
        nuevo_tcb->prioridad = prioridad;
        nuevo_tcb->archivo_pseudocodigo = archivo_pseudocodigo;

        pthread_mutex_lock(&mutex_hilos_sistema);
        list_add(hilos_sistema, nuevo_tcb);
        pthread_mutex_unlock(&mutex_hilos_sistema);

        // Enviar hilo a cola READY dependiendo del algoritmo de planificacion
        agregar_a_ready(nuevo_tcb);

        pthread_mutex_lock(&mutex_log);
        log_info(logger_obligatorio, "Creación de Hilo: “## (%d:%d) Se crea el Hilo - Estado: READY", nuevo_tcb->PID, nuevo_tcb->TID);
        pthread_mutex_unlock(&mutex_log);
    }
    else
    {
        pthread_mutex_lock(&mutex_log);
        log_error(logger, "Hilo %d para el proceso %d no iniciado", list_size(proceso->TIDs), proceso->PID);
        pthread_mutex_unlock(&mutex_log);
    }
}

void finalizar_hilo(int pid, int hilo_id)
{
    int respuesta = informar_finalizacion_hilo_a_memoria(pid, hilo_id);
    // Procesar la respuesta de la memoria
    TCB *hilo = buscar_hilo_por_pid_tid(pid, hilo_id);
    if (respuesta == 1)
    {
        mover_hilos_bloqueados_por_thread_join(hilo);
        desbloquear_hilos_por_mutex_tomado(hilo);

        mandar_hilo_a_exit(pid, hilo_id);

        pthread_mutex_lock(&mutex_log);
        log_info(logger_obligatorio, "## (%d:%d) Finaliza el hilo", pid, hilo_id);
        pthread_mutex_unlock(&mutex_log);
    }
    else
    {
        pthread_mutex_lock(&mutex_log);
        log_error(logger, "No se pudo finalizar el hilo %d", hilo_id);
        pthread_mutex_unlock(&mutex_log);
    }
}

void liberar_PCB(PCB *proceso)
{
    pthread_mutex_lock(&mutex_procesos_sistema);
    list_remove_element(procesos_sistema, proceso);
    pthread_mutex_unlock(&mutex_procesos_sistema);

    list_destroy(proceso->TIDs);
    list_destroy(proceso->mutexs);
    free(proceso->archivo_pseudocodigo_principal);
    free(proceso);
}

void liberar_TCB(TCB *hilo)
{
    free(hilo->archivo_pseudocodigo);
    free(hilo);
}

void mover_hilos_bloqueados_por_thread_join(TCB *hilo)
{
    pthread_mutex_lock(&mutex_cola_join_wait);
    for (int i = 0; i < list_size(cola_joins); i++)
    {
        t_join_wait *relacion = list_get(cola_joins, i);

        // Verificamos que coincidan el PID y el TID del hilo esperado
        if (relacion->pid == hilo->PID && relacion->tid_esperado == hilo->TID)
        {
            TCB *hilo_esperador = relacion->tid_esperador;

            if (hilo_esperador != NULL)
            {
                hilo_esperador->estado = READY;
                agregar_a_ready(hilo_esperador); // Movemos el hilo esperador a la cola de ready
            }

            list_remove_and_destroy_element(cola_joins, i, free);
            i--;
        }
    }
    pthread_mutex_unlock(&mutex_cola_join_wait);
}

void desbloquear_hilos_por_mutex_tomado(TCB *hilo)
{
    PCB *proceso = obtener_proceso_por_pid(hilo->PID);

    // Recorrer la lista de mutex del proceso
    for (int i = 0; i < list_size(proceso->mutexs); i++)
    {
        t_mutex *mutex = list_get(proceso->mutexs, i);

        // Verificar si el mutex está tomado por el hilo que se está finalizando
        if (mutex->TID == hilo->TID)
        {
            // Si hay hilos bloqueados esperando este mutex, desbloquear al primero
            if (list_size(mutex->cola_bloqueados) > 0)
            {
                TCB *hilo_desbloqueado = list_remove(mutex->cola_bloqueados, 0);
                mutex->TID = hilo_desbloqueado->TID; // Asignar el mutex al hilo desbloqueado

                // Cambiar el estado del hilo desbloqueado a "READY" o su equivalente
                hilo_desbloqueado->estado = READY;
                agregar_a_ready(hilo_desbloqueado);

                pthread_mutex_lock(&mutex_log);
                log_info(logger, "Mutex '%s' reasignado al hilo TID: %d (anteriormente tomado por TID: %d)\n",
                         mutex->nombre, hilo_desbloqueado->TID, hilo->TID);
                pthread_mutex_unlock(&mutex_log);
            }
            else
            {
                // No hay hilos bloqueados, liberar el mutex
                mutex->TID = -1; // Liberar el mutex (ningún hilo lo tiene)

                pthread_mutex_lock(&mutex_log);
                log_info(logger, "Mutex '%s' liberado (anteriormente tomado por TID: %d)\n", mutex->nombre, hilo->TID);
                pthread_mutex_unlock(&mutex_log);
            }
        }
    }
}

// MUTEXS
void crear_mutex(char *nombre)
{
    PCB *proceso = obtener_proceso_por_pid(hilo_en_exec->PID);

    t_mutex *nuevo_mutex = malloc(sizeof(t_mutex));
    nuevo_mutex->nombre = nombre;
    nuevo_mutex->cola_bloqueados = list_create();
    nuevo_mutex->TID = -1;

    list_add(proceso->mutexs, nuevo_mutex); // Asociar el mutex al proceso

    pthread_mutex_lock(&mutex_log);
    log_info(logger, "Mutex %s creado para el proceso %d\n", nombre, proceso->PID);
    pthread_mutex_unlock(&mutex_log);
}

void lockear_mutex(char *nombre)
{
    // Obtener el proceso al que pertenece el hilo en ejecución
    PCB *proceso = obtener_proceso_por_pid(hilo_en_exec->PID);

    // Buscar el mutex dentro del proceso que coincida con el nombre proporcionado
    t_mutex *mutex = NULL;
    for (int i = 0; i < list_size(proceso->mutexs); i++)
    {
        t_mutex *m = list_get(proceso->mutexs, i);
        if (strcmp(m->nombre, nombre) == 0)
        {
            mutex = m;
            break;
        }
    }
    if (mutex->TID == -1)
    { // El mutex está libre,
        mutex->TID = hilo_en_exec->TID;
        pthread_mutex_lock(&mutex_log);
        log_info(logger, "Mutex '%s' asignado al hilo TID: %d del proceso %d\n", nombre, hilo_en_exec->TID, hilo_en_exec->PID);
        pthread_mutex_unlock(&mutex_log);

        interrupcion = 0;
        sem_post(&mandar_interrupcion);
    }
    else
    {
        pthread_mutex_lock(&mutex_log);
        log_info(logger_obligatorio, "## (%d:%d) - Bloqueado por: MUTEX", hilo_en_exec->PID, hilo_en_exec->TID);
        pthread_mutex_unlock(&mutex_log);
        hilo_desalojado = true;

        // Agregar el hilo en ejecución a la cola de bloqueados del mutex
        list_add(mutex->cola_bloqueados, hilo_en_exec);

        pthread_mutex_lock(&mutex_hilo_exec);
        hilo_en_exec = NULL;
        pthread_mutex_unlock(&mutex_hilo_exec);

        interrupcion = 1;
        sem_post(&mandar_interrupcion);
        sem_post(&hay_hilos_en_ready);
    }
}

void unlockear_mutex(char *nombre)
{
    PCB *proceso = obtener_proceso_por_pid(hilo_en_exec->PID);
    t_mutex *mutex = NULL;
    for (int i = 0; i < list_size(proceso->mutexs); i++)
    {
        t_mutex *m = list_get(proceso->mutexs, i);
        if (strcmp(m->nombre, nombre) == 0)
        {
            mutex = m;
            break;
        }
    }
    // Verificar si el mutex está tomado por el hilo en ejecución
    if (mutex->TID != hilo_en_exec->TID)
    {
        pthread_mutex_lock(&mutex_log);
        log_info(logger, "Error: El hilo TID: %d no tiene el mutex '%s' asignado\n", hilo_en_exec->TID, nombre);
        pthread_mutex_unlock(&mutex_log);
        return;
    }
    if (list_size(mutex->cola_bloqueados) > 0)
    {
        TCB *hilo_desbloqueado = list_remove(mutex->cola_bloqueados, 0);

        // Asignar el mutex al hilo desbloqueado
        mutex->TID = hilo_desbloqueado->TID;
        hilo_desbloqueado->estado = READY;
        agregar_a_ready(hilo_desbloqueado);

        pthread_mutex_lock(&mutex_log);
        log_info(logger, "Mutex '%s' reasignado al hilo TID: %d\n", nombre, hilo_desbloqueado->TID);
        pthread_mutex_unlock(&mutex_log);
    }
    else
    {
        // No hay hilos bloqueados, libero el mutex
        mutex->TID = -1;

        pthread_mutex_lock(&mutex_log);
        log_info(logger, "Mutex '%s' liberado por el hilo TID: %d\n", nombre, hilo_en_exec->TID);
        pthread_mutex_unlock(&mutex_log);
    }

    // Devolver el control al hilo que realizó la syscall MUTEX_UNLOCK (ya está en exec)
}

// PEDIDOS A MEMORIA

int informar_inicializacion_proceso_a_memoria(int pid, int tamanio)
{
    // Crear conexión efímera a la memoria
    t_paquete *paquete = crear_paquete();
    agregar_a_paquete(paquete, &pid, sizeof(int));
    agregar_a_paquete(paquete, &tamanio, sizeof(int));
    enviar_peticion(paquete, fd_memoria, INICIALIZAR_PROCESO);
    eliminar_paquete(paquete);
    int respuesta = recibir_entero(fd_memoria);
    return respuesta;
}

int informar_finalizacion_proceso_a_memoria(int pid)
{
    // Crear conexión efímera a la memoria
    t_paquete *paquete = crear_paquete();
    agregar_a_paquete(paquete, &pid, sizeof(int));
    enviar_peticion(paquete, fd_memoria, FINALIZACION_PROCESO);
    eliminar_paquete(paquete);
    int respuesta = recibir_entero(fd_memoria);
    return respuesta;
}

int informar_creacion_hilo_a_memoria(int pid, int tid, char *archivo_pseudocodigo)
{
    t_paquete *paquete = crear_paquete();
    agregar_a_paquete(paquete, &pid, sizeof(int));
    agregar_a_paquete(paquete, &tid, sizeof(int));
    agregar_a_paquete(paquete, archivo_pseudocodigo, strlen(archivo_pseudocodigo) + 1);
    enviar_peticion(paquete, fd_memoria, CREACION_HILO);
    eliminar_paquete(paquete);
    int respuesta = recibir_entero(fd_memoria);
    return respuesta;
}

int informar_finalizacion_hilo_a_memoria(int pid, int tid)
{
    t_paquete *paquete = crear_paquete();
    agregar_a_paquete(paquete, &pid, sizeof(int));
    agregar_a_paquete(paquete, &tid, sizeof(int));
    enviar_peticion(paquete, fd_memoria, FINALIZACION_HILO);
    eliminar_paquete(paquete);
    int respuesta = recibir_entero(fd_memoria);
    return respuesta;
}

/// FUNCIONES AUXILIARES ///
bool buscar_por_pid(void *proceso)
{
    PCB *un_proceso = (PCB *)proceso;
    return un_proceso->PID == pid_a_buscar;
}

PCB *obtener_proceso_por_pid(int pid)
{
    pid_a_buscar = pid;
    return (PCB *)list_find(procesos_sistema, buscar_por_pid);
}

bool buscar_por_pid_tid(void *hilo)
{
    TCB *un_hilo = (TCB *)hilo;
    return un_hilo->PID == pid_a_buscar && un_hilo->TID == tid_a_buscar;
}

TCB *buscar_hilo_por_pid_tid(int pid, int tid)
{
    pid_a_buscar = pid;
    tid_a_buscar = tid;
    return (TCB *)list_find(procesos_sistema, buscar_por_pid_tid);
}

void eliminar_hilos_asociados(PCB *proceso)
{
    for (int i = 0; i < list_size(proceso->TIDs); i++)
    {
        int hilo_asociado = list_get(proceso->TIDs, i);
        finalizar_hilo(proceso->PID, hilo_asociado);
    }
}

int obtener_numero_proximo_hilo(PCB *proceso)
{
    int proximo_hilo = -1;

    for (int i = 0; i < list_size(proceso->TIDs); i++)
    {
        int tid_actual = list_get(proceso->TIDs, i);
        if (tid_actual > proximo_hilo)
        {
            proximo_hilo = tid_actual;
        }
    }
    return proximo_hilo + 1;
}