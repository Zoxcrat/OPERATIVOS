#include "../include/mem_kernel.h"

// Por ahora, solamente esta contemplado el esquema de particiones fijas.
int asignar_memoria(int tamanio, t_proceso *proceso)
{
    if (tamanio <= 0)
    {
        log_error(logger, "INGRESE UN VALOR MAYOR A CERO PARA ASIGNAR MEMORIA.");
        return -1;
    }

    t_particion *particion_elegida = NULL;
    for (int i = 0; i < list_size(lista_particiones); i++)
    {
        t_particion *particion_actual = list_get(lista_particiones, i);

        // Si no elegi una particion aun O si encontre una particion que se ajuste mejor, cambio la que encontre por la que habia elegido previamente.
        if ((!particion_elegida || particion_actual->tamanio < particion_elegida->tamanio) && particion_actual->tamanio >= tamanio && particion_actual->libre)
        {
            particion_elegida = particion_actual;
        }
    }

    if (particion_elegida)
    {
        particion_elegida->libre = false;
        proceso->contexto->base = particion_elegida->base;
        log_info(logger, "PARTICION ASIGNADA, NUMERO: %d / TAMANIO PEDIDO: %d / TAMANIO DE LA PARTICION: %d", particion_elegida->orden, tamanio, particion_elegida->tamanio);
        return 0;
    }
    else
    {
        log_error(logger, "NO SE PUDO ASIGNAR LA MEMORIA.");
        return -1;
    }
}

int crear_proceso(int pid, int tamanio)
{
    t_proceso *proceso = malloc(sizeof(t_proceso));
    proceso->pid = pid;
    proceso->contexto->limite = tamanio;
    proceso->lista_hilos = list_create();
    if (asignar_memoria(tamanio, proceso) == -1)
    {
        log_error(logger, "NO PUDO ASIGNARSE LA MEMORIA AL PROCESO");
        EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

void atender_kernel()
{
    // Cuando llega una peticion del Kernel, se crea un hilo nuevo y se le delega el trabajo     bool control_key = 1;
    bool control_key = 1;
    t_buffer *buffer;

    while (control_key)
    {
        int cod_op = recibir_operacion(fd_kernel);
        recibir_entero(fd_kernel);
        int pid;
        int tid;
        t_paquete *paquete;
        switch (cod_op)
        {
        case INICIALIZAR_PROCESO:
            pid = recibir_entero(fd_kernel);
            tid = recibir_entero(fd_kernel);
            break;
        case FINALIZACION_PROCESO:
            break;
        case CREACION_HILO:
            break;
        case FINALIZACION_HILO:
            break;
        default:
            log_error(logger, "No se entendio la operacion del Kernel");
            control_key = 0;
        }
    }
}
