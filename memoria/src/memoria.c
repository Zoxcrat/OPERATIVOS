#include "../include/mem_gestor.h"

int main(int argc, char **argv)
{
    if (argc > 3)
    {
        return EXIT_FAILURE;
    }

    inicializar_memoria();
    asignar_memoria(13);
    asignar_memoria(13);

    leer_archivo_pseudocodigo("prueba.txt");

    // Conexion con FS
    fd_filesystem = crear_conexion2(IP_FILESYSTEM, PUERTO_FILESYSTEM);
    if (fd_filesystem == -1)
    {
        log_error(logger, "Error al conectar con el módulo FILESYSEM");
        // return EXIT_FAILURE;
    }

    int fd_cliente;
    for (int i = 0; i < 3; i++)
    {
        log_info(logger, "Esperando Clientes...");
        fd_cliente = esperar_cliente(memoria_socket, logger);

        int cod_op = recibir_operacion(fd_cliente);
        switch (cod_op)
        {
        case KERNEL:
            log_info(logger, "Se conecto el KERNEL");
            fd_kernel = fd_cliente;
            atender_kernel();
            break;
        case CPU:
            log_info(logger, "Se conecto la CPU");
            fd_cpu = fd_cliente;
            atender_cpu();
            break;
        default:
            log_error(logger, "No reconozco ese codigo");
            break;
        }
    }
    // Esperar conexión del módulo CPU
    /*     int fd_cpu = esperar_cliente(memoria_socket, logger);
        if (fd_cpu == -1)
        {
            log_error(logger, "Error al conectar con el módulo CPU");
            // return EXIT_FAILURE;
        }
        else
        {
            log_info(logger, "Conexión con CPU establecida");
        }
        while (1)
        {
            pthread_t hilo_cliente;
            int socket_cliente = esperar_cliente(memoria_socket, logger);
            if (socket_cliente == -1)
            {
                log_info(logger, "Hubo un error en la conexión del cliente");
                continue;
            }
            pthread_create(&hilo_cliente, NULL, (void *)procesar_conexion, (void *)(intptr_t)socket_cliente);
            pthread_detach(hilo_cliente);
        } */

    terminar_programa();
    return 0;
}

void *procesar_conexion(void *arg)
{
    int socket_cliente = (int)(intptr_t)arg;
    op_code cop;

    while (recv(socket_cliente, &cop, sizeof(op_code), 0) == sizeof(op_code))
    {
        pthread_t hilo_peticion;
        pthread_create(&hilo_peticion, NULL, procesar_peticion, (void *)(intptr_t)socket_cliente);
        pthread_detach(hilo_peticion);
    }

    log_info(logger, "El cliente se desconectó del servidor");
    close(socket_cliente);
    return NULL;
}

void *procesar_peticion(void *arg)
{
    int socket_cliente = (int)(intptr_t)arg;

    // Lógica para procesar la petición, ejemplo:
    // op_code cop;
    // recv(socket_cliente, &cop, sizeof(op_code), 0);

    // Conectar con FileSystem para cada petición
    int fd_filesystem = crear_conexion2(IP_FILESYSTEM, PUERTO_FILESYSTEM);
    if (fd_filesystem == -1)
    {
        log_error(logger, "Fallo la conexión con Filesystem para la petición");
        return NULL;
    }
    // Enviar una solicitud al FileSystem
    enviar_mensaje("Petición al FileSystem", fd_filesystem);

    // Recibir respuesta del FileSystem y procesarla
    // ...

    // Cerrar la conexión con FileSystem
    close(fd_filesystem);

    // Simular el retardo de respuesta
    sleep(RETARDO_RESPUESTA);

    // Enviar resultado de la operación al Kernel
    // enviar_mensaje("Resultado de la petición", socket_cliente);

    return NULL;
}

void terminar_programa()
{
    log_destroy(logger);
    log_destroy(logger_obligatorio);
    config_destroy(config);
    if (memoria_socket != -1)
        close(memoria_socket);
}