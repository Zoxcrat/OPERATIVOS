#include "./main.h"

// Variables config
t_log* logger_obligatorio;
t_log* logger;
t_config* config;
int fd_memoria;
int fd_cpu_interrupt;
int fd_cpu_dispatch;

// Variables del config (Las pongo aca asi no estamos revoleando el cfg para todos lados)
char* IP_CPU;
char* PUERTO_CPU_DISPATCH;
char* PUERTO_CPU_INTERRUPT;
char* IP_MEMORIA;
char* PUERTO_MEMORIA;
t_algoritmo ALGORITMO_PLANIFICACION;
int QUANTUM;
t_log_level LOG_LEVEL;

// Variables PCBs
int pid_global;
t_list* procesos_sistema;
t_list* hilos_sistema;
t_list* cola_new;
t_list* cola_ready;
t_list* cola_ready_multinivel;
bool io_en_uso; // Estado que indica si el I/O está en uso
t_list* cola_io;
t_list* cola_joins;
t_list* cola_dump_memory; 
TCB* hilo_en_exec;
TCB* hilo_inicial;
int pid_a_buscar;
int tid_a_buscar;
bool hilo_desalojado;
int interrupcion;  

// Semaforos
sem_t verificar_cola_new;
sem_t hay_hilos_en_ready;
sem_t hay_hilos_en_io;
sem_t hay_hilos_en_dump_memory;
sem_t sem_io_mutex;
sem_t mandar_interrupcion;
sem_t comenzar_quantum;

// Hilos
pthread_t* planificador_largo_plazo;
pthread_t* planificador_corto_plazo;
pthread_t* hilo_gestor_io;
pthread_t* hilo_gestor_dump_memory;
pthread_t* hilo_gestor_quantum;
pthread_t* conexion_cpu_dispatch;
pthread_t* conexion_cpu_interrupt;

// Mutexs
pthread_mutex_t mutex_procesos_en_new;
pthread_mutex_t mutex_procesos_sistema;
pthread_mutex_t mutex_hilos_sistema;
pthread_mutex_t mutex_cola_ready;
pthread_mutex_t mutex_colas_multinivel;
pthread_mutex_t mutex_cola_io;
pthread_mutex_t mutex_log;
pthread_mutex_t mutex_socket_dispatch;
pthread_mutex_t mutex_socket_interrupt;
pthread_mutex_t mutex_hilo_exec;
pthread_mutex_t mutex_cola_join_wait;
pthread_mutex_t mutex_cola_dump_memory;

int main(int argc, char **argv)
{
    if (argc > 3)
    {
        printf("Uso: %s [archivo_pseudocodigo] [tamanio_proceso]\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Inicializar proceso con argumentos
    char* archivo_pseudocodigo = argv[1];
    int tamanio_proceso = atoi(argv[2]);

    // Crear loggers con el nivel de log desde la configuración
    logger = log_create("kernel.log", "kernel", 1, LOG_LEVEL);
    logger_obligatorio = log_create("kernel_obligatorio.log", "kernel_obligatorio", 1, LOG_LEVEL);

    config = config_create("src/kernel.config");
    if (config == NULL)
    {
        log_error(logger, "No se encontró el archivo :(");
        config_destroy(config);
        exit(1);
    }
    leer_config();

    inicializar_estructuras();

    //conecto con CPU Dispatch y CPU interrupt
    fd_cpu_dispatch = -1, fd_cpu_interrupt = -1,fd_memoria = -1;
	if (!generar_conexiones()) {
		log_error(logger, "Alguna conexion falló :(");
		terminar_programa();
		exit(1);
	}
    // Mensajes iniciales de saludo a los módulos
    enviar_mensaje("Hola CPU interrupt, Soy Kernel!", fd_cpu_interrupt);
    enviar_mensaje("Hola CPU dispatcher, Soy Kernel!", fd_cpu_dispatch);

    //inicializar_plani_largo_plazo();
    //inicializar_plani_corto_plazo();
    //inicializar_gestor_io();
    //inicializar_gestor_dump_memory();

    crear_proceso(archivo_pseudocodigo, tamanio_proceso, 0);

    sleep(15);

    terminar_programa();
    return 0;
}

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
    else if (strcmp(algoritmo, "PRIORIDADES") == 0)
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

bool generar_conexiones(){
    // Conexión con CPU (Dispatch e Interrupt)
    pthread_t conexion_cpu_interrupt;
	pthread_t conexion_cpu_dispatch;

	fd_cpu_interrupt = crear_conexion2(IP_CPU, PUERTO_CPU_INTERRUPT);
	pthread_create(&conexion_cpu_interrupt, NULL, (void*) procesar_conexion_cpu_interrupt, (void*) &fd_cpu_interrupt);
	pthread_detach(conexion_cpu_interrupt);

	fd_cpu_dispatch = crear_conexion2(IP_CPU, PUERTO_CPU_DISPATCH);
	pthread_create(&conexion_cpu_dispatch, NULL, (void*) procesar_conexion_cpu_dispatch, (void*) &fd_cpu_dispatch);
	pthread_detach(conexion_cpu_dispatch);

	return fd_cpu_interrupt != -1 && fd_cpu_dispatch != -1;
}

void procesar_conexion_cpu_dispatch() {
    while (1) {
        t_instruccion_completa* syscall = malloc(sizeof(t_instruccion_completa));

        t_list* paquete = recibir_paquete(fd_cpu_dispatch);

        int cantidad_parametros = *(int*)list_get(paquete, list_size(paquete) - 1);

        // Obtener el código de operación
        syscall->instruccion = *(op_code*)list_get(paquete, 0);  // Primer elemento es el codigo_operacion

        // Inicializar los parámetros a valores por defecto
        syscall->parametro1 = NULL;
        syscall->parametro2 = NULL;
        syscall->parametro3 = NULL;

        // Cargar los parámetros según la cantidad recibida
        if (cantidad_parametros >= 1) {
            syscall->parametro1 = list_get(paquete, 1);
        }
        if (cantidad_parametros >= 2) {
            syscall->parametro2 = list_get(paquete, 2);
        }
        if (cantidad_parametros >= 3) {
            syscall->parametro3 = list_get(paquete, 3);
        }

        // Procesar la syscall según el código recibido
        switch (syscall->instruccion) {
            case PROCESS_CREATE: {
                pthread_mutex_lock(&mutex_log);
                log_info(logger_obligatorio, "## (%d:%d) - Solicitó syscall: PROCESS_CREATE", hilo_en_exec->PID,hilo_en_exec->TID);
                pthread_mutex_unlock(&mutex_log);

                crear_proceso(syscall->parametro1, atoi(syscall->parametro2), atoi(syscall->parametro3));
                free(syscall->parametro1);
                free(syscall);

                interrupcion = 0;
                sem_post(&mandar_interrupcion);
            }
            case PROCESS_EXIT: {
                pthread_mutex_lock(&mutex_log);
                log_info(logger_obligatorio, "## (%d:%d) - Solicitó syscall: PROCESS_EXIT", hilo_en_exec->PID,hilo_en_exec->TID);
                pthread_mutex_unlock(&mutex_log);
                
                finalizar_proceso(hilo_en_exec->PID);
                
                hilo_en_exec = NULL;
                interrupcion = 1;
                sem_post(&mandar_interrupcion);
                sem_post(&hay_hilos_en_ready);
                break;
            }
            case THREAD_CREATE: {
                // Recibir los parámetros específicos de THREAD_CREATE
                pthread_mutex_lock(&mutex_log);
                log_info(logger_obligatorio, "## (%d:%d) - Solicitó syscall: THREAD_CREATE", hilo_en_exec->PID,hilo_en_exec->TID);
                pthread_mutex_unlock(&mutex_log);

                pthread_mutex_lock(&mutex_procesos_sistema);
                PCB* proceso_asociado = obtener_proceso_por_pid(hilo_en_exec->PID);
                pthread_mutex_lock(&mutex_procesos_sistema);

                crear_hilo(proceso_asociado, atoi(syscall->parametro2), syscall->parametro1);

                free(syscall->parametro1);
                free(syscall);

                interrupcion = 0;
                sem_post(&mandar_interrupcion);
                break;
            }
            case THREAD_JOIN: {
                pthread_mutex_lock(&mutex_log);
                log_info(logger_obligatorio, "## (%d:%d) - Solicitó syscall: THREAD_JOIN", hilo_en_exec->PID,hilo_en_exec->TID);
                pthread_mutex_unlock(&mutex_log);

                pthread_mutex_lock(&mutex_procesos_sistema);
                PCB* proceso_asociado = obtener_proceso_por_pid(hilo_en_exec->PID);
                pthread_mutex_unlock(&mutex_procesos_sistema);

                TCB* hilo_asociado = buscar_hilo_por_pid_tid(proceso_asociado->PID, atoi(syscall->parametro1));

                if (hilo_asociado != NULL) {
                    manejar_thread_join(atoi(syscall->parametro1));
                    interrupcion = 1;
                    sem_post(&mandar_interrupcion);
                    sem_post(&hay_hilos_en_ready);
                } else {
                    // Si el hilo con el TID no existe o ya ha terminado, no hacer nada
                    interrupcion = 0;
                    sem_post(&mandar_interrupcion);

                    pthread_mutex_lock(&mutex_log);
                    log_info(logger, "Hilo %d del proceso %d no existe o ya ha terminado, el hilo que invoca THREAD_JOIN continuará su ejecución",atoi(syscall->parametro1),proceso_asociado->PID);
                    pthread_mutex_unlock(&mutex_log);
                }
                break;
            }
            case THREAD_CANCEL: {
                // Recibir los parámetros específicos de THREAD_CREATE
                pthread_mutex_lock(&mutex_log);
                log_info(logger_obligatorio, "## (%d:%d) - Solicitó syscall: THREAD_CANCEL", hilo_en_exec->PID,hilo_en_exec->TID);
                pthread_mutex_unlock(&mutex_log);

                finalizar_hilo(hilo_en_exec->PID, atoi(syscall->parametro1));

                interrupcion = 0;
                sem_post(&mandar_interrupcion);
                break;
            }
            case THREAD_EXIT: {
                pthread_mutex_lock(&mutex_log);
                log_info(logger_obligatorio, "## (%d:%d) - Solicitó syscall: THREAD_EXIT", hilo_en_exec->PID,hilo_en_exec->TID);
                pthread_mutex_unlock(&mutex_log);

                finalizar_hilo(hilo_en_exec->PID, hilo_en_exec->TID);

                hilo_en_exec = NULL;

                interrupcion = 1;
                sem_post(&mandar_interrupcion);
                sem_post(&hay_hilos_en_ready);
                break;
            }
            case MUTEX_CREATE: {
                pthread_mutex_lock(&mutex_log);
                log_info(logger_obligatorio, "## (%d:%d) - Solicitó syscall: MUTEX_CREATE", hilo_en_exec->PID,hilo_en_exec->TID);
                pthread_mutex_unlock(&mutex_log);

                crear_mutex(syscall->parametro1);
                free(syscall->parametro1);

                interrupcion = 0;
                sem_post(&mandar_interrupcion);
                break;
            }
            case MUTEX_LOCK: {
                pthread_mutex_lock(&mutex_log);
                log_info(logger_obligatorio, "## (%d:%d) - Solicitó syscall: MUTEX_LOCK", hilo_en_exec->PID,hilo_en_exec->TID);
                pthread_mutex_unlock(&mutex_log);

                lockear_mutex(syscall->parametro1);
                free(syscall->parametro1);
                break;
            }
            case MUTEX_UNLOCK: {
                pthread_mutex_lock(&mutex_log);
                log_info(logger_obligatorio, "## (%d:%d) - Solicitó syscall: MUTEX_UNLOCK", hilo_en_exec->PID,hilo_en_exec->TID);
                pthread_mutex_unlock(&mutex_log);

                unlockear_mutex(syscall->parametro1);
                free(syscall->parametro1);

                interrupcion = 0;
                sem_post(&mandar_interrupcion);
                break;
            }
            case DUMP_MEMORY: {
                pthread_mutex_lock(&mutex_log);
                log_info(logger_obligatorio, "## (%d:%d) - Solicitó syscall: DUMP_MEMORY", hilo_en_exec->PID,hilo_en_exec->TID);
                pthread_mutex_unlock(&mutex_log);

                pthread_mutex_lock(&mutex_log);
                log_info(logger_obligatorio, "## (%d:%d) - Bloqueado por: DUMP_MEMORY", hilo_en_exec->PID,hilo_en_exec->TID);
                pthread_mutex_unlock(&mutex_log);

                hilo_desalojado = true;
                
                pthread_mutex_lock(&mutex_hilo_exec);
                TCB* hilo = hilo_en_exec;
                hilo_en_exec = NULL;
                pthread_mutex_unlock(&mutex_hilo_exec);

                hilo->estado = BLOCKED_DM;
                
                pthread_mutex_lock(&mutex_cola_dump_memory);
                list_add(cola_dump_memory, hilo);
                pthread_mutex_unlock(&mutex_cola_dump_memory);
                
                interrupcion = 1;
                sem_post(&mandar_interrupcion);
                sem_post(&hay_hilos_en_dump_memory);
                sem_post(&hay_hilos_en_ready);
            }
            case IO: {
                pthread_mutex_lock(&mutex_log);
                log_info(logger_obligatorio, "## (%d:%d) - Solicitó syscall: IO", hilo_en_exec->PID,hilo_en_exec->TID);
                pthread_mutex_unlock(&mutex_log);

                hilo_desalojado = true;
                pthread_mutex_lock(&mutex_log);
                log_info(logger_obligatorio, "## (%d:%d) - Bloqueado por: IO", hilo_en_exec->PID,hilo_en_exec->TID);
                pthread_mutex_unlock(&mutex_log);

                pthread_mutex_lock(&mutex_hilo_exec);
                TCB* hilo = hilo_en_exec;
                hilo->tiempo_bloqueo_io = atoi(syscall->parametro1);
                hilo->estado = BLOCKED_IO;
                pthread_mutex_unlock(&mutex_hilo_exec);
                
                pthread_mutex_lock(&mutex_cola_io);
                list_add(cola_io,hilo);
                pthread_mutex_unlock(&mutex_cola_io);
                
                pthread_mutex_lock(&mutex_hilo_exec);
                hilo_en_exec = NULL;
                pthread_mutex_unlock(&mutex_hilo_exec);

                interrupcion = 1;
                sem_post(&mandar_interrupcion);
                sem_post(&hay_hilos_en_io);
            }
            case SEGMENTATION_FAULT: {
                pthread_mutex_lock(&mutex_log);
                log_info(logger_obligatorio, "## (%d:%d) - Hubo un: SEGMENTATION FAULT", hilo_en_exec->PID,hilo_en_exec->TID);
                pthread_mutex_unlock(&mutex_log);

                hilo_desalojado = true;
                finalizar_proceso(hilo_en_exec->PID);

                pthread_mutex_lock(&mutex_hilo_exec);
                hilo_en_exec = NULL;
                pthread_mutex_unlock(&mutex_hilo_exec);

                interrupcion =1 ;
                sem_post(&mandar_interrupcion);
                sem_post(&hay_hilos_en_ready);
                break;
            }
            default: {
                log_error(logger, "Código de syscall no reconocido");
                break;
            }
        }
    }
}

void procesar_conexion_cpu_interrupt(){
    while (1){
        sem_wait(&mandar_interrupcion);

        enviar_entero(interrupcion, fd_cpu_interrupt);// Enviar la interrupción
    }
}

void conectar_memoria(){
    fd_memoria = crear_conexion2(IP_MEMORIA, PUERTO_MEMORIA);
    if (fd_memoria == -1)
    {
        log_error(logger, "Fallo la conexión con Memoria");
    }
}

/// INICIALIZAR VARIABLES ///
void inicializar_estructuras()
{
    procesos_sistema = list_create();
    hilos_sistema = list_create();
    cola_new = list_create();
    cola_ready = list_create();
    cola_ready_multinivel = list_create();
    cola_io = list_create();
    cola_dump_memory = list_create();
    cola_joins = list_create();

    iniciar_semaforos();
    iniciar_mutex();
    iniciar_hilos();
}

void iniciar_semaforos()
{
    sem_init(&verificar_cola_new, 0, 0);
    sem_init(&hay_hilos_en_ready, 0, 0);
    sem_init(&hay_hilos_en_io, 0, 0);
    sem_init(&hay_hilos_en_dump_memory, 0, 0);
    sem_init(&comenzar_quantum, 0, 0);
    sem_init(&mandar_interrupcion, 0, 0);
    sem_init(&sem_io_mutex, 0, 1); // Inicializa con 1 para permitir acceso exclusivo
}

void iniciar_mutex()
{
    pthread_mutex_init(&mutex_cola_ready,NULL);
    pthread_mutex_init(&mutex_colas_multinivel,NULL);
    pthread_mutex_init(&mutex_procesos_en_new,NULL);
    pthread_mutex_init(&mutex_log,NULL);
    pthread_mutex_init(&mutex_socket_dispatch,NULL);
    pthread_mutex_init(&mutex_socket_interrupt,NULL);
    pthread_mutex_init(&mutex_hilo_exec,NULL);
    pthread_mutex_init(&mutex_cola_io,NULL);
    pthread_mutex_init(&mutex_procesos_sistema,NULL);
    pthread_mutex_init(&mutex_hilos_sistema,NULL);
    pthread_mutex_init(&mutex_cola_join_wait,NULL);
    pthread_mutex_init(&mutex_cola_dump_memory,NULL);
}

void iniciar_hilos()
{
    conexion_cpu_interrupt = malloc(sizeof(pthread_t));
    conexion_cpu_dispatch = malloc(sizeof(pthread_t));
    hilo_gestor_io = malloc(sizeof(pthread_t));
    hilo_gestor_dump_memory = malloc(sizeof(pthread_t));
    hilo_gestor_quantum = malloc(sizeof(pthread_t));
    planificador_corto_plazo = malloc(sizeof(pthread_t));
    planificador_largo_plazo = malloc(sizeof(pthread_t));
}

/// LIBERAR MEMORIA ////
void terminar_programa()
{
    list_destroy(procesos_sistema);
    list_destroy(hilos_sistema);
    list_destroy(cola_new);
    list_destroy(cola_ready);
    list_destroy(cola_ready_multinivel);
    list_destroy(cola_io);
    list_destroy(cola_dump_memory);
    list_destroy(cola_joins);
    free(hilo_en_exec);

    log_destroy(logger);
    log_destroy(logger_obligatorio);
    config_destroy(config);

    liberar_semaforos();
    liberar_mutex();
    liberar_hilos();

    if (fd_cpu_interrupt != -1) liberar_conexion(fd_cpu_interrupt);
    if (fd_cpu_dispatch != -1) liberar_conexion(fd_cpu_dispatch);
    if (fd_memoria != -1) liberar_conexion(fd_memoria);
}

void liberar_mutex()
{
    pthread_mutex_destroy(&mutex_procesos_en_new);
    pthread_mutex_destroy(&mutex_procesos_sistema);
    pthread_mutex_destroy(&mutex_hilos_sistema);
    pthread_mutex_destroy(&mutex_cola_ready);
    pthread_mutex_destroy(&mutex_colas_multinivel);
    pthread_mutex_destroy(&mutex_cola_io);
    pthread_mutex_destroy(&mutex_hilo_exec);
    pthread_mutex_destroy(&mutex_log);
    pthread_mutex_destroy(&mutex_socket_dispatch);
    pthread_mutex_destroy(&mutex_socket_interrupt);
    pthread_mutex_destroy(&mutex_cola_join_wait);
}

void liberar_semaforos()
{
    sem_destroy(&verificar_cola_new);
    sem_destroy(&hay_hilos_en_ready);
    sem_destroy(&hay_hilos_en_io);
    sem_destroy(&hay_hilos_en_dump_memory);
    sem_destroy(&comenzar_quantum);
    sem_destroy(&sem_io_mutex);
    sem_destroy(&mandar_interrupcion);
}

void liberar_hilos()
{
    free(planificador_largo_plazo);
    free(planificador_corto_plazo);
    free(hilo_gestor_io);
    free(hilo_gestor_quantum);
    free(conexion_cpu_dispatch);
    free(conexion_cpu_interrupt);
}