#include "./memoria.h"

int main(int argc, char **argv) {
	if (argc > 2)
	{
		return EXIT_FAILURE;
	}

	logger_obligatorio = log_create("memoria_obligatorio.log", "memoria_obligatorio", 1, LOG_LEVEL);
	logger = log_create("memoria.log", "memoria", 1, LOG_LEVEL);

	config = config_create("src/memoria.config");
	if (config == NULL)
	{
		log_error(logger, "No se encontró el archivo :(");
		config_destroy(config);
		exit(1);
	}
	leer_config();

	// Iniciar servidor de Memoria 
	memoria_socket = iniciar_servidor(logger, IP_ESCUCHA, PUERTO_ESCUCHA);
    if (memoria_socket == -1) {
        log_error(logger, "Error al iniciar el servidor de la memoria");
        return EXIT_FAILURE;
    }

	// Esperar conexión del módulo CPU
	int fd_cpu = esperar_cliente(memoria_socket, logger);
    if (fd_cpu == -1) {
        log_error(logger, "Error al conectar con el módulo CPU");
        return EXIT_FAILURE;
    }
	log_info(logger, "Conexión con CPU establecida");
	
    while (1) {
        pthread_t hilo_cliente;
        int socket_cliente = esperar_cliente(memoria_socket, logger);
        if (socket_cliente == -1) {
            log_info(logger, "Hubo un error en la conexión del cliente");
            continue;
        }
        pthread_create(&hilo_cliente, NULL, (void*) procesar_conexion, (void*) (intptr_t) socket_cliente);
        pthread_detach(hilo_cliente);
    }

	terminar_programa();
	return 0;
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
	PARTICIONES = config_get_string_value(config, "PARTICIONES"); //REVISAR
	char *log_level = config_get_string_value(config, "LOG_LEVEL");
	LOG_LEVEL = log_level_from_string(log_level);
}

void* procesar_conexion(void* arg) {
    int socket_cliente = (int) (intptr_t) arg;
    op_code cop;

    while (recv(socket_cliente, &cop, sizeof(op_code), 0) == sizeof(op_code)) {
        pthread_t hilo_peticion;
        pthread_create(&hilo_peticion, NULL, procesar_peticion, (void*) (intptr_t) socket_cliente);
        pthread_detach(hilo_peticion);
    }

    log_info(logger, "El cliente se desconectó del servidor");
    close(socket_cliente);
    return NULL;
}

void* procesar_peticion(void* arg) {
    int socket_cliente = (int) (intptr_t) arg;
    
    // Lógica para procesar la petición, ejemplo:
    // op_code cop;
    // recv(socket_cliente, &cop, sizeof(op_code), 0);

    // Conectar con FileSystem para cada petición
    int fd_filesystem = crear_conexion2(IP_FILESYSTEM, PUERTO_FILESYSTEM);
    if (fd_filesystem == -1) {
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

void terminar_programa(){
	log_destroy(logger);
	log_destroy(logger_obligatorio);
	config_destroy(config);
	if (memoria_socket != -1) close(memoria_socket);
}