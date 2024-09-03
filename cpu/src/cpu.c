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

	// Iniciar servidor de Dispatch
	int dispatch_socket = iniciar_servidor(logger, IP_ESCUCHA, PUERTO_ESCUCHA_DISPATCH);
    if (dispatch_socket == -1) {
        log_error(logger, "Error al iniciar el servidor de dispatch");
        return EXIT_FAILURE;
    }
    // Iniciar servidor de Interrupt
    int interrupt_socket = iniciar_servidor(logger, IP_ESCUCHA, PUERTO_ESCUCHA_INTERRUPT);
    if (interrupt_socket == -1) {
        log_error(logger, "Error al iniciar el servidor de interrupt");
        return EXIT_FAILURE;
    }
	//////////////
	while (1) {
        int socket_cliente_dispatch = esperar_cliente(dispatch_socket, logger);
        if (socket_cliente_dispatch != -1) {
            pthread_t hilo_dispatch;
            if (pthread_create(&hilo_dispatch, NULL, manejar_cliente_dispatch, (void*)socket_cliente_dispatch) != 0) {
                log_error(logger, "Error al crear el hilo para dispatch");
                close(socket_cliente_dispatch);
            } else {
                pthread_detach(hilo_dispatch);
            }
        }

        int socket_cliente_interrupt = esperar_cliente(interrupt_socket, logger);
        if (socket_cliente_interrupt != -1) {
            pthread_t hilo_interrupt;
            if (pthread_create(&hilo_interrupt, NULL, manejar_cliente_interrupt, (void*)socket_cliente_interrupt) != 0) {
                log_error(logger, "Error al crear el hilo para interrupt");
                close(socket_cliente_interrupt);
            } else {
                pthread_detach(hilo_interrupt);
            }
        }
    }
	terminar_programa();
	return 0;
}

void leer_config()
{
	IP_MEMORIA = config_get_string_value(config, "IP_MEMORIA");
	PUERTO_MEMORIA = config_get_string_value(config, "PUERTO_MEMORIA");
	IP_ESCUCHA = config_get_string_value(config, "IP_ESCUCHA");
	PUERTO_ESCUCHA_DISPATCH = config_get_string_value(config, "PUERTO_ESCUCHA_DISPATCH");
	PUERTO_ESCUCHA_INTERRUPT = config_get_string_value(config, "PUERTO_ESCUCHA_INTERRUPT");
	char *log_level = config_get_string_value(config, "LOG_LEVEL");
	LOG_LEVEL = log_level_from_string(log_level);
}
// Funciones para manejar conexiones
void* manejar_cliente_dispatch(void* socket_cliente) {
    int socket = (int)socket_cliente;
    // Maneja la petición del cliente dispatch aquí
    // ...

    close(socket);
    return NULL;
}

void* manejar_cliente_interrupt(void* socket_cliente) {
    int socket = (int)socket_cliente;
    // Maneja la petición del cliente interrupt aquí
    // ...

    close(socket);
    return NULL;
}
void terminar_programa(){
	log_destroy(logger);
	log_destroy(logger_obligatorio);
	config_destroy(config);
}