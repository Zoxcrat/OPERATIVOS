#include "./gestor_io.h"

void inicializar_gestor_io()
{
    pthread_create(hilo_gestor_io, NULL, gestor_io, NULL);
    pthread_detach(*hilo_gestor_io);
}

void *gestor_io(void)
{
    while (true)
    {
        // Espera a que haya procesos en estado BLOCKED
        sem_wait(&hay_hilos_en_io);

        if (!list_is_empty(cola_io))
        {
            // Tomar el primer proceso de la lista de procesos en cola IO
            pthread_mutex_lock(&mutex_cola_io);
            TCB *hilo = list_remove(cola_io, 0);
            pthread_mutex_unlock(&mutex_cola_io);

            pthread_mutex_lock(&mutex_log);
            log_warning(logger, "El hilo %d del proceso %d hace su I/O de %d segundos", hilo->TID, hilo->PID, hilo->tiempo_bloqueo_io);
            pthread_mutex_unlock(&mutex_log);

            // Bloquear el tiempo correspondiente
            usleep(hilo->tiempo_bloqueo_io);

            // Loggear que el proceso terminó el I/O
            pthread_mutex_lock(&mutex_log);
            log_info(logger_obligatorio, "## (%d:%d) finalizó IO y pasa a READY", hilo->TID, hilo->PID);
            pthread_mutex_unlock(&mutex_log);

            // Pasar el proceso a READY
            hilo->estado = READY;
            agregar_a_ready(hilo);

            sem_post(&hay_hilos_en_ready);
        }
    }
}
