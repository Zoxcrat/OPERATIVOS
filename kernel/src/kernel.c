#include "./kernel.h"

int main(int argc, char **argv)
{
	if (argc > 2)
	{
		return EXIT_FAILURE;
	}

	config = config_create("./kernel.config");
	if (config == NULL)
	{
		log_error(logger, "No se encontró el archivo :(");
		config_destroy(config);
		exit(1);
	}
	leer_config();

	// Crear loggers con el nivel de log desde la configuración
	logger = log_create("kernel.log", "KERNEL", 1, LOG_LEVEL);
	logger_obligatorio = log_create("kernel_obligatorio.log", "kernel_obligatorio", 1, LOG_LEVEL);

	// Conexión con los módulos: Memoria y CPU (Dispatch e Interrupt)
	fd_memoria = crear_conexion2(IP_MEMORIA, PUERTO_MEMORIA);
	if (fd_memoria == -1)
	{
		log_error(logger_obligatorio, "Fallo la conexión con Memoria");
		terminar_programa();
		exit(1);
	}
	fd_cpu_dispatch = crear_conexion2(IP_CPU, PUERTO_CPU_DISPATCH);
	if (fd_cpu_dispatch == -1)
	{
		log_error(logger_obligatorio, "Fallo la conexión con CPU dispatch");
		terminar_programa();
		exit(1);
	}
	fd_cpu_interrupt = crear_conexion2(IP_CPU, PUERTO_CPU_INTERRUPT);
	if (fd_cpu_interrupt == -1)
	{
		log_error(logger_obligatorio, "Fallo la conexión con CPU interrupt");
		terminar_programa();
		exit(1);
	}

	// Mensajes iniciales de saludo a los módulos
	enviar_mensaje("Hola memoria, Soy Kernel!", fd_memoria);
	enviar_mensaje("Hola CPU Dispatch, Soy Kernel!", fd_cpu_dispatch);
	enviar_mensaje("Hola CPU Interrupt, Soy Kernel!", fd_cpu_interrupt);

	// planificar();

	terminar_programa();
	return 0;
}
// lee el archivo kernel.config
void leer_config()
{
	IP_CPU = config_get_string_value(config, "IP_CPU");
	PUERTO_CPU_INTERRUPT = config_get_string_value(config, "PUERTO_CPU_INTERRUPT");
	PUERTO_CPU_DISPATCH = config_get_string_value(config, "PUERTO_CPU_DISPATCH");
	IP_MEMORIA = config_get_string_value(config, "IP_MEMORIA");
	PUERTO_MEMORIA = config_get_string_value(config, "PUERTO_MEMORIA");
	char *algoritmo = config_get_string_value(config, "ALGORITMO_PLANIFICACION");
	asignar_algoritmo(algoritmo);
	QUANTUM = config_get_int_value(config, "QUANTUM");
	char *log_level = config_get_string_value(config, "LOG_LEVEL");
	LOG_LEVEL = log_level_from_string(log_level);
}

void asignar_algoritmo(char *algoritmo)
{
	if (strcmp(algoritmo, "FIFO") == 0)
	{
		ALGORITMO_PLANIFICACION = FIFO;
	}
	else if (strcmp(algoritmo, "PRIODIDADES") == 0)
	{
		ALGORITMO_PLANIFICACION = PRIORIDADES;
	}
	else if (strcmp(algoritmo, "COLAS MULTINIVEL") == 0)
	{
		ALGORITMO_PLANIFICACION = COLAS_MULTINIVEL;
	}
	else
	{
		log_error(logger, "El algoritmo no es valido");
	}
}

void terminar_programa(){
	log_destroy(logger);
	log_destroy(logger_obligatorio);
	config_destroy(config);
}
