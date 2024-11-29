#include "../include/mem_kernel.h"

t_proceso *obtener_proceso(int pid)
{
    proceso_buscado_kernel = pid;
    t_proceso *proceso = list_find(lista_procesos_en_memoria, (void *)coincidePidKernel);
    if (!proceso)
    {
        log_error(logger, "NO SE ENCONTRO EL PROCESO BUSCADO.");
        exit(EXIT_FAILURE);
    }
    return proceso;
}

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

t_particion *encontrar_particion_bestfit(int tamanio)
{
    t_particion *particion_elegida = NULL;
    for (int i = 0; i < list_size(lista_particiones); i++)
    {
        t_particion *particion_actual = list_get(lista_particiones, i);

        // Si no elegi una particion aun \\ si encontre una particion que se ajuste mejor, cambio la que encontre por la que habia elegido previamente.
        if ((!particion_elegida || particion_actual->tamanio < particion_elegida->tamanio) && particion_actual->tamanio >= tamanio && particion_actual->libre)
        {
            particion_elegida = particion_actual;
        }
    }
    return particion_elegida;
}

void asignar_memoria_estatica(t_proceso *proceso, t_particion *particion_elegida, int tamanio)
{
    particion_elegida->libre = false;
    proceso->contexto->base = particion_elegida->base;
    proceso->contexto->limite = particion_elegida->tamanio;
    log_info(logger, "PARTICION ASIGNADA A PROCESO CON PID: %d, NUMERO: %d / TAMANIO PEDIDO: %d / TAMANIO DE LA PARTICION: %d", proceso->pid, particion_elegida->orden, tamanio, particion_elegida->tamanio);
}

void asignar_memoria_dinamica(t_proceso *proceso, t_particion *particion_original, int tamanio_pedido)
{
    // Creo una particion con el tamanio adecuado
    t_particion *particion_nueva = malloc(sizeof(t_particion));
    particion_nueva->base = particion_original->base;
    particion_nueva->tamanio = tamanio_pedido;
    particion_nueva->libre = false;
    particion_nueva->orden = NULL;

    list_add(lista_particiones, particion_nueva);

    // Si en la particion original no va a quedar espacio, la borro de la lista y la libero.
    if (particion_original->tamanio == tamanio_pedido)
    {
        list_remove_element(lista_particiones, particion_original);
        free(particion_original);
    }

    // Resto el tamanio que ocupe y muevo el puntero a la base tantos bytes como haya solicitado el proceso.
    particion_original->tamanio -= tamanio_pedido;
    particion_original->base += tamanio_pedido;

    proceso->contexto->base = particion_nueva->base;
    proceso->contexto->limite = particion_nueva->tamanio;
    log_info(logger, "PARTICION ASIGNADA A PROCESO CON PID: %d, NUMERO: %d / TAMANIO PEDIDO: %d / TAMANIO DE LA PARTICION: %d", proceso->pid, particion_nueva->orden, tamanio_pedido, particion_nueva->tamanio);
}

// Por ahora, solamente esta contemplado el esquema de particiones fijas. Esta funcion sigue una logica best-fit.
respuesta_pedido asignar_memoria(int tamanio, t_proceso *proceso)
{
    if (tamanio <= 0)
    {
        log_error(logger, "INGRESE UN VALOR MAYOR A CERO PARA ASIGNAR MEMORIA.");
        return ERROR;
    }

    t_particion *particion_elegida = encontrar_particion_bestfit(tamanio);

    if (!particion_elegida)
    {
        log_error(logger, "NO SE PUDO ASIGNAR LA MEMORIA AL PROCESO %d", proceso->pid);
        return ERROR;
    }

    if (strcmp(ESQUEMA, "FIJAS"))
    {
        asignar_memoria_estatica(proceso, particion_elegida, tamanio);
    }
    else
    {
        asignar_memoria_dinamica(proceso, particion_elegida, tamanio);
    }

    return OK;
}

respuesta_pedido crear_proceso(int pid, int tamanio)
{
    t_proceso *proceso = malloc(sizeof(t_proceso));
    proceso->pid = pid;
    proceso->contexto = malloc(sizeof(t_contexto_proceso));
    proceso->contexto->limite = tamanio;
    proceso->lista_hilos = list_create();
    proceso->nro_hilos = 0;
    if (asignar_memoria(tamanio, proceso) == -1)
    {
        log_error(logger, "NO PUDO ASIGNARSE LA MEMORIA AL PROCESO");
        return ERROR;
    }
    list_add(lista_procesos_en_memoria, proceso);
    return OK;
}

void compactar_memoria()
{
    t_particion *particion_elegida = NULL;
    for (int i = 0; i < list_size(lista_particiones) - 1; i++)
    {
        t_particion *particion_actual = list_get(lista_particiones, i);
        t_particion *particion_siguiente = list_get(lista_particiones, i + 1);
        if (particion_actual->libre && particion_siguiente->libre)
        {
            // Esto funciona pues, al finalizar un proceso, nunca voy a tener 3 espacios de memoria libres y consecutivos.
            particion_actual->tamanio += particion_siguiente->tamanio;
            list_remove_and_destroy_element(lista_particiones, i + 1, (void *)free);
        }
    }
}

respuesta_pedido finalizar_proceso(int pid)
{
    t_proceso *proceso = obtener_proceso(pid);
    list_remove_element(lista_procesos_en_memoria, proceso);
    destruir_proceso(proceso); // Libera la memoria utilizada por la estructura del proceso.
    if (strcmp(ESQUEMA, "DINAMICAS"))
    {
        compactar_memoria();
    }
    return OK;
}

respuesta_pedido crear_hilo(int pid, char *archivo_de_pseudocodigo)
{
    t_proceso *proceso = obtener_proceso(pid);
    t_hilo *nuevo_hilo = malloc(sizeof(t_hilo));
    nuevo_hilo->tid = proceso->nro_hilos;
    nuevo_hilo->registros = malloc(sizeof(t_registros_cpu));
    inicializar_registros(nuevo_hilo->registros);
    nuevo_hilo->lista_instrucciones = list_create();

    list_add(proceso->lista_hilos, nuevo_hilo);
    leer_archivo_pseudocodigo(archivo_de_pseudocodigo, nuevo_hilo->lista_instrucciones);

    if (!nuevo_hilo)
    {
        log_error("Error en la creacion del hilo para el proceso %d", pid);
        return ERROR;
    }

    proceso->nro_hilos += 1;
    log_info(logger, "HILO CREADO EXITOSAMENTE PARA PID: %d CON TID: %d", proceso->pid, nuevo_hilo->tid);
    return OK;
}

respuesta_pedido finalizar_hilo(int pid, int tid)
{
    t_proceso *proceso = obtener_proceso(pid);
    t_hilo *hilo = obtener_hilo(pid, tid);
    list_remove_element(proceso->lista_hilos, hilo);
    destruir_hilo(hilo);
    return OK;
}

// ----------------------------------------
// CICLO DE ATENCION DE PROCESOS DEL KERNEL
// ----------------------------------------

void atender_kernel()
{
    // Cuando llega una peticion del Kernel, se crea un hilo nuevo y se le delega el trabajo     bool control_key = 1;
    bool control_key = 1;
    t_buffer *buffer;

    while (control_key)
    {
        int cod_op = recibir_operacion(fd_kernel);
        recibir_entero(fd_kernel);
        int pid = recibir_entero(fd_kernel);
        int tid;
        int tamanio;
        respuesta_pedido respuesta;
        t_paquete *paquete;
        switch (cod_op)
        {
        case INICIALIZAR_PROCESO:
            tamanio = recibir_entero(fd_kernel);
            respuesta = crear_proceso(pid, tamanio);
            sleep(RETARDO_RESPUESTA);
            paquete = crear_paquete();
            agregar_respuesta_enviar_paquete(paquete, respuesta);
            free(paquete);
            break;
        case FINALIZACION_PROCESO:
            respuesta = finalizar_proceso(pid);
            sleep(RETARDO_RESPUESTA);
            paquete = crear_paquete();
            agregar_respuesta_enviar_paquete(paquete, respuesta);
            free(paquete);
            break;
        case CREACION_HILO:
            tid = recibir_entero(fd_kernel);
            tamanio = recibir_entero(fd_kernel);
            char *nombre_archivo = malloc(tamanio);
            recibir_buffer(nombre_archivo, tamanio);
            respuesta = crear_hilo(pid, nombre_archivo);
            sleep(RETARDO_RESPUESTA);
            paquete = crear_paquete();
            agregar_respuesta_enviar_paquete(paquete, respuesta);
            free(paquete);
            break;
        case FINALIZACION_HILO:
            tid = recibir_entero(fd_kernel);
            respuesta = finalizar_hilo(pid, tid);
            sleep(RETARDO_RESPUESTA);
            paquete = crear_paquete();
            agregar_respuesta_enviar_paquete(paquete, respuesta);
            free(paquete);
            break;
        case MEMORY_DUMP:
            tid = recibir_entero(fd_kernel);
            sleep(RETARDO_RESPUESTA);
            respuesta = generarMemoryDump(pid, tid);
            paquete = crear_paquete();
            agregar_respuesta_enviar_paquete(paquete, respuesta);
            free(paquete);
            break;
        case EXIT:
            control_key = 0;
            break;
        default:
            log_error(logger, "No se entendio la operacion del Kernel");
        }
    }
}
