#include "../include/memoria.h"

void inicializar_memoria()
{
    // Inicializar logs
    logger_obligatorio = log_create("memoria_obligatorio.log", "memoria_obligatorio", 1, LOG_LEVEL);
    logger = log_create("memoria.log", "memoria", 1, LOG_LEVEL);

    // Inicializar configs
    config = config_create("src/memoria.config");
    if (config == NULL)
    {
        log_error(logger, "No se encontró el archivo de configuracion.");
        config_destroy(config);
        exit(1);
    }
    leer_config();

    memoria_usuario = malloc(TAM_MEMORIA);
    lista_particiones = list_create();

    // Inicializar servidor de Memoria
    memoria_socket = iniciar_servidor(logger, IP_ESCUCHA, PUERTO_ESCUCHA);
    if (memoria_socket == -1)
    {
        log_error(logger, "Error al iniciar el servidor de la memoria");
        return EXIT_FAILURE;
    }
}

void leer_config()
{
    IP_ESCUCHA = config_get_string_value(config, "IP_ESCUCHA");
    PUERTO_ESCUCHA = config_get_string_value(config, "PUERTO_ESCUCHA");
    IP_FILESYSTEM = config_get_string_value(config, "IP_FILESYSTEM");
    PUERTO_FILESYSTEM = config_get_string_value(config, "PUERTO_FILESYSTEM");
    TAM_MEMORIA = config_get_int_value(config, "TAM_MEMORIA");
    PATH_INSTRUCCIONES = config_get_string_value(config, "PATH_INSTRUCCIONES");
    RETARDO_RESPUESTA = config_get_int_value(config, "RETARDO_RESPUESTA");
    ESQUEMA = config_get_string_value(config, "ESQUEMA");
    ALGORITMO_BUSQUEDA = config_get_string_value(config, "ALGORITMO_BUSQUEDA");
    PARTICIONES = config_get_string_value(config, "PARTICIONES"); // REVISAR
    char *log_level = config_get_string_value(config, "LOG_LEVEL");
    LOG_LEVEL = log_level_from_string(log_level);
}