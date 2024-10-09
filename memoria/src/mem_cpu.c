#include "../include/mem_cpu.h"

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