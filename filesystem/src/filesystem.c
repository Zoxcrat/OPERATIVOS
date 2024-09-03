#include "./filesystem.h"

int main(int argc, char **argv) {
	if (argc > 2)
	{
		return EXIT_FAILURE;
	}

	config = config_create(argv[1]);
	if (config == NULL)
	{
		log_error(logger, "No se encontró el archivo :(");
		config_destroy(config);
		exit(1);
	}
	leer_config();
    
	logger_obligatorio = log_create("filesystem.log", "filesystem_obligatorio", 1, LOG_LEVEL);
	if(config == NULL){
		log_error(logger, "No se encontró el archivo :(");
		terminar_programa();
		exit(1);
	}
	leer_config();

	// Iniciar servidor de Filesystem
	filesystem_socket = iniciar_servidor(logger, IP_ESCUCHA, PUERTO_ESCUCHA);
    if (filesystem_socket == -1) {
        log_error(logger, "Error al iniciar el servidor del Filesystem");
        return EXIT_FAILURE;
    }
	socket_cliente = esperar_cliente(filesystem_socket, logger);

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

void terminar_programa(){
	log_destroy(logger);
	log_destroy(logger_obligatorio);
	config_destroy(config);
}