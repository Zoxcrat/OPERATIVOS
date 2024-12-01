#ifndef CONEXIONES_CLIENTE
#define CONEXIONES_CLIENTE
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <commons/log.h>
#include <commons/string.h>
#include "protocolo.h"

typedef enum
{
	MENSAJE,
	PAQUETE,
	INICIALIZAR_PROCESO,
	FINALIZACION_PROCESO,
	CREACION_HILO,
	FINALIZACION_HILO,
	EJECUTAR_HILO,
	// MEMORIA
	MEMORY_DUMP,
	PEDIDO_CONTEXTO,
	ACTUALIZAR_CONTEXTO,
	OBTENER_INSTRUCCION,
	ESCRIBIR_MEMORIA, // WRITE MEM
	LEER_MEMORIA,	  // READ MEM
	PROXIMA_INSTRUCCION,
	//
	SET,
	READ_MEM,
	WRITE_MEM,
	SUM,
	SUB,
	JNZ,
	LOG,
	PROCESS_CREATE,
	PROCESS_EXIT,
	THREAD_CREATE,
	THREAD_JOIN,
	THREAD_CANCEL,
	THREAD_EXIT,
	MUTEX_CREATE,
	MUTEX_LOCK,
	MUTEX_UNLOCK,
	DUMP_MEMORY,
	IO,
	SEGMENTATION_FAULT,
	EXIT
} op_code;

typedef struct t_instruccion
{
	op_code instruccion;
	char *parametro1;
	char *parametro2;
	char *parametro3;
} t_instruccion_completa;

typedef struct
{
	char *nombre_archivo_pseudocodigo;
	int tamano_proceso;
	int prioridad_hilo_0;
} t_syscall_process_create;

typedef struct
{
	char *nombre_archivo_pseudocodigo;
	int prioridad;
} t_syscall_thread_create;

typedef struct
{
	int size;
	void *stream;
} t_buffer;

typedef struct
{
	op_code codigo_operacion;
	t_buffer *buffer;
} t_paquete;

void enviar_contexto(t_contexto_ejecucion *contexto, int socket_cliente);
/**
 * @fn crear_conexion
 * @brief
 * @param ip direccion donde nos vamos a conectar
 * @param puerto puerto en el cual nos vamos a conectar
 * @param identificador nombre de quien se esta identificando
 * @return retrona el socket del servidor
 */
int crear_conexion(char *ip, char *puerto, char *identificador);
/**
 * @fn enviar_mensaje
 * @brief
 * @param mensaje
 * @param socket_cliente
 * @return
 */
void enviar_mensaje(char *mensaje, int socket_cliente);
/**
 * @fn crear_paquete
 * @brief
 * @return
 */
t_paquete *crear_paquete(void);
/**
 * @fn agregar_a_paquete
 * @brief
 * @param paquete
 * @param valor
 * @param tamanio
 * @return
 */
void agregar_a_paquete(t_paquete *paquete, void *valor, int tamanio);
/**
 * @fn enviar_paquete
 * @brief
 * @param paquete
 * @param socket_cliente
 * @return
 */
void enviar_paquete(t_paquete *paquete, int socket_cliente);
/**
 * @fn liberar_conexion
 * @brief
 * @param socket_cliente
 * @return
 */
void liberar_conexion(int socket_cliente);
/**
 * @fn eliminar_paquete
 * @brief
 * @param
 * @return
 */
void eliminar_paquete(t_paquete *paquete);

void enviar_peticion(t_paquete *paquete, int socket_cliente, op_code codigo);

void enviar_entero(int valor, int socket_cliente);
void enviar_entero_como_int(int valor, int socket_cliente);

/**
 * Similar a crear_conexion, pero no envía un mensaje (para tener control sobre el handshake).
 */
int crear_conexion2(char *ip, char *puerto);
void agregar_instruccion_a_paquete(t_paquete *paquete, t_instruccion_completa *instruccion);
#endif // !CONEXIONES_CLIENTE