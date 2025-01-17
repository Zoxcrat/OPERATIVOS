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
#include <semaphore.h>

char* IP_ESCUCHA;
char* PUERTO_ESCUCHA_DISPATCH;
char* PUERTO_ESCUCHA_INTERRUPT;
char* IP_MEMORIA;
char* PUERTO_MEMORIA;
t_log_level LOG_LEVEL;

t_log* logger;
t_log* logger_obligatorio;
t_config* config;

int fd_memoria;
int dispatch_socket;
int interrupt_socket;

int* pid_actual;
int* tid_actual;
t_instruccion_completa* instruccion_actual;
t_contexto_ejecucion* contexto;
int interrupcion; // flag para manejar ciclo 

// threads
pthread_t conexion_memoria;
pthread_t hilo_dispatch;
pthread_t hilo_interrupt;

sem_t pedir_contexto;
sem_t verificar_interrupcion;

// Variables del config (Las pongo aca asi no estamos revoleando el cfg para todos lados)

// Funciones principales del ciclo de instrucción

void leer_config();
bool generar_conexiones();
void* manejar_cliente_dispatch();
void* manejar_cliente_interrupt();
void pedir_contexto_a_memoria();
void cpu_cycle();
char* fetch();
void decode(char* mensaje);
void execute();
int traducir_direccion_logica_a_fisica(char* registro_direccion);
void set_valor_registro(char* registro, uint32_t valor);
void sumar_registros(char* destino, char* origen);
op_code string_a_instruccion(char *str);
void restar_registros(char* destino, char* origen);
void jnz_registro(char* registro, uint32_t instruccion);
void log_registro(char* registro);
uint32_t leer_memoria(int direccion_fisica);
void escribir_memoria(char* registro_datos,int direccion_fisica);
void inicializar_estructuras();
void terminar_programa();


#endif