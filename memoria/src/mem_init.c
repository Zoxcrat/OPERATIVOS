#include "../include/mem_init.h"

int inicializar_memoria()
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

    // Creo la memoria y la particiono segun el esquema que se indique.
    memoria_usuario = malloc(TAM_MEMORIA);
    lista_particiones = list_create();
    particionar_memoria();

    // Inicializar servidor de Memoria
    memoria_socket = iniciar_servidor(logger, IP_ESCUCHA, PUERTO_ESCUCHA);
    if (memoria_socket == -1)
    {
        log_error(logger, "Error al iniciar el servidor de la memoria");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
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
    PARTICIONES = config_get_array_value(config, "PARTICIONES");
    char *log_level = config_get_string_value(config, "LOG_LEVEL");
    LOG_LEVEL = log_level_from_string(log_level);
}

void particionar_memoria()
{
    // Solamente tengo que tomarme el trabajo de crear las particiones si el esquema es Fijo. En cualquier otro caso, la complejidad viene al asignar el espacio a los procesos.
    if (strcmp(ESQUEMA, "FIJAS") == 0)
    {
        int i = 0;
        int acumulador = 0;
        while (PARTICIONES[i])
        {
            int tamanio = atoi(PARTICIONES[i]);
            t_particion *particion = malloc(sizeof(t_particion));
            particion->base = memoria_usuario + acumulador;
            particion->tamanio = tamanio;
            particion->libre = true;
            particion->orden = i;

            log_info(logger, "PARTICION CREADA. BASE - %d / TAMANIO: %d\n / NUMERO: %d", acumulador, particion->tamanio, particion->orden);
            list_add(lista_particiones, particion);

            acumulador += tamanio;
            i++;
        }
    }
}