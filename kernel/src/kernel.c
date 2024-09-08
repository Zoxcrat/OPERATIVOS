#include "./kernel.h"

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
    fd_cpu_dispatch = -1, fd_cpu_interrupt = -1;
	if (!generar_conexiones()) {
		log_error(logger, "Alguna conexion con el CPU fallo :(");
		terminar_programa();
		exit(1);
	}
    
    // Mensajes iniciales de saludo a los módulos
    enviar_mensaje("Hola CPU interrupt, Soy Kernel!", fd_cpu_interrupt);
    enviar_mensaje("Hola CPU dispatcher, Soy Kernel!", fd_cpu_dispatch);

    inicializar_plani_largo_plazo();
    inicializar_plani_corto_plazo();

    crear_proceso(archivo_pseudocodigo, tamanio_proceso, 0);

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
    //FALTA IMPLEMENTAR
}

void procesar_conexion_cpu_interrupt(){}

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
    cola_new = list_create();
    cola_ready = list_create();
    cola_ready_multinivel = list_create();
    cola_blocked = list_create();
    cola_exit = list_create();

    iniciar_semaforos();
    iniciar_mutex();
    iniciar_hilos();
}

void iniciar_semaforos()
{
    sem_init(&verificar_cola_new, 0, 0);
    sem_init(&hay_hilos_en_ready, 0, 0);
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
    pthread_mutex_init(&mutex_cola_blocked,NULL);
}

void iniciar_hilos()
{
    conexion_cpu_interrupt = malloc(sizeof(pthread_t));
    conexion_cpu_dispatch = malloc(sizeof(pthread_t));
    planificador_corto_plazo = malloc(sizeof(pthread_t));
    planificador_largo_plazo = malloc(sizeof(pthread_t));
}

/// LIBERAR MEMORIA ////
void terminar_programa()
{
    list_destroy(procesos_sistema);
    list_destroy(cola_new);
    list_destroy(cola_ready);
    list_destroy(cola_ready_multinivel);
    list_destroy(cola_blocked);
    list_destroy(cola_exit);

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
    pthread_mutex_destroy(&mutex_cola_ready);
    pthread_mutex_destroy(&mutex_colas_multinivel);
    pthread_mutex_destroy(&mutex_procesos_en_new);
    pthread_mutex_destroy(&mutex_log);
    pthread_mutex_destroy(&mutex_socket_dispatch);
    pthread_mutex_destroy(&mutex_socket_interrupt);
    pthread_mutex_destroy(&mutex_hilo_exec);
    pthread_mutex_destroy(&mutex_cola_blocked);
}

void liberar_semaforos()
{
    sem_destroy(&verificar_cola_new);
    sem_destroy(&hay_hilos_en_ready);
}

void liberar_hilos()
{
    free(planificador_largo_plazo);
    free(planificador_corto_plazo);
    free(conexion_cpu_dispatch);
    free(conexion_cpu_interrupt);
}