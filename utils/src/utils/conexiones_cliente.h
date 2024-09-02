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

typedef enum {
	MOD_KERNEL,
	MOD_CPU,
	MOD_MEMORIA,
	MOD_IO
} modulos;

typedef enum
{
	MENSAJE,
	PAQUETE,
	MEM_ENVIAR_INSTRUCCION,
    MEM_CREAR_PROCESO,
    MEM_FINALIZAR_PROCESO,
    MEM_ACCESO_TABLA_PAGINAS,
    MEM_RESIZE_PROCESO,
    MEM_LEER_MEMORIA,
    MEM_ESCRIBIR_MEMORIA,
	KER_STDIN_READ,
	KER_STDOUT_WRITE,
	IO_GEN_SLEEP,
    IO_STDIN_READ,
    IO_STDOUT_WRITE,
    IO_FS_CREATE,
    IO_FS_DELETE,
    IO_FS_TRUNCATE,
    IO_FS_WRITE,
    IO_FS_READ,
	CPU_INTERRUPT,
	CPU_EXEC_PROC,
	KER_CDE
} op_code;

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

typedef struct
{
	int uTrabajo;
} t_genSleep;


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

/**
 * Similar a crear_conexion, pero no envía un mensaje (para tener control sobre el handshake).
*/
int crear_conexion2(char *ip, char* puerto);

#endif // !CONEXIONES_CLIENTE