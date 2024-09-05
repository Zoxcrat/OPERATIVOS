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
#include <commons/temporal.h>
#include <pthread.h>

typedef enum{
	FIFO,
	PRIORIDADES,
    COLAS_MULTINIVEL
} t_algoritmo;

typedef enum{
	NEW,
	READY,
	EXEC,
	BLOCK,
	EXIT
} estado_proceso;
typedef struct {
    int PID;                        // Identificador del proceso
    t_list* TIDs;                   // Lista de TIDs (hilos) asociados al proceso
    t_list* mutex_list;             // Lista de mutex creados para el proceso
} PCB;

typedef struct {
    int TID;             // Identificador del hilo
    int prioridad;       // Prioridad del hilo
    estado_proceso estado;       // Prioridad del hilo
} TCB;

t_log* logger_obligatorio;
t_log* logger;
t_config* config;

int fd_memoria;
int fd_cpu_interrupt;
int fd_cpu_dispatch;
// Variables del config (Las pongo aca asi no estamos revoleando el cfg para todos lados)
char *archivo_pseudocodigo;
int tamanio_proceso;
char* IP_CPU;
char* PUERTO_CPU_DISPATCH;
char* PUERTO_CPU_INTERRUPT;
char* IP_MEMORIA;
char* PUERTO_MEMORIA;
t_algoritmo ALGORITMO_PLANIFICACION;
int QUANTUM;
t_log_level LOG_LEVEL;

// Variables PCBs
int generador_pid;
t_list* cola_new;
t_list* lista_ready;
t_list* cola_exec;
t_list* cola_blocked;
t_list* cola_exit;



void leer_config();
void asignar_algoritmo(char *algoritmo);
void planificar_procesos_y_hilos();
void manejar_syscalls();
void manejar_conexiones_memoria();
void manejar_cola_new();
int inicializar_proceso_en_memoria(int PID);
void terminar_programa();

#endif
