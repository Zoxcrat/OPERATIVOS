#include "../include/mem_cpu.h"

bool coincidePid(t_proceso *proceso)
{
    return (proceso->pid == proceso_buscado);
}

t_contexto_ejecucion *obtener_contexto(int pid, int tid)
{
    // Atajar errores si no encuentra algo de esto que esta abajo.
    t_proceso *proceso = list_find(lista_procesos_en_memoria, (void *)coincidePid);
    t_hilo *hilo = list_get(proceso->lista_hilos, tid);

    t_contexto_ejecucion *contexto_completo = malloc(sizeof(t_contexto_ejecucion));
    contexto_completo->base = proceso->contexto->base;
    contexto_completo->limite = proceso->contexto->limite;
    contexto_completo->AX = hilo->registros->AX;
    contexto_completo->BX = hilo->registros->BX;
    contexto_completo->CX = hilo->registros->CX;
    contexto_completo->DX = hilo->registros->DX;
    contexto_completo->EX = hilo->registros->EX;
    contexto_completo->FX = hilo->registros->FX;
    contexto_completo->GX = hilo->registros->GX;
    contexto_completo->HX = hilo->registros->HX;
    contexto_completo->PC = hilo->registros->PC;

    return contexto_completo;
}

void atender_cpu()
{

    bool control_key = 1;
    t_buffer *buffer;

    while (control_key)
    {
        int cod_op = recibir_operacion(fd_cpu);
        recibir_entero(fd_cpu);
        int pid;
        int tid;
        t_paquete *paquete;
        switch (cod_op)
        {
        case PEDIDO_CONTEXTO:
            pid = recibir_entero(fd_cpu);
            tid = recibir_entero(fd_cpu);
            t_contexto_ejecucion *contexto = obtener_contexto(pid, tid);
            agregar_a_paquete(paquete, PEDIDO_CONTEXTO, sizeof(op_code));
            agregar_a_paquete(paquete, (void *)contexto, sizeof(t_contexto_ejecucion));
            enviar_paquete(fd_cpu);
            break;
        case ACTUALIZAR_CONTEXTO:
            break;
        case OBTENER_INSTRUCCION:
            break;
        case ESCRIBIR_MEMORIA:
            break;
        case LEER_MEMORIA:
            break;
        default:
            log_error(logger, "La CPU no entendio la operacion.");
            control_key = 0;
        }
    }
}