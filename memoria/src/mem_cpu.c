#include "../include/mem_cpu.h"

void inicializar_registros(t_registros_cpu *registros)
{
    registros->AX = 0;
    registros->BX = 0;
    registros->CX = 0;
    registros->DX = 0;
    registros->EX = 0;
    registros->FX = 0;
    registros->GX = 0;
    registros->HX = 0;
    registros->PC = 0;
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