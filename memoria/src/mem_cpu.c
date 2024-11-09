#include "../include/mem_cpu.h"

int proceso_buscado_cpu;

bool coincidePid(t_proceso *proceso)
{
    return (proceso->pid == proceso_buscado_cpu);
}

t_hilo *obtener_hilo(int pid, int tid)
{
    proceso_buscado_cpu = pid;
    t_proceso *proceso = list_find(lista_procesos_en_memoria, (void *)coincidePid);
    t_hilo *hilo = list_get(proceso->lista_hilos, tid);

    if (!hilo)
    {
        log_error(logger, "NO SE ENCONTRO EL HILO BUSCADO.");
        exit(EXIT_FAILURE);
    }
    return hilo;
}

t_contexto_ejecucion *obtener_contexto(int pid, int tid)
{

    t_hilo *hilo = obtener_hilo(pid, tid);

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

respuesta_pedido actualizar_contexto(int pid, int tid, t_registros_cpu *nuevo_contexto)
{
    t_hilo *hilo = obtener_hilo(pid, tid);
    t_registros_cpu registros_hilo = hilo->registros;
    registros_hilo = nuevo_contexto;
    return OK;
}

void agregar_respuesta_enviar_paquete(t_paquete *paquete, respuesta_pedido respuesta)
{
    agregar_a_paquete(paquete, (void *)respuesta, sizeof(respuesta_pedido));
    enviar_paquete(paquete, fd_kernel);
}

char *obtener_instruccion(int pid, int tid)
{
    t_hilo *hilo = obtener_hilo(pid, tid);
    int program_counter = hilo->registros->PC;
    char *instruccion = list_get(hilo->lista_instrucciones, program_counter);
    return instruccion;
}

void *read_mem(int base)
{
    void *inicio_lectura = memoria_usuario + base;
    void *contenido = malloc(sizeof(char) * 4);
    if (!contenido)
    {
        log_error(logger, "NO SE PUDO RESERVAR MEMORIA PARA LA LECTURA.");
        exit(EXIT_FAILURE);
    }
    memcpy(contenido, inicio_lectura, sizeof(char) * 4);
    return contenido;
}

respuesta_pedido write_mem(int base, void *contenido_escritura)
{
    void *inicio_lectura = memoria_usuario + base;
    memcpy(inicio_lectura, contenido_escritura, sizeof(char) * 4);
    return OK;
}

void atender_cpu()
{

    bool control_key = 1;
    t_buffer *buffer;

    while (control_key)
    {
        int cod_op = recibir_operacion(fd_cpu);
        recibir_entero(fd_cpu);
        int pid = recibir_entero(fd_cpu);
        int tid;
        t_paquete *paquete;
        respuesta_pedido respuesta;
        switch (cod_op)
        {
        case PEDIDO_CONTEXTO:
            tid = recibir_entero(fd_cpu);
            t_contexto_ejecucion *contexto = obtener_contexto(pid, tid);
            agregar_a_paquete(paquete, PEDIDO_CONTEXTO, sizeof(op_code));
            agregar_a_paquete(paquete, (void *)contexto, sizeof(t_contexto_ejecucion));
            sleep(RETARDO_RESPUESTA);
            enviar_paquete(fd_cpu);
            break;
        case ACTUALIZAR_CONTEXTO:
            tid = recibir_entero(fd_cpu);
            t_registros_cpu *contexto = recibir_contexto(fd_cpu);
            respuesta = actualizar_contexto(pid, tid, contexto);
            sleep(RETARDO_RESPUESTA);
            agregar_respuesta_enviar_paquete(paquete, respuesta);
            break;
        case OBTENER_INSTRUCCION:
            tid = recibir_entero(fd_cpu);
            char *instruccion = obtener_instruccion(pid, tid);
            agregar_a_paquete(paquete, (void *)instruccion, strlen(instruccion));
            sleep(RETARDO_RESPUESTA);
            enviar_paquete(fd_cpu);
            break;
        case ESCRIBIR_MEMORIA:
            int base = recibir_entero(fd_cpu);
            void *contenido = recibir_buffer(fd_cpu, sizeof(char) * 4);
            respuesta = write_mem(base, contenido);
            sleep(RETARDO_RESPUESTA);
            agregar_respuesta_enviar_paquete(paquete, respuesta);
            break;
        case LEER_MEMORIA:
            int base = recibir_entero(fd_cpu);
            void *contenido = read_mem(base);
            agregar_a_paquete(paquete, contenido, sizeof(char) * 4);
            sleep(RETARDO_RESPUESTA);
            enviar_paquete(paquete, fd_cpu);
            break;
        default:
            log_error(logger, "La CPU no entendio la operacion.");
            control_key = 0;
        }
    }
}