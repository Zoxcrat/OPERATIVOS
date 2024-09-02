#ifndef CONEXIONES_SERVIDOR
#define CONEXIONES_SERVIDOR

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <commons/string.h>
/**
 * @fn iniciar_servidor
 * @brief Inicia el servidor con un puerto determinado
 * @param puerto determina el puerto
 * @return retorna elsocket del servidor
*/
int iniciar_servidor(char*);
/**
 * @fn esperar_cliente
 * @brief Espera a que el cliente se conecte al servidor
 * @param socket_servidor Cual es el socket en el que se enceuntra el servidor. 
 * @param logger puntero del archivo log al cual se enviara el estado.
 * @return retorna elsocket del cliente que se conecto
*/
int esperar_cliente(int, t_log*);
/**
 * @fn recibir_buffer
 * @brief se recibe la cantidad de bytes indicados en el buffer
 * @param size cantidad de bytes a recibir
 * @param socket_cliente socket del cliciente del cual va a recibir los bytes
 * @return retonar los bytes recibidos
*/
void* recibir_buffer(int*, int );
/**
 * @fn recibir_mensaje 
 * @brief recibe un mensaje del cliente indicado
 * @param socket_cliente socket donde se recibira el mensaje
 * @param logger puntero del archivo log al cual se enviara el estado.
*/
void recibir_mensaje(int, t_log*);
/**
 * @fn recibir_paquete
 * @brief recibe un paquete de datos del cliente indicado
 * @param socket_cliente socket del cliende desde el cual va a recibir el paquete
 * @return una lista con los valores recibidos
*/
t_list* recibir_paquete(int);
/**
 * @fn recibir_operacion
 * @brief recibe el codigo de la operacion q va a enviar el cliente
 * @param socket_cliente scket del cliende desde el cual va a recibir el codigo
 * @return retorna el codigo de la operacion que se va a enviar.
*/
int recibir_operacion(int);

int recibir_entero(int);
char *recibir_msg(int socket_cliente);

/**
 * Similar a esperar_cliente, pero no recibe la operación ni mensaje (hecho para poder
 * tener control sobre el handshake.
*/
int esperar_cliente2(int socket_servidor, t_log *logger);
#endif
