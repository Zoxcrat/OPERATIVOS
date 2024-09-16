#ifndef MAIN_H_
#define MAIN_H_

#include <stdlib.h>
#include <stdio.h>
#include <../../utils/src/utils/conexiones_cliente.h>
#include <../../utils/src/utils/conexiones_servidor.h>
#include <commons/config.h>
#include <commons/log.h>
#include <commons/error.h>
#include <readline/readline.h>
#include <string.h>
#include <commons/temporal.h>
#include <pthread.h>
#include <semaphore.h>

typedef enum{
	FIFO,
	PRIORIDADES,
    COLAS_MULTINIVEL
} t_algoritmo;

typedef enum{
	NEW,
	READY,
	EXEC,
	BLOCKED_IO,
	BLOCKED_TJ,
	BLOCKED_MUTEX,
	BLOCKED_DM,
	EXIT
} estado;
typedef struct {
    int PID;                        // Identificador del proceso
    t_list* TIDs;                   // Lista de IDs de los hilos (TCB) asociados al proceso
    t_list* mutexs;             // Lista de mutex creados para el proceso
    int tamanio;              // Tamaño del proceso en memoria
    int prioridad_hilo_0; // prioridad del hilo principal
    char *archivo_pseudocodigo_principal; // Nombre del archivo de pseudocódigo
} PCB;

typedef struct {
    int TID;             // Identificador del hilo
    int PID;         // PID asociado (proceso padre)
    int prioridad;       // Prioridad del hilo
    char *archivo_pseudocodigo; // Nombre del archivo de pseudocódigo
    estado estado;       // Estado del hilo
    int tiempo_bloqueo_io; // Tiempo de bloqueo por operación de I/O (milisegundos)
} TCB;

typedef struct {
    t_list* cola;          // hilos almacenados
    int prioridad;         // Prioridad de esta cola
    int quantum_restante;  // Quantum restante para el Round Robin actual
} t_cola_multinivel;

typedef struct {
    int pid;           // PID del proceso (el mismo para el hilo esperador y el esperado)
    TCB* tid_esperador; // TID del hilo que espera
    int tid_esperado;  // TID del hilo esperado
} t_join_wait;

typedef struct {
    char* nombre;               // Identificador único del mutex
    int TID;                    // TID del hilo que posee el mutex (si está ocupado)
    t_list* cola_bloqueados;    // Lista de hilos bloqueados esperando el mutex
} t_mutex;

// Variables config
extern t_log* logger_obligatorio;
extern t_log* logger;
extern t_config* config;
extern int fd_memoria;
extern int fd_cpu_interrupt;
extern int fd_cpu_dispatch;

// Variables del config (Las pongo aca asi no estamos revoleando el cfg para todos lados)
extern char* IP_CPU;
extern char* PUERTO_CPU_DISPATCH;
extern char* PUERTO_CPU_INTERRUPT;
extern char* IP_MEMORIA;
extern char* PUERTO_MEMORIA;
extern t_algoritmo ALGORITMO_PLANIFICACION;
extern int QUANTUM;
extern t_log_level LOG_LEVEL;

// Variables PCBs
extern int pid_global;
extern t_list* procesos_sistema;
extern t_list* cola_new;
extern t_list* cola_ready;
extern t_list* cola_ready_multinivel;
extern t_list* cola_io;
extern t_list* cola_joins; 
extern t_list* cola_dump_memory; 
extern bool io_en_uso;// Estado que indica si el I/O está en uso
extern TCB* hilo_en_exec;
extern int pid_a_buscar; 

// Semaforos
extern sem_t hay_hilos_en_dump_memory;
extern sem_t verificar_cola_new;
extern sem_t hay_hilos_en_ready;
extern sem_t sem_io_mutex;
extern sem_t mandar_interrupcion;
extern sem_t hay_hilos_en_io;

// Hilos
extern pthread_t* planificador_largo_plazo;
extern pthread_t* planificador_corto_plazo;
extern pthread_t* hilo_gestor_dump_memory;
extern pthread_t* hilo_gestor_io;
extern pthread_t* conexion_cpu_dispatch;
extern pthread_t* conexion_cpu_interrupt;

// Mutexs
extern pthread_mutex_t mutex_procesos_en_new;
extern pthread_mutex_t mutex_procesos_sistema;
extern pthread_mutex_t mutex_cola_ready;
extern pthread_mutex_t mutex_colas_multinivel;
extern pthread_mutex_t mutex_cola_io;
extern pthread_mutex_t mutex_cola_join_wait;
extern pthread_mutex_t mutex_cola_dump_memory;
extern pthread_mutex_t mutex_log;
extern pthread_mutex_t mutex_socket_dispatch;
extern pthread_mutex_t mutex_socket_interrupt;
extern pthread_mutex_t mutex_hilo_exec;

// Funciones
void leer_config();
void asignar_algoritmo(char *algoritmo);
bool generar_conexiones();
void procesar_conexion_cpu_dispatch();
void procesar_conexion_cpu_interrupt();
void inicializar_estructuras();
void conectar_memoria();
void iniciar_semaforos();
void iniciar_mutex();
void iniciar_hilos();
void terminar_programa();
void liberar_mutex();
void liberar_semaforos();
void liberar_hilos();

#include <plani_corto_plazo.h>
#include <plani_largo_plazo.h>
#include <procesos_hilos.h>
#include <gestor_io.h>
#include <gestor_dump_memory.h>

#endif
