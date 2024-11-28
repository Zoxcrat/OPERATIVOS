#include "../include/mem_gestor.h"

t_log *logger;
t_log *logger_obligatorio;
t_config *config;

// t_contexto_ejecucion* contexto_de_ejecucion;
int socket_cliente;
int memoria_socket;
int fd_filesystem;
int fd_cpu;
int fd_kernel;

// Variables del config (Las pongo aca asi no estamos revoleando el cfg para todos lados)

char *IP_ESCUCHA;
char *PUERTO_ESCUCHA;
char *IP_FILESYSTEM;
char *PUERTO_FILESYSTEM;
int TAM_MEMORIA;
char *PATH_INSTRUCCIONES;
int RETARDO_RESPUESTA;
char *ESQUEMA;
char *ALGORITMO_BUSQUEDA;
char **PARTICIONES; // REVISAR
t_log_level LOG_LEVEL;
void destruir_hilo(t_hilo *hilo);

// Variables de memoria
void *memoria_usuario;
t_list *lista_particiones;
t_list *lista_procesos_en_memoria;
int proceso_buscado_cpu;
int proceso_buscado_kernel;