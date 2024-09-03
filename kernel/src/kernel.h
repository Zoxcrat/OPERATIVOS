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

t_log* logger_obligatorio;
t_log* logger;
t_config* config;

int fd_memoria;
int fd_cpu_interrupt;
int fd_cpu_dispatch;
// Variables del config (Las pongo aca asi no estamos revoleando el cfg para todos lados)
char* IP;
char* PUERTO;
char* IP_CPU;
char* PUERTO_CPU_DISPATCH;
char* PUERTO_CPU_INTERRUPT;
char* IP_MEMORIA;
char* PUERTO_MEMORIA;
t_algoritmo ALGORITMO_PLANIFICACION;
int QUANTUM;
t_log_level LOG_LEVEL;
// faltaria log-level

void leer_config();
void asignar_algoritmo(char *algoritmo);
void terminar_programa();

#endif
