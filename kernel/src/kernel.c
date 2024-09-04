#include "./kernel.h"

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        printf("Uso: %s [archivo_pseudocodigo] [tamanio_proceso]\n", argv[0]);
        return EXIT_FAILURE;
    }

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

    // Inicializar proceso con argumentos
    archivo_pseudocodigo = argv[1];
    tamanio_proceso = atoi(argv[2]);

    // Conexión con CPU (Dispatch e Interrupt)
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
    enviar_mensaje("Hola CPU interrupt, Soy Kernel!", fd_cpu_interrupt);
    enviar_mensaje("Hola CPU dispatcher, Soy Kernel!", fd_cpu_dispatch);

    // Simulación de ejecución
    while (1)
    {
        // Planificar procesos e hilos
        planificar_procesos_y_hilos();

        // Simulación de entrada/salida
        manejar_syscalls();

        // Simulación de solicitudes a Memoria
        manejar_conexiones_memoria();
    }

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

void planificar_procesos_y_hilos()
{
    // Lógica para la planificación de procesos y hilos
    // Dependiendo del algoritmo configurado
    // Enviar procesos a CPU para ser ejecutados
}

void manejar_syscalls()
{
    // Lógica para manejar llamadas al sistema que simulan entrada/salida
}


void manejar_cola_new()
{
    while (1)
    {
        if (list_size(cola_new) > 0)
        {
            // Tomar el primer proceso de la cola NEW
            PCB* nuevo_proceso = list_remove(cola_new, 0);

            // Intentar inicializar el proceso en memoria
            if (inicializar_proceso_en_memoria(nuevo_proceso->PID))
            {
                // Crear el TID 0 para el proceso
                TCB* tcb_inicial = malloc(sizeof(TCB));
                tcb_inicial->TID = 0;
                tcb_inicial->prioridad = 0; // Puede cambiar la prioridad!!!

                // Añadir el TCB a la lista de TIDs del proceso
                list_add(nuevo_proceso->TIDs, tcb_inicial);

                // Pasar el proceso al estado READY
                list_add(lista_ready, nuevo_proceso);
            }
            else
            {
                // Si no se puede inicializar el proceso, volver a encolarlo en NEW
                list_add(cola_new, nuevo_proceso);
            }
        }

        // Esperar un poco antes de intentar nuevamente (ajustar el tiempo de espera según sea necesario)
        sleep(1);
    }
}

int inicializar_proceso_en_memoria(int PID)
{
    // Crear conexión efímera a la memoria
    int fd_memoria = crear_conexion2(IP_MEMORIA, PUERTO_MEMORIA);
    if (fd_memoria == -1)
    {
        log_error(logger_obligatorio, "Fallo la conexión efímera con Memoria");
        return 0; // No se pudo inicializar el proceso
    }

    // Enviar solicitud para inicializar el proceso
    char mensaje[256];
    snprintf(mensaje, sizeof(mensaje), "INICIALIZAR_PROCESO %d", PID);
    enviar_mensaje(mensaje, fd_memoria);

    // Esperar respuesta de la memoria
    char respuesta[256];
    recibir_mensaje(respuesta, fd_memoria);

    cerrar_conexion(fd_memoria);

    // Verificar si la respuesta indica éxito
    if (strcmp(respuesta, "OK") == 0)
    {
        return 1; // Proceso inicializado exitosamente
    }
    else
    {
        return 0; // No se pudo inicializar el proceso
    }
}

void terminar_programa()
{
    log_destroy(logger);
    log_destroy(logger_obligatorio);
    config_destroy(config);
}