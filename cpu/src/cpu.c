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
    inicializar_estructuras();

    interrupt_socket = -1, dispatch_socket = -1,fd_memoria = -1;
	if (!generar_conexiones()) {
		log_error(logger, "Alguna conexion falló :(");
		terminar_programa();
		exit(1);
	}

    sleep(15);

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

bool generar_conexiones(){
	fd_memoria = crear_conexion2(IP_MEMORIA, PUERTO_MEMORIA);
	pthread_create(&conexion_memoria, NULL, (void*) pedir_contexto_a_memoria, (void*) &fd_memoria);
	pthread_detach(conexion_memoria);

    dispatch_socket = iniciar_servidor(logger, IP_ESCUCHA, PUERTO_ESCUCHA_DISPATCH);
    pthread_create(&hilo_dispatch, NULL, manejar_cliente_dispatch, &dispatch_socket);
    pthread_detach(hilo_dispatch);

    interrupt_socket = iniciar_servidor(logger, IP_ESCUCHA, PUERTO_ESCUCHA_INTERRUPT);
    pthread_create(&hilo_interrupt, NULL, manejar_cliente_interrupt, &interrupt_socket);
    pthread_detach(hilo_interrupt);

    return fd_memoria != -1 && dispatch_socket != -1 && interrupt_socket != -1;
}

// Funciones para manejar conexiones
void* manejar_cliente_dispatch() {
    int cliente_socket_dispatch = esperar_cliente(dispatch_socket, logger);
    
    char *respuesta = "Mensaje a enviar";
    enviar_mensaje(respuesta, cliente_socket_dispatch);
    
    if (cliente_socket_dispatch == -1) {
        log_error(logger, "Error al aceptar cliente de dispatch");
    }
    while (1){
        t_list* paquete = recibir_paquete(cliente_socket_dispatch);

        op_code codigo_operacion = *(op_code*)list_get(paquete, 0);  // Primer elemento es el codigo_operacion
        pid_actual = atoi(list_get(paquete, 1));
        tid_actual = atoi(list_get(paquete, 2));

        sem_post(&pedir_contexto);
    }
    close(cliente_socket_dispatch);
    close(dispatch_socket);
    return NULL;
}

void* manejar_cliente_interrupt() {
    int cliente_socket_interrupt = esperar_cliente(interrupt_socket, logger);
    if (cliente_socket_interrupt == -1) {
        log_error(logger, "Error al aceptar cliente de dispatch");
    }
    while (1) {
        sem_wait(&verificar_interrupcion);
        
        int interrupcion = recibir_entero(fd_memoria);

        if (interrupcion == 1) {
            log_info(logger, "Interrupción recibida");
            interrupcion=1;
        }
    }
    close(cliente_socket_interrupt);
    close(interrupt_socket);
}

void pedir_contexto_a_memoria() {
    while (1) {
        sem_wait(&pedir_contexto);

        t_paquete* paquete = crear_paquete();
        agregar_a_paquete(paquete, &(pid_actual), sizeof(int));
        agregar_a_paquete(paquete, &(tid_actual), sizeof(int));
        enviar_peticion(paquete, fd_memoria, PEDIDO_CONTEXTO);
        eliminar_paquete(paquete);

        // Recibir el contexto de memoria
        contexto = recibir_contexto(fd_memoria);
        
        interrupcion = 0;
        cpu_cycle();
    }
    close(fd_memoria);
}

void cpu_cycle() {
    while (interrupcion == 0) {
        char* instruccion = fetch();     // Obtener la siguiente instrucción de la memoria
        decode(instruccion);             // Decodificar la instrucción
        execute();                       // Ejecutar la instrucción
        if (instruccion_actual->instruccion != SET && strcmp(instruccion_actual->parametro1, "PC") != 0){
            contexto->PC++;
        }
        sem_post(&verificar_interrupcion); // Verificar si hay interrupciones del Kernel
    }
    log_info(logger, "Ciclo de CPU interrumpido");
}

char* fetch() {
    log_info(logger, "FETCH - Solicitando instrucción PC=%d", contexto->PC);

    t_paquete* paquete = crear_paquete();
    agregar_a_paquete(paquete, &pid_actual, sizeof(int));
    agregar_a_paquete(paquete, &tid_actual, sizeof(int));
    agregar_a_paquete(paquete, &contexto->PC, sizeof(int));
    enviar_peticion(paquete,fd_memoria,PROXIMA_INSTRUCCION);
    eliminar_paquete(paquete);

    char* instruccion = recibir_mensaje2(dispatch_socket,logger);

    log_info(logger, "Instrucción recibida: %d", instruccion_actual->instruccion);
    
    return instruccion;
}

void decode(char* mensaje)
{
    // Tokenizamos el mensaje separando por espacios
    char *token = strtok(mensaje, " ");

    // La primera parte es la instrucción (convertimos la cadena al valor del enum)
    instruccion_actual->instruccion = string_a_instruccion(token);

    // Los siguientes tokens son los parámetros (si existen)
    token = strtok(NULL, " ");
    if (token != NULL) {
        instruccion_actual->parametro1 = strdup(token);
    } else {
        instruccion_actual->parametro1 = NULL;
    }

    token = strtok(NULL, " ");
    if (token != NULL) {
        instruccion_actual->parametro2 = strdup(token);
    } else {
        instruccion_actual->parametro2 = NULL;
    }

    token = strtok(NULL, " ");
    if (token != NULL) {
        instruccion_actual->parametro3 = strdup(token);
    } else {
        instruccion_actual->parametro3 = NULL;
    }
}

void execute() {
    switch (instruccion_actual->instruccion) {
        t_paquete *paquete;
        case SET:
            uint32_t valor = (uint32_t) strtoul(instruccion_actual->parametro2, NULL, 5);
            set_valor_registro(instruccion_actual->parametro1, valor);
            break;
        case READ_MEM:{
            int direccion_fis = traducir_direccion_logica_a_fisica(atoi(instruccion_actual->parametro2)); // DECODE
            log_info(logger, "READ MEM");
            if (direccion_fis != -1) { // Verificar si no hubo segmentation fault
                // Leer el valor desde la memoria física
                uint32_t valor_leido = leer_memoria(direccion_fis);

                // Asignar el valor leído al registro de datos
                set_valor_registro(instruccion_actual->parametro1, valor_leido);

                log_info(logger, "READ MEM: Leído valor %d desde la dirección física %d", valor_leido, direccion_fis);
            } else {
                free(instruccion_actual->parametro1);
                free(instruccion_actual->parametro2);
                free(instruccion_actual->parametro3);
                // Crear un paquete
                t_paquete *paquete = crear_paquete();
                enviar_peticion(paquete,dispatch_socket,SEGMENTATION_FAULT);
                eliminar_paquete(paquete);

                free(instruccion_actual);
                log_error(logger, "READ MEM: Segmentation Fault al intentar acceder a la dirección lógica %d", direccion_fis);
            }
            break;}
        case WRITE_MEM:{
            int direccion_fis = traducir_direccion_logica_a_fisica(atoi(instruccion_actual->parametro1)); // DECODE
            
            if (direccion_fis != -1) { // Verificar si no hubo segmentation fault
                // EScribir el valor desde la memoria física
                escribir_memoria(instruccion_actual->parametro2,direccion_fis);
                log_info(logger, "READ MEM: Leído escrito en la dirección física %d", direccion_fis);
            } else {
                free(instruccion_actual->parametro1);
                free(instruccion_actual->parametro2);
                free(instruccion_actual->parametro3);
                // Crear un paquete
                t_paquete *paquete = crear_paquete();
                enviar_peticion(paquete,dispatch_socket,SEGMENTATION_FAULT);
                eliminar_paquete(paquete);
                free(instruccion_actual);
                log_error(logger, "READ MEM: Segmentation Fault al intentar acceder a la dirección lógica %d", direccion_fis);
            }
            break;}
        case SUM:
            sumar_registros(instruccion_actual->parametro1, instruccion_actual->parametro2);
            break;
        case SUB:
            restar_registros(instruccion_actual->parametro1, instruccion_actual->parametro2);
            break;
        case JNZ:
            jnz_registro(instruccion_actual->parametro1, atoi(instruccion_actual->parametro2));
            break;
        case LOG:
            log_registro(instruccion_actual->parametro1);
            break;
        case PROCESS_CREATE:
            log_info(logger, "operacion PROCESS_CREATE - Notificando al Kernel");
                // Crear un paquete
            paquete= crear_paquete();
            agregar_a_paquete(paquete, instruccion_actual->parametro1, strlen(instruccion_actual->parametro1) + 1); // +1 for null terminator
            agregar_a_paquete(paquete, instruccion_actual->parametro2, strlen(instruccion_actual->parametro2) + 1);
            agregar_a_paquete(paquete, instruccion_actual->parametro3, strlen(instruccion_actual->parametro3) + 1);
            enviar_peticion(paquete,dispatch_socket,PROCESS_CREATE);
            eliminar_paquete(paquete);
            break;
        case PROCESS_EXIT:
            log_info(logger, "operacion PROCESS_EXIT - Notificando al Kernel");
            paquete= crear_paquete();
            agregar_a_paquete(paquete, instruccion_actual->parametro1, strlen(instruccion_actual->parametro1) + 1); // +1 for null terminator
            agregar_a_paquete(paquete, instruccion_actual->parametro2, strlen(instruccion_actual->parametro2) + 1);
            agregar_a_paquete(paquete, instruccion_actual->parametro3, strlen(instruccion_actual->parametro3) + 1);
            enviar_peticion(paquete,dispatch_socket,PROCESS_EXIT);
            eliminar_paquete(paquete);
            break;
        case THREAD_CREATE:
            log_info(logger, "operacion THREAD_CREATE - Notificando al Kernel");
            paquete= crear_paquete();
            agregar_a_paquete(paquete, instruccion_actual->parametro1, strlen(instruccion_actual->parametro1) + 1); // +1 for null terminator
            agregar_a_paquete(paquete, instruccion_actual->parametro2, strlen(instruccion_actual->parametro2) + 1);
            agregar_a_paquete(paquete, instruccion_actual->parametro3, strlen(instruccion_actual->parametro3) + 1);
            enviar_peticion(paquete,dispatch_socket,THREAD_CREATE);
            eliminar_paquete(paquete);
            break;
        case THREAD_JOIN:
            log_info(logger, "operacion THREAD_JOIN - Notificando al Kernel");
            paquete= crear_paquete();
            agregar_a_paquete(paquete, instruccion_actual->parametro1, strlen(instruccion_actual->parametro1) + 1); // +1 for null terminator
            agregar_a_paquete(paquete, instruccion_actual->parametro2, strlen(instruccion_actual->parametro2) + 1);
            agregar_a_paquete(paquete, instruccion_actual->parametro3, strlen(instruccion_actual->parametro3) + 1);
            enviar_peticion(paquete,dispatch_socket,THREAD_JOIN);
            eliminar_paquete(paquete);
            break;
        case THREAD_CANCEL:
            log_info(logger, "operacion THREAD_CANCEL - Notificando al Kernel");
            paquete= crear_paquete();
            agregar_a_paquete(paquete, instruccion_actual->parametro1, strlen(instruccion_actual->parametro1) + 1); // +1 for null terminator
            agregar_a_paquete(paquete, instruccion_actual->parametro2, strlen(instruccion_actual->parametro2) + 1);
            agregar_a_paquete(paquete, instruccion_actual->parametro3, strlen(instruccion_actual->parametro3) + 1);
            enviar_peticion(paquete,dispatch_socket,THREAD_CANCEL);
            eliminar_paquete(paquete);
            break;
        case THREAD_EXIT:
            log_info(logger, "operacion THREAD_EXIT - Notificando al Kernel");
            paquete= crear_paquete();
            agregar_a_paquete(paquete, instruccion_actual->parametro1, strlen(instruccion_actual->parametro1) + 1); // +1 for null terminator
            agregar_a_paquete(paquete, instruccion_actual->parametro2, strlen(instruccion_actual->parametro2) + 1);
            agregar_a_paquete(paquete, instruccion_actual->parametro3, strlen(instruccion_actual->parametro3) + 1);
            enviar_peticion(paquete,dispatch_socket,THREAD_EXIT);
            eliminar_paquete(paquete);
            break;
        case MUTEX_CREATE:
            log_info(logger, "operacion MUTEX_CREATE - Notificando al Kernel");
            paquete= crear_paquete();
            agregar_a_paquete(paquete, instruccion_actual->parametro1, strlen(instruccion_actual->parametro1) + 1); // +1 for null terminator
            agregar_a_paquete(paquete, instruccion_actual->parametro2, strlen(instruccion_actual->parametro2) + 1);
            agregar_a_paquete(paquete, instruccion_actual->parametro3, strlen(instruccion_actual->parametro3) + 1);
            enviar_peticion(paquete,dispatch_socket,MUTEX_CREATE);
            eliminar_paquete(paquete);
            break;
        case MUTEX_LOCK:
            log_info(logger, "operacion MUTEX_LOCK - Notificando al Kernel");
            paquete= crear_paquete();
            agregar_a_paquete(paquete, instruccion_actual->parametro1, strlen(instruccion_actual->parametro1) + 1); // +1 for null terminator
            agregar_a_paquete(paquete, instruccion_actual->parametro2, strlen(instruccion_actual->parametro2) + 1);
            agregar_a_paquete(paquete, instruccion_actual->parametro3, strlen(instruccion_actual->parametro3) + 1);
            enviar_peticion(paquete,dispatch_socket,MUTEX_LOCK);
            eliminar_paquete(paquete);
            break;
        case MUTEX_UNLOCK:
            log_info(logger, "operacion MUTEX_UNLOCK - Notificando al Kernel");
            paquete= crear_paquete();
            agregar_a_paquete(paquete, instruccion_actual->parametro1, strlen(instruccion_actual->parametro1) + 1); // +1 for null terminator
            agregar_a_paquete(paquete, instruccion_actual->parametro2, strlen(instruccion_actual->parametro2) + 1);
            agregar_a_paquete(paquete, instruccion_actual->parametro3, strlen(instruccion_actual->parametro3) + 1);
            enviar_peticion(paquete,dispatch_socket,MUTEX_UNLOCK);
            eliminar_paquete(paquete);
            break;
        case DUMP_MEMORY:
            log_info(logger, "operacion DUMP_MEMORY - Notificando al Kernel");
            paquete= crear_paquete();
            agregar_a_paquete(paquete, instruccion_actual->parametro1, strlen(instruccion_actual->parametro1) + 1); // +1 for null terminator
            agregar_a_paquete(paquete, instruccion_actual->parametro2, strlen(instruccion_actual->parametro2) + 1);
            agregar_a_paquete(paquete, instruccion_actual->parametro3, strlen(instruccion_actual->parametro3) + 1);
            enviar_peticion(paquete,dispatch_socket,DUMP_MEMORY);
            eliminar_paquete(paquete);
            break;
        case IO:
            log_info(logger, "operacion IO - Notificando al Kernel");
            paquete= crear_paquete();
            agregar_a_paquete(paquete, instruccion_actual->parametro1, strlen(instruccion_actual->parametro1) + 1); // +1 for null terminator
            agregar_a_paquete(paquete, instruccion_actual->parametro2, strlen(instruccion_actual->parametro2) + 1);
            agregar_a_paquete(paquete, instruccion_actual->parametro3, strlen(instruccion_actual->parametro3) + 1);
            enviar_peticion(paquete,dispatch_socket,IO);
            eliminar_paquete(paquete);
            break;
        default:
            log_warning(logger, "Instrucción no reconocida");
            break;
    }
}

op_code string_a_instruccion(char *str) {
    if (strcmp(str, "SET") == 0) return SET;
    if (strcmp(str, "READ_MEM") == 0) return READ_MEM;
    if (strcmp(str, "WRITE_MEM") == 0) return WRITE_MEM;
    if (strcmp(str, "SUM") == 0) return SUM;
    if (strcmp(str, "SUB") == 0) return SUB;
    if (strcmp(str, "JNZ") == 0) return JNZ;
    if (strcmp(str, "LOG") == 0) return LOG;
    if (strcmp(str, "PROCESS_CREATE") == 0) return PROCESS_CREATE;
    if (strcmp(str, "PROCESS_EXIT") == 0) return PROCESS_EXIT;
    if (strcmp(str, "THREAD_CREATE") == 0) return THREAD_CREATE;
    if (strcmp(str, "THREAD_JOIN") == 0) return THREAD_JOIN;
    if (strcmp(str, "THREAD_CANCEL") == 0) return THREAD_CANCEL;
    if (strcmp(str, "THREAD_EXIT") == 0) return THREAD_EXIT;
    if (strcmp(str, "MUTEX_CREATE") == 0) return MUTEX_CREATE;
    if (strcmp(str, "MUTEX_LOCK") == 0) return MUTEX_LOCK;
    if (strcmp(str, "MUTEX_UNLOCK") == 0) return MUTEX_UNLOCK;
    if (strcmp(str, "DUMP_MEMORY") == 0) return DUMP_MEMORY;
    if (strcmp(str, "IO") == 0) return IO;
    return SEGMENTATION_FAULT;  // Valor por defecto en caso de error
}

int traducir_direccion_logica_a_fisica(int direccion_logica){
    int direccion_fisica;
    if (direccion_logica < 0 || direccion_logica >= contexto->limite){
        return -1;
        }
    direccion_fisica = contexto->base + direccion_logica;
    return direccion_fisica;
}

// MANEJO INSTRUCCIONES 
void set_valor_registro(char* registro, uint32_t valor) {
    if (strcmp(registro, "PC") == 0) {
        contexto->PC = valor;
        log_info(logger, "Se seteo el valor %u en registro PC", contexto->PC);
    } else if (strcmp(registro, "AX") == 0) {
        contexto->AX = valor;
        log_info(logger, "Se seteo el valor %u en registro AX", contexto->AX);
    } else if (strcmp(registro, "BX") == 0) {
        contexto->BX = valor;
        log_info(logger, "Se seteo el valor %u en registro BX", contexto->BX);
    } else if (strcmp(registro, "CX") == 0) {
        contexto->CX = valor;
        log_info(logger, "Se seteo el valor %u en registro CX", contexto->CX);
    } else if (strcmp(registro, "DX") == 0) {
        contexto->DX = valor;
        log_info(logger, "Se seteo el valor %u en registro DX", contexto->DX);
    } else if (strcmp(registro, "EX") == 0) {
        contexto->EX = valor;
        log_info(logger, "Se seteo el valor %u en registro EX", contexto->EX);
    } else if (strcmp(registro, "FX") == 0) {
        contexto->FX = valor;
        log_info(logger, "Se seteo el valor %u en registro FX", contexto->FX);
    } else if (strcmp(registro, "GX") == 0) {
        contexto->GX = valor;
        log_info(logger, "Se seteo el valor %u en registro GX", contexto->GX);
    } else if (strcmp(registro, "HX") == 0) {
        contexto->HX = valor;
        log_info(logger, "Se seteo el valor %u en registro HX", contexto->HX);
    } else if (strcmp(registro, "base") == 0) {
        contexto->base = valor;
        log_info(logger, "Se seteo el valor %u en registro base", contexto->base);
    } else if (strcmp(registro, "limite") == 0) {
        contexto->limite = valor;
        log_info(logger, "Se seteo el valor %u en registro limite", contexto->limite);
    } else {
        log_info(logger, "Registro %s no reconocido", registro);
    }
}

void sumar_registros(char* destino, char* origen) {
    uint32_t valor_origen = 0;

    // Obtener valor del registro origen
    if (strcmp(origen, "PC") == 0) {
        valor_origen = contexto->PC;
    } else if (strcmp(origen, "AX") == 0) {
        valor_origen = contexto->AX;
    } else if (strcmp(origen, "BX") == 0) {
        valor_origen = contexto->BX;
    } else if (strcmp(origen, "CX") == 0) {
        valor_origen = contexto->CX;
    } else if (strcmp(origen, "DX") == 0) {
        valor_origen = contexto->DX;
    } else if (strcmp(origen, "EX") == 0) {
        valor_origen = contexto->EX;
    } else if (strcmp(origen, "FX") == 0) {
        valor_origen = contexto->FX;
    } else if (strcmp(origen, "GX") == 0) {
        valor_origen = contexto->GX;
    } else if (strcmp(origen, "HX") == 0) {
        valor_origen = contexto->HX;
    } else if (strcmp(origen, "base") == 0) {
        valor_origen = contexto->base;
    } else if (strcmp(origen, "limite") == 0) {
        valor_origen = contexto->limite;
    } else {
        log_info(logger, "Registro de origen %s no reconocido", origen);
        return;
    }

    // Obtener valor del registro destino y sumar
    if (strcmp(destino, "PC") == 0) {
        contexto->PC += valor_origen;
        log_info(logger, "Se sumo el valor %u de %s en registro PC. Nuevo valor: %u", valor_origen, origen, contexto->PC);
    } else if (strcmp(destino, "AX") == 0) {
        contexto->AX += valor_origen;
        log_info(logger, "Se sumo el valor %u de %s en registro AX. Nuevo valor: %u", valor_origen, origen, contexto->AX);
    } else if (strcmp(destino, "BX") == 0) {
        contexto->BX += valor_origen;
        log_info(logger, "Se sumo el valor %u de %s en registro BX. Nuevo valor: %u", valor_origen, origen, contexto->BX);
    } else if (strcmp(destino, "CX") == 0) {
        contexto->CX += valor_origen;
        log_info(logger, "Se sumo el valor %u de %s en registro CX. Nuevo valor: %u", valor_origen, origen, contexto->CX);
    } else if (strcmp(destino, "DX") == 0) {
        contexto->DX += valor_origen;
        log_info(logger, "Se sumo el valor %u de %s en registro DX. Nuevo valor: %u", valor_origen, origen, contexto->DX);
    } else if (strcmp(destino, "EX") == 0) {
        contexto->EX += valor_origen;
        log_info(logger, "Se sumo el valor %u de %s en registro EX. Nuevo valor: %u", valor_origen, origen, contexto->EX);
    } else if (strcmp(destino, "FX") == 0) {
        contexto->FX += valor_origen;
        log_info(logger, "Se sumo el valor %u de %s en registro FX. Nuevo valor: %u", valor_origen, origen, contexto->FX);
    } else if (strcmp(destino, "GX") == 0) {
        contexto->GX += valor_origen;
        log_info(logger, "Se sumo el valor %u de %s en registro GX. Nuevo valor: %u", valor_origen, origen, contexto->GX);
    } else if (strcmp(destino, "HX") == 0) {
        contexto->HX += valor_origen;
        log_info(logger, "Se sumo el valor %u de %s en registro HX. Nuevo valor: %u", valor_origen, origen, contexto->HX);
    } else if (strcmp(destino, "base") == 0) {
        contexto->base += valor_origen;
        log_info(logger, "Se sumo el valor %u de %s en registro base. Nuevo valor: %u", valor_origen, origen, contexto->base);
    } else if (strcmp(destino, "limite") == 0) {
        contexto->limite += valor_origen;
        log_info(logger, "Se sumo el valor %u de %s en registro limite. Nuevo valor: %u", valor_origen, origen, contexto->limite);
    } else {
        log_info(logger, "Registro destino %s no reconocido", destino);
    }
}

void restar_registros(char* destino, char* origen) {
    uint32_t valor_origen = 0;

    // Obtener valor del registro origen
    if (strcmp(origen, "PC") == 0) {
        valor_origen = contexto->PC;
    } else if (strcmp(origen, "AX") == 0) {
        valor_origen = contexto->AX;
    } else if (strcmp(origen, "BX") == 0) {
        valor_origen = contexto->BX;
    } else if (strcmp(origen, "CX") == 0) {
        valor_origen = contexto->CX;
    } else if (strcmp(origen, "DX") == 0) {
        valor_origen = contexto->DX;
    } else if (strcmp(origen, "EX") == 0) {
        valor_origen = contexto->EX;
    } else if (strcmp(origen, "FX") == 0) {
        valor_origen = contexto->FX;
    } else if (strcmp(origen, "GX") == 0) {
        valor_origen = contexto->GX;
    } else if (strcmp(origen, "HX") == 0) {
        valor_origen = contexto->HX;
    } else if (strcmp(origen, "base") == 0) {
        valor_origen = contexto->base;
    } else if (strcmp(origen, "limite") == 0) {
        valor_origen = contexto->limite;
    } else {
        log_info(logger, "Registro de origen %s no reconocido", origen);
        return;
    }

    // Obtener valor del registro destino y restar
    if (strcmp(destino, "PC") == 0) {
        contexto->PC -= valor_origen;
        log_info(logger, "Se restó el valor %u de %s en registro PC. Nuevo valor: %u", valor_origen, origen, contexto->PC);
    } else if (strcmp(destino, "AX") == 0) {
        contexto->AX -= valor_origen;
        log_info(logger, "Se restó el valor %u de %s en registro AX. Nuevo valor: %u", valor_origen, origen, contexto->AX);
    } else if (strcmp(destino, "BX") == 0) {
        contexto->BX -= valor_origen;
        log_info(logger, "Se restó el valor %u de %s en registro BX. Nuevo valor: %u", valor_origen, origen, contexto->BX);
    } else if (strcmp(destino, "CX") == 0) {
        contexto->CX -= valor_origen;
        log_info(logger, "Se restó el valor %u de %s en registro CX. Nuevo valor: %u", valor_origen, origen, contexto->CX);
    } else if (strcmp(destino, "DX") == 0) {
        contexto->DX -= valor_origen;
        log_info(logger, "Se restó el valor %u de %s en registro DX. Nuevo valor: %u", valor_origen, origen, contexto->DX);
    } else if (strcmp(destino, "EX") == 0) {
        contexto->EX -= valor_origen;
        log_info(logger, "Se restó el valor %u de %s en registro EX. Nuevo valor: %u", valor_origen, origen, contexto->EX);
    } else if (strcmp(destino, "FX") == 0) {
        contexto->FX -= valor_origen;
        log_info(logger, "Se restó el valor %u de %s en registro FX. Nuevo valor: %u", valor_origen, origen, contexto->FX);
    } else if (strcmp(destino, "GX") == 0) {
        contexto->GX -= valor_origen;
        log_info(logger, "Se restó el valor %u de %s en registro GX. Nuevo valor: %u", valor_origen, origen, contexto->GX);
    } else if (strcmp(destino, "HX") == 0) {
        contexto->HX -= valor_origen;
        log_info(logger, "Se restó el valor %u de %s en registro HX. Nuevo valor: %u", valor_origen, origen, contexto->HX);
    } else if (strcmp(destino, "base") == 0) {
        contexto->base -= valor_origen;
        log_info(logger, "Se restó el valor %u de %s en registro base. Nuevo valor: %u", valor_origen, origen, contexto->base);
    } else if (strcmp(destino, "limite") == 0) {
        contexto->limite -= valor_origen;
        log_info(logger, "Se restó el valor %u de %s en registro limite. Nuevo valor: %u", valor_origen, origen, contexto->limite);
    } else {
        log_info(logger, "Registro destino %s no reconocido", destino);
    }
}

void jnz_registro(char* registro, uint32_t instruccion) {
    uint32_t valor_registro = 0;

    // Obtener el valor del registro
    if (strcmp(registro, "PC") == 0) {
        valor_registro = contexto->PC;
    } else if (strcmp(registro, "AX") == 0) {
        valor_registro = contexto->AX;
    } else if (strcmp(registro, "BX") == 0) {
        valor_registro = contexto->BX;
    } else if (strcmp(registro, "CX") == 0) {
        valor_registro = contexto->CX;
    } else if (strcmp(registro, "DX") == 0) {
        valor_registro = contexto->DX;
    } else if (strcmp(registro, "EX") == 0) {
        valor_registro = contexto->EX;
    } else if (strcmp(registro, "FX") == 0) {
        valor_registro = contexto->FX;
    } else if (strcmp(registro, "GX") == 0) {
        valor_registro = contexto->GX;
    } else if (strcmp(registro, "HX") == 0) {
        valor_registro = contexto->HX;
    } else if (strcmp(registro, "base") == 0) {
        valor_registro = contexto->base;
    } else if (strcmp(registro, "limite") == 0) {
        valor_registro = contexto->limite;
    } else {
        log_info(logger, "Registro %s no reconocido", registro);
        return;
    }

    // Si el valor del registro es distinto de 0, actualiza el PC
    if (valor_registro != 0) {
        contexto->PC = instruccion;
        log_info(logger, "Se actualizó el PC al valor %u porque el registro %s es distinto de 0", instruccion, registro);
    } else {
        log_info(logger, "El valor del registro %s es 0, no se actualiza el PC", registro);
    }
}

void log_registro(char* registro) {
    uint32_t valor_registro = 0;

    // Obtener el valor del registro
    if (strcmp(registro, "PC") == 0) {
        valor_registro = contexto->PC;
    } else if (strcmp(registro, "AX") == 0) {
        valor_registro = contexto->AX;
    } else if (strcmp(registro, "BX") == 0) {
        valor_registro = contexto->BX;
    } else if (strcmp(registro, "CX") == 0) {
        valor_registro = contexto->CX;
    } else if (strcmp(registro, "DX") == 0) {
        valor_registro = contexto->DX;
    } else if (strcmp(registro, "EX") == 0) {
        valor_registro = contexto->EX;
    } else if (strcmp(registro, "FX") == 0) {
        valor_registro = contexto->FX;
    } else if (strcmp(registro, "GX") == 0) {
        valor_registro = contexto->GX;
    } else if (strcmp(registro, "HX") == 0) {
        valor_registro = contexto->HX;
    } else if (strcmp(registro, "base") == 0) {
        valor_registro = contexto->base;
    } else if (strcmp(registro, "limite") == 0) {
        valor_registro = contexto->limite;
    } else {
        log_info(logger, "Registro %s no reconocido", registro);
        return;
    }

    // Escribir el valor del registro en el archivo de log
    log_info(logger, "El valor del registro %s es %u", registro, valor_registro);
}

uint32_t leer_memoria(int direccion_fisica){
    t_paquete* paquete = crear_paquete();
    agregar_a_paquete(paquete, &direccion_fisica, sizeof(int));
    enviar_peticion(paquete, fd_memoria, LEER_MEMORIA);
    eliminar_paquete(paquete);

    uint32_t valor_leido = recibir_entero(fd_memoria);
    return valor_leido;
}

void escribir_memoria(char* registro_datos,int direccion_fisica){
    t_paquete* paquete = crear_paquete();
    agregar_a_paquete(paquete, &registro_datos, sizeof(char));
    agregar_a_paquete(paquete, &direccion_fisica, sizeof(int));
    enviar_peticion(paquete, fd_memoria, ESCRIBIR_MEMORIA);
    eliminar_paquete(paquete);

    int respuesta = recibir_entero(fd_memoria);
}

// variables del programa

void inicializar_estructuras()
{
    contexto = malloc(sizeof(t_contexto_ejecucion));
    instruccion_actual = malloc(sizeof(t_instruccion_completa));
    sem_init(&pedir_contexto, 0, 0);
    sem_init(&verificar_interrupcion, 0, 0);
    interrupcion = 0;

    conexion_memoria = malloc(sizeof(pthread_t));
    hilo_dispatch = malloc(sizeof(pthread_t));
    hilo_interrupt = malloc(sizeof(pthread_t));
}

void terminar_programa(){
	log_destroy(logger);
	log_destroy(logger_obligatorio);
	config_destroy(config);
    sem_destroy(&pedir_contexto);
    sem_destroy(&verificar_interrupcion);
}
