#include "./filesystem.h"

int main(int argc, char **argv)
{
    if (argc > 2)
    {
        return EXIT_FAILURE;
    }

    logger_obligatorio = log_create("filesystem_obligatorio.log", "filesystem_obligatorio", 1, LOG_LEVEL);
    logger = log_create("filesystem.log", "filesystem", 1, LOG_LEVEL);

    config = config_create("src/filesystem.config");
    if (config == NULL)
    {
        log_error(logger, "No se encontró el archivo :");
        exit(1);
    }
    leer_config();

    inicializar_filesystem();

    filesystem_socket = iniciar_servidor(logger, IP_ESCUCHA, PUERTO_ESCUCHA);
    if (filesystem_socket == -1)
    {
        log_error(logger, "Error al iniciar el servidor del Filesystem");
        return EXIT_FAILURE;
    }

    while (1)
    {
        pthread_t hilo_cliente;
        int socket_cliente = esperar_cliente(filesystem_socket, logger);
        if (socket_cliente == -1)
        {
            log_info(logger, "Hubo un error en la conexion del cliente");
            continue;
        }
        pthread_create(&hilo_cliente, NULL, procesar_conexion, (void *)(intptr_t)socket_cliente);
        pthread_detach(hilo_cliente);
    }

    terminar_programa();
    return 0;
}

void leer_config()
{
    IP_ESCUCHA = config_get_string_value(config, "IP_ESCUCHA");
    PUERTO_ESCUCHA = config_get_string_value(config, "PUERTO_ESCUCHA");
    MOUNT_DIR = config_get_string_value(config, "MOUNT_DIR");
    BLOCK_SIZE = config_get_int_value(config, "BLOCK_SIZE");
    BLOCK_COUNT = config_get_int_value(config, "BLOCK_COUNT");
    RETARDO_ACCESO_BLOQUE = config_get_int_value(config, "RETARDO_ACCESO_BLOQUE");
    char *log_level = config_get_string_value(config, "LOG_LEVEL");
    LOG_LEVEL = log_level_from_string(log_level);
}

void *procesar_conexion(void *arg)
{
    int socket_cliente = (int)(intptr_t)arg;

    pthread_t hilo_peticion;
    pthread_create(&hilo_peticion, NULL, procesar_peticion, (void *)(intptr_t)socket_cliente);
    pthread_detach(hilo_peticion);

    log_info(logger, "El Hilo se desconecto del filesystem");
    close(socket_cliente);
    return NULL;
}

void *procesar_peticion(void *arg)
{
    int socket_cliente = (int)(intptr_t)arg;

    // sacar datos de buffer
    t_buffer buffer *int pid = recibir_entero(socket_cliente) int tid = recibir_entero(socket_cliente) int tamanio = recibir_entero(socket_cliente) char *contenido = malloc(tamanio) if (contenido == NULL)
    {
        enviar_mensaje("error", socket_cliente)
    }

    char *nombre_archivo = generar_nombre_archivo(pid, tid) if (nombre_archivo == NULL)
    {
        log_error(logger, "Error al generar el nombre del archivo.");
        free(contenido);
        enviar_mensaje("error", socket_cliente);
        return NULL;
    }

    int respuesta = crear_archivo_dump(nombre_archivo, contenido, tamanio_archivo)

        if (respuesta == 0)
    {
        enviar_mensaje("OK", socket_cliente);
    }
    else
    {
        enviar_mensaje("error", socket_cliente);
    }

    free(contenido) return NULL;
}

void terminar_programa()
{
    log_destroy(logger);
    log_destroy(logger_obligatorio);
    config_destroy(config);
    if (filesystem_socket != -1)
        close(filesystem_socket);
}