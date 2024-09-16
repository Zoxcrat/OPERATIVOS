#include "./gestor_dump_memory.h"

void inicializar_gestor_dump_memory()
{
    pthread_create(hilo_gestor_dump_memory, NULL, gestor_dump_memory, NULL);
    pthread_detach(*hilo_gestor_dump_memory);
}

void *gestor_dump_memory(void)
{
    while (true)
    {
        // Espera a que haya procesos en estado BLOCKED
        sem_wait(&hay_hilos_en_dump_memory);

        if (!list_is_empty(cola_dump_memory)){
            // Tomar el primer proceso de la lista de procesos en cola IO
            pthread_mutex_lock(&mutex_cola_dump_memory);
            TCB* hilo = list_remove(cola_dump_memory, 0);
            pthread_mutex_unlock(&mutex_cola_dump_memory);

            pthread_mutex_lock(&mutex_log);
            log_warning(logger, "El hilo %d del proceso %d comienza a hacer DUMP_MEMORY",hilo->TID,hilo->PID);
            pthread_mutex_unlock(&mutex_log);

            conectar_memoria();
            t_paquete* paquete = crear_paquete();
            agregar_a_paquete(paquete, &hilo->PID, sizeof(int));
            agregar_a_paquete(paquete, &hilo->TID, sizeof(int));
            enviar_peticion(paquete,fd_memoria,DUMP_MEMORY);
            eliminar_paquete(paquete);
            int respuesta;
            recv(fd_cpu_dispatch, &respuesta, sizeof(int), 0);

            pthread_mutex_lock(&mutex_log);
            log_warning(logger, "El hilo %d del proceso %d completo el DUMP_MEMORY, vuelve a READY",hilo->TID,hilo->PID);
            pthread_mutex_unlock(&mutex_log);

            // Pasar el proceso a READY
            hilo->estado = READY;
            agregar_a_ready(hilo);
            
            close(fd_memoria);

            sem_post(&hay_hilos_en_ready);
        }
    }
}