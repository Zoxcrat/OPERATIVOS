#include "./cpu.h"

int main(int argc, char **argv) {
	if (argc > 2)
	{
		return EXIT_FAILURE;
	}

	logger_obligatorio = log_create("cpu_obligatorio.log", "cpu_obligatorio", 1, LOG_LEVEL);
	logger = log_create("cpu.log", "cpu", 1, LOG_LEVEL);

	config = config_create("/home/utnso/Desktop/TP/tp-2024-2c-LAMBDA/cpu/src/cpu.config");
	if (config == NULL)
	{
		log_error(logger, "No se encontró el archivo :(");
		config_destroy(config);
		exit(1);
	}
	leer_config();
    
	// Conecto CPU con memoria
	fd_memoria = crear_conexion2(IP_MEMORIA, PUERTO_MEMORIA);
	if (fd_memoria == -1)
	{
		log_error(logger_obligatorio, "Fallo la conexión con Memoria");
		terminar_programa();
		exit(1);
	}
	enviar_mensaje("Hola Memoria, soy CPU!", fd_memoria);

	// Iniciar servidor de Dispatch
	int dispatch_socket = iniciar_servidor(logger, IP_ESCUCHA, PUERTO_ESCUCHA_DISPATCH);
    if (dispatch_socket == -1) {
        log_error(logger, "Error al iniciar el servidor de dispatch");
        return EXIT_FAILURE;
    }
	socket_cliente_dispatch = esperar_cliente(dispatch_socket, logger);

    // Iniciar servidor de Interrupt
    int interrupt_socket = iniciar_servidor(logger, IP_ESCUCHA, PUERTO_ESCUCHA_INTERRUPT);
    if (interrupt_socket == -1) {
        log_error(logger, "Error al iniciar el servidor de interrupt");
        return EXIT_FAILURE;
    }
	socket_cliente_interrupt = esperar_cliente(interrupt_socket, logger);

	if (socket_cliente_dispatch == -1 || socket_cliente_interrupt == -1 ) {
		log_info(logger, "Hubo un error en la conexion del Kernel");
	}
	
	terminar_programa();
	return 0;
}

void leer_config()
{
	IP_MEMORIA = config_get_string_value(config, "IP_MEMORIA");
	PUERTO_MEMORIA = config_get_string_value(config, "PUERTO_MEMORIA");
	IP_ESCUCHA = config_get_string_value(config, "IP_ESCUCHA");
	PUERTO_ESCUCHA_DISPATCH = config_get_string_value(config, "PUERTO_CPU_DISPATCH");
	PUERTO_ESCUCHA_INTERRUPT = config_get_string_value(config, "IP_MEMORIA");
	char *log_level = config_get_string_value(config, "LOG_LEVEL");
	LOG_LEVEL = log_level_from_string(log_level);
}

void terminar_programa(){
	log_destroy(logger);
	log_destroy(logger_obligatorio);
	config_destroy(config);
}