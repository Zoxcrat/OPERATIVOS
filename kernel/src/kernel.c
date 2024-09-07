#include "./kernel.h"

int main(int argc, char **argv)
{
    if (argc > 3) // no se por que habias puesto <, pero el programa se quedaba trabado, asi que lo cambie por >. Fijate porque capaz no eniendo el motivo
    {
        printf("Uso: %s [archivo_pseudocodigo] [tamanio_proceso]\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Inicializar proceso con argumentos
    archivo_pseudocodigo = argv[1];
    tamanio_proceso = atoi(argv[2]);

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

    inicializar_variables();

    //conecto con CPU Dispatch y CPU interrupt
    fd_cpu_dispatch = -1, fd_cpu_interrupt = -1;
	if (!generar_conexiones()) {
		log_error(logger, "Alguna conexion con el CPU fallo :(");
		terminar_programa();
		exit(1);
	}
    // Conecto con memoria
    conectar_memoria();
        if (fd_memoria == -1) {
        log_error(logger, "Error al conectar con el módulo memoria");
        return EXIT_FAILURE;
    }
    // Mensajes iniciales de saludo a los módulos
    enviar_mensaje("Hola CPU interrupt, Soy Kernel!", fd_cpu_interrupt);
    enviar_mensaje("Hola CPU dispatcher, Soy Kernel!", fd_cpu_dispatch);


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

void inicializar_variables(){
    cola_new = list_create();
    cola_ready = list_create();
    cola_exec= list_create();
    cola_blocked = list_create();
    cola_exit = list_create();
}

bool generar_conexiones(){
    // Conexión con CPU (Dispatch e Interrupt)
    pthread_t conexion_cpu_interrupt;
	pthread_t conexion_cpu_dispatch;

	fd_cpu_interrupt = crear_conexion2(IP_CPU, PUERTO_CPU_INTERRUPT);
	pthread_create(&conexion_cpu_interrupt, NULL, (void*) procesar_conexion_CPUi, (void*) &fd_cpu_interrupt);
	pthread_detach(conexion_cpu_interrupt);

	fd_cpu_dispatch = crear_conexion2(IP_CPU, PUERTO_CPU_DISPATCH);
	pthread_create(&conexion_cpu_dispatch, NULL, (void*) procesar_conexion_CPUd, (void*) &fd_cpu_dispatch);
	pthread_detach(conexion_cpu_dispatch);

	return fd_cpu_interrupt != -1 && fd_cpu_dispatch != -1;
}

void procesar_conexion_CPUi(){}

void procesar_conexion_CPUd(){}

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

void send_inicializar_proceso(int proceso_id){
    // Crear conexión efimera a la memoria
    conectar_memoria();
	t_paquete* paquete = crear_paquete();
	agregar_a_paquete(paquete, &proceso_id, sizeof(int));
    enviar_peticion(paquete,fd_memoria,INICIALIZAR_PROCESO);
	eliminar_paquete(paquete);
     // Esperar la respuesta de la memoria
    int respuesta;
    recv(fd_memoria, &respuesta, sizeof(int), 0);
    close(fd_memoria);  // Cerrar la conexión con memoria
    // Procesar la respuesta de la memoria
    if (respuesta == 1) {
        log_info(logger, "Proceso %d inicializado correctamente", proceso_id);
        // Crear el hilo principal (TID 0) del proceso y pasarlo a READY
        TCB* hilo_principal = malloc(sizeof(TCB));
        hilo_principal->TID = 0;  // TID 0 para el hilo principal
        hilo_principal->estado = READY;
        hilo_principal->prioridad = 0; // a priori para probar, pero deberia variar
        // Aca pasaria a la cola READY (deberia ajustar su posicion segun su prioridad pero no se sabe cual sera todavia)
    } else {
        log_error(logger, "No se pudo inicializar el proceso %d. Memoria llena", proceso_id);
    }
}
void send_finalizar_proceso(int proceso_id){
    // Crear conexión efimera a la memoria 
    conectar_memoria();
	t_paquete* paquete = crear_paquete();
	agregar_a_paquete(paquete, &proceso_id, sizeof(int));
    enviar_peticion(paquete,fd_memoria,FINALIZAR_PROCESO);
	eliminar_paquete(paquete);
     // Esperar la respuesta de la memoria
    int respuesta;
    recv(fd_memoria, &respuesta, sizeof(int), 0);
    close(fd_memoria);
    
    // Procesar la respuesta de la memoria
    if (respuesta == 1) {
        log_info(logger, "Proceso %d finalizado correctamente", proceso_id);
        liberar_PCB(proceso_id);

        // Intentar inicializar un nuevo proceso de la cola NEW
        if (!list_is_empty(cola_new)) {
            // Obtener el primer proceso de la cola NEW
            PCB* nuevo_proceso = list_get(cola_new, 0);
            // Enviar solicitud a memoria para inicializar el nuevo proceso
            send_inicializar_proceso(nuevo_proceso->PID);
        }
    } else {
        log_error(logger, "No se pudo finalizar el proceso %d.", proceso_id);
    }
}
void send_inicializar_hilo(int hilo_id, int prioridad) {
    // Crear conexión efímera a la memoria
    conectar_memoria();
	t_paquete* paquete = crear_paquete();
	agregar_a_paquete(paquete, &hilo_id, sizeof(int));
    enviar_peticion(paquete,fd_memoria,INICIALIZAR_HILO);
	eliminar_paquete(paquete);

    // Esperar la respuesta de la memoria
    int respuesta; 
    recv(fd_memoria, &respuesta, sizeof(int), 0);

    // Procesar la respuesta de la memoria
    if (respuesta == 1) {  // Considerar OK como una constante que represente un valor numérico
        log_info(logger, "Hilo %d inicializado correctamente", hilo_id);
        // Agregar el hilo a la cola de READY según su prioridad

        TCB* nuevo_tcb = malloc(sizeof(TCB));
        nuevo_tcb->TID = hilo_id;     
        nuevo_tcb->prioridad = prioridad;
        nuevo_tcb->estado = READY;
        agregar_a_ready_segun_algoritmo(nuevo_tcb);
    } else {
        log_error(logger, "No se pudo inicializar el hilo %d. Memoria llena", hilo_id);
    }

    close(fd_memoria);  // Cerrar la conexión con memoria
}
void send_finalizar_hilo(int hilo_id){
    // Crear conexión efimera a la memoria 
    conectar_memoria();
	t_paquete* paquete = crear_paquete();
	agregar_a_paquete(paquete, &hilo_id, sizeof(int));
    enviar_peticion(paquete,fd_memoria,FINALIZAR_HILO);
	eliminar_paquete(paquete);
     // Esperar la respuesta de la memoria
    int respuesta;
    recv(fd_memoria, &respuesta, sizeof(int), 0);
    
    // Procesar la respuesta de la memoria
    if (respuesta == 1) {
        log_info(logger, "Proceso %d inicializado correctamente", hilo_id);
        mover_a_ready_hilos_bloqueados(hilo_id);
    } else {
        log_error(logger, "No se pudo inicializar el proceso %d. Memoria llena", hilo_id);
    }
    close(fd_memoria);
}

void liberar_PCB(int proceso_id) {
    // Buscar el proceso en las colas o listas donde esté registrado y eliminarlo
    int index = buscar_proceso_en_cola(cola_exit,proceso_id);
    if (index != -1) {
        // Eliminar el proceso de cualquier estructura asociada
        PCB* proceso=list_get(cola_exit,index);
        list_remove(cola_exit,index);
        free(proceso);  // Liberar la memoria asociada al PCB
        log_info(logger, "Proceso %d liberado", proceso_id);
    } else {
        log_error(logger, "No se encontró el proceso %d para liberar", proceso_id);
    }
}

int buscar_proceso_en_cola(t_list* cola,int proceso_id) {
    for (int i = 0; i < list_size(cola); i++) {
        PCB* proceso = list_get(cola, i);
        if (proceso->PID == proceso_id) {
            return i;  // Devolver el index si coincide
        }
    }
    
    return -1;  // Si no se encontró el proceso
}

void agregar_a_ready_segun_algoritmo(TCB* nuevo_hilo) {};

void mover_a_ready_hilos_bloqueados(int hilo_id) {};

void terminar_programa()
{
    log_destroy(logger);
    log_destroy(logger_obligatorio);
    config_destroy(config);
    
    if (fd_cpu_interrupt != -1) liberar_conexion(fd_cpu_interrupt);
    if (fd_cpu_dispatch != -1) liberar_conexion(fd_cpu_dispatch);
    if (fd_memoria != -1) liberar_conexion(fd_memoria);
}