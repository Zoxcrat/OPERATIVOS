#include "../include/mem_cleanup.h"

void destruir_hilo(t_hilo *hilo)
{
    free(hilo->tid);
    free(hilo->registros);
    list_destroy_and_destroy_elements(hilo->lista_instrucciones, (void *)free);
}

void destruir_proceso(t_proceso *proceso)
{
    free(proceso->pid);
    free(proceso->contexto);
    list_destroy_and_destroy_elements(proceso->lista_hilos, (void *)destruir_hilo);
}

bool coincidePidKernel(t_proceso *proceso)
{
    return (proceso->pid == proceso_buscado_kernel);
}

bool coincidePidCpu(t_proceso *proceso)
{
    return (proceso->pid == proceso_buscado_cpu);
}

bool coincidePidFs(t_proceso *proceso)
{
    return (proceso->pid == proceso_buscado_fs);
}

t_hilo *obtener_hilo(int pid, int tid)
{
    t_proceso *proceso = obtener_proceso(pid);

    t_hilo *hilo = list_get(proceso->lista_hilos, tid);
    if (!hilo)
    {
        log_error(logger, "NO SE ENCONTRO EL HILO BUSCADO.");
        exit(EXIT_FAILURE);
    }
    return hilo;
}

void agregar_respuesta_enviar_paquete(t_paquete *paquete, respuesta_pedido respuesta)
{
    agregar_a_paquete(paquete, (void *)respuesta, sizeof(respuesta_pedido));
    enviar_paquete(paquete, fd_kernel);
}