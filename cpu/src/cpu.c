#include "./cpu.h"

int main(int argc, char **argv) {
	if (argc > 2)
	{
		return EXIT_FAILURE;
	}

	logger_obligatorio = log_create("cpu_obligatorio.log", "cpu_obligatorio", 1, LOG_LEVEL);
	logger = log_create("cpu.log", "cpu", 1, LOG_LEVEL);

	config = config_create("src/cpu.config");
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

/*
CPU_Registers registers;

typedef enum {
    SET, SUM, SUB, READ_MEM, WRITE_MEM, LOG_REG, MUTEX_CREATE, MUTEX_LOCK, MUTEX_UNLOCK, PROCESS_CREATE, THREAD_CREATE
} Instruction;

void cpu_cycle() {
    while (1) {
        fetch();        // Obtener la siguiente instrucción de la memoria
        decode();       // Decodificar la instrucción
        execute();      // Ejecutar la instrucción
        check_interrupt(); // Verificar si hay interrupciones del Kernel
    }
}

void fetch() {
    log_info(logger, "FETCH - Solicitando instrucción en PC=%d", registers.PC);

    // Enviar solicitud de instrucción a Memoria
    send(memory_socket, &registers.PC, sizeof(registers.PC), 0);
    
    // Recibir la instrucción desde Memoria
    Instruction instruction;
    recv(memory_socket, &instruction, sizeof(Instruction), 0);

    log_info(logger, "Instrucción recibida: %d", instruction);
    
    // Actualizar el PC para la siguiente instrucción
    registers.PC++;
}

void decode() {
    log_info(logger, "DECODE - Decodificando instrucción...");
    // Aquí se puede manejar la lógica de decodificación
}

// Execute: Ejecutar la instrucción decodificada
void execute() {
    Instruction instruction;
    recv(memory_socket, &instruction, sizeof(Instruction), 0);

    switch (instruction) {
        case SET:
            registers.AX = 10; // Ejemplo de SET
            log_info(logger, "SET AX = 10");
            break;
        case SUM:
            registers.AX += registers.BX; // Ejemplo de SUM
            log_info(logger, "SUM AX = AX + BX");
            break;
        case SUB:
            registers.AX -= registers.BX; // Ejemplo de SUB
            log_info(logger, "SUB AX = AX - BX");
            break;
        case READ_MEM:
            // Enviar solicitud de lectura a Memoria
            uint32_t address = registers.AX;
            send(memory_socket, &address, sizeof(address), 0);
            
            // Recibir valor desde Memoria
            uint32_t value;
            recv(memory_socket, &value, sizeof(value), 0);
            registers.BX = value; // Almacenar en BX
            log_info(logger, "READ_MEM - Dirección: %d, Valor: %d", address, value);
            break;
        case WRITE_MEM:
            // Escribir en Memoria
            uint32_t address_to_write = registers.AX;
            uint32_t value_to_write = registers.BX;
            send(memory_socket, &address_to_write, sizeof(address_to_write), 0);
            send(memory_socket, &value_to_write, sizeof(value_to_write), 0);
            log_info(logger, "WRITE_MEM - Dirección: %d, Valor: %d", address_to_write, value_to_write);
            break;
        case PROCESS_CREATE:
            log_info(logger, "Syscall PROCESS_CREATE - Notificando al Kernel");
            send(kernel_socket, &instruction, sizeof(instruction), 0);
            break;
        case MUTEX_LOCK:
            log_info(logger, "Syscall MUTEX_LOCK - Notificando al Kernel");
            send(kernel_socket, &instruction, sizeof(instruction), 0);
            break;
        default:
            log_warning(logger, "Instrucción no reconocida");
            break;
    }
}


void check_interrupt() {
    int interrupt_signal;
    recv(kernel_socket, &interrupt_signal, sizeof(int), MSG_DONTWAIT); // Leer sin bloquear

    if (interrupt_signal > 0) {
        log_info(logger, "Interrupción recibida: %d", interrupt_signal);
        // Procesar la interrupción (ej. cambiar de contexto)
    }
}
*/