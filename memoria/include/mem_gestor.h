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

#include <dirent.h>
#include <readline/readline.h>
#include <commons/string.h>
#include <commons/config.h>

extern t_log *logger;
extern t_log *logger_obligatorio;
extern t_config *config;

// t_contexto_ejecucion* contexto_de_ejecucion;
extern int socket_cliente;
extern int memoria_socket;
extern int fd_filesystem;
extern int fd_cpu;
extern int fd_kernel;

// Variables del config (Las pongo aca asi no estamos revoleando el cfg para todos lados)

extern char *IP_ESCUCHA;
extern char *PUERTO_ESCUCHA;
extern char *IP_FILESYSTEM;
extern char *PUERTO_FILESYSTEM;
extern int TAM_MEMORIA;
extern char *PATH_INSTRUCCIONES;
extern int RETARDO_RESPUESTA;
extern char *ESQUEMA;
extern char *ALGORITMO_BUSQUEDA;
extern char **PARTICIONES; // REVISAR
extern t_log_level LOG_LEVEL;

// Variables de memoria
extern void *memoria_usuario;
extern t_list *lista_particiones;
extern t_list *lista_procesos_en_memoria;
extern int proceso_buscado_cpu;
extern int proceso_buscado_kernel;
extern int proceso_buscado_fs;
typedef struct t_contexto_proceso
{
    void *base;
    int limite;
} t_contexto_proceso;

typedef struct t_hilo
{
    int tid;
    t_registros_cpu *registros;
    t_list *lista_instrucciones;
} t_hilo;

typedef struct t_proceso
{
    int pid;
    int nro_hilos;
    t_contexto_proceso *contexto;
    t_list *lista_hilos;
} t_proceso;

enum ID_CLIENTE
{
    KERNEL,
    CPU
};

#include "mem_instrucciones.h"
#include "mem_init.h"
#include "mem_kernel.h"
#include "mem_cpu.h"
#include "mem_cleanup.h"
#include "mem_fs.h"

// INIT
void leer_config();
void *procesar_conexion(void *arg);
void *procesar_peticion(void *arg);
void terminar_programa();
void atender_cpu();
void atender_kernel();
void destruir_hilo(t_hilo *hilo);

typedef struct t_particion
{
    bool libre;
    void *base;
    int tamanio;
    int orden;
} t_particion;

#endif