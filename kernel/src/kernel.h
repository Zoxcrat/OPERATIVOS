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
    estado_proceso estado;       // Estado del hilo
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
t_list* cola_new;
t_list* cola_ready;
t_list* cola_exec;
t_list* cola_blocked;
t_list* cola_exit;
t_list* lista_mutex_bloqueados;

void leer_config();
void inicializar_variables();
bool generar_conexiones();
void procesar_conexion_CPUi();
void procesar_conexion_CPUd();
void asignar_algoritmo(char *algoritmo);
void conectar_memoria();
void send_inicializar_proceso(int proceso_id);
void send_finalizar_proceso(int proceso_id);
void send_inicializar_hilo(int hilo_id, int prioridad);
void send_finalizar_hilo(int hilo_id);
void liberar_PCB(int proceso_id);
int buscar_proceso_en_cola(t_list* cola,int proceso_id);
void agregar_a_ready_segun_algoritmo(TCB* nuevo_hilo);
void mover_a_ready_hilos_bloqueados(int hilo_id);
void terminar_programa();

#endif
