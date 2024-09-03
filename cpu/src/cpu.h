#ifndef CPU_H_
#define CPU_H_

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

t_log* logger;
t_log* logger_obligatorio;
t_config* config;

//t_contexto_ejecucion* contexto_de_ejecucion;
int fd_memoria;
int socket_cliente_dispatch;
int socket_cliente_interrupt;
// Variables del config (Las pongo aca asi no estamos revoleando el cfg para todos lados)

char* IP_ESCUCHA;
char* IP_MEMORIA;
char* PUERTO_MEMORIA;
char* PUERTO_ESCUCHA_DISPATCH;
char* PUERTO_ESCUCHA_INTERRUPT;
t_log_level LOG_LEVEL;

// INIT
void leer_config();
void terminar_programa();

#endif