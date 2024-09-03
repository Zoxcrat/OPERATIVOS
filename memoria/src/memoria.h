#ifndef MEMORIA_H_
#define MEMORIA_H_

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
#include <sockets.h>


t_log* logger;
t_log* logger_obligatorio;
t_config* config;

//t_contexto_ejecucion* contexto_de_ejecucion;
int socket_cliente;
int memoria_socket;
int fd_filesystem;

// Variables del config (Las pongo aca asi no estamos revoleando el cfg para todos lados)

char* IP_ESCUCHA;
char* PUERTO_ESCUCHA;
char* IP_FILESYSTEM;
char* PUERTO_FILESYSTEM;
int TAM_MEMORIA;
char* PATH_INSTRUCCIONES;
int RETARDO_RESPUESTA;
char* ESQUEMA;
char* ALGORITMO_BUSQUEDA;
char* PARTICIONES; //REVISAR
t_log_level LOG_LEVEL;

// INIT
void leer_config();
void terminar_programa();

#endif