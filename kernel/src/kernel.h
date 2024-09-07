#ifndef KERNEL_H_
#define KERNEL_H_

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
	BLOCKED,
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
} TCB;


// Variables
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
t_list* procesos_sistema;
t_list* cola_new;
t_list* cola_ready;
t_list* cola_exec;
t_list* cola_blocked;
t_list* cola_exit;
int pid_global;

// Semaforos
sem_t verificar_cola_new;

// Hilos
pthread_t* planificador_largo_plazo;
pthread_t* planificador_corto_plazo;
pthread_t* conexion_cpu_dispatch;
pthread_t* conexion_cpu_interrupt;

// Mutexs
pthread_mutex_t mutex_cola_ready;
pthread_mutex_t mutex_colas_multinivel;

// Funciones
void leer_config();
void inicializar_variables();
void asignar_algoritmo(char *algoritmo);
bool generar_conexiones();
void procesar_conexion_cpu_dispatch();
void procesar_conexion_cpu_interrupt();
void conectar_memoria();
void iniciar_hilos();
void terminar_programa();


#include <plani_corto_plazo.h>
#include <plani_largo_plazo.h>
#include <procesos_hilos.h>


#endif
