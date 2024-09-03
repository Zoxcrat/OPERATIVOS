#include "./conexiones_servidor.h"

int iniciar_servidor(t_log *logger, char *ip, char *puerto)
{
    int socket_servidor;
    struct addrinfo hints, *servinfo;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    getaddrinfo(ip, puerto, &hints, &servinfo);
    // Creamos el socket de escucha del servidor
    socket_servidor = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    // Asociamos el socket a un puerto
    bind(socket_servidor, servinfo->ai_addr, servinfo->ai_addrlen);
    // Escuchamos las conexiones entrantes
    listen(socket_servidor, SOMAXCONN);
    log_info(logger, "Listo para escuchar a mi cliente");
    freeaddrinfo(servinfo);
    return socket_servidor;
}

int esperar_cliente(int socket_servidor, t_log *logger)
{
    int socket_cliente = accept(socket_servidor, NULL, NULL);
    if (socket_cliente == -1)
    {
        return -1;
    }
    log_info(logger, "Se conecto un Cliente.");
    recibir_operacion(socket_cliente);
    recibir_mensaje(socket_cliente, logger);
    return socket_cliente;
}
int recibir_operacion(int socket_cliente)
{
    int codigo_operacion;
    if (recv(socket_cliente, &codigo_operacion, sizeof(int), MSG_WAITALL) > 0)
    {
        return codigo_operacion;
    }
    else
    {
        close(socket_cliente);
        return -1;
    }
}
void *recibir_buffer(int *size, int socket_cliente)
{
    void *buffer;
    recv(socket_cliente, size, sizeof(int), MSG_WAITALL);
    buffer = malloc(*size);
    recv(socket_cliente, buffer, *size, MSG_WAITALL);
    return buffer;
}
void recibir_mensaje(int socket_cliente, t_log *logger)
{
    int size;
    char *buffer = recibir_buffer(&size, socket_cliente);
    log_info(logger, "Me llego el mensaje %s", buffer);
    free(buffer);
}
t_list *recibir_paquete(int socket_cliente)
{
    int size, desplazamiento = 0, tamanho;
    t_list *valores = list_create();
    void *buffer = recibir_buffer(&size, socket_cliente);
    while (desplazamiento < size)
    {
        memcpy(&tamanho, buffer + desplazamiento, sizeof(int));
        desplazamiento += sizeof(int);
        char *valor = malloc(tamanho);
        memcpy(valor, buffer + desplazamiento, tamanho);
        desplazamiento += tamanho;
        list_add(valores, valor);
    }
    free(buffer);
    return valores;
}

int recibir_entero(int socket_cliente)
{
    char *rta = recibir_msg(socket_cliente);

    int valor = atoi(rta);
    free(rta);
    return valor;
}
