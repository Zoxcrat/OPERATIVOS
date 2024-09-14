#ifndef KERNEL_PROCESOS_HILOS_H
#define KERNEL_PROCESOS_HILOS_H

#include "./main.h"

void crear_proceso(char *archivo_pseudocodigo, int tamanio_proceso, int prioridad_hilo_0);
void inicializar_proceso(PCB* proceso);
void finalizar_proceso(int proceso_id);
void crear_hilo(PCB* proceso, int prioridad, char* archivo_pseudocodigo);
void finalizar_hilo(int pid,int hilo_id);
void liberar_PCB(PCB* hilo);
void liberar_TCB(TCB* hilo);
void mover_hilos_bloqueados_por_thread_join(TCB* hilo);
int informar_inicializacion_proceso_a_memoria(int pid, int tamanio);
int informar_finalizacion_proceso_a_memoria(int pid);
int informar_creacion_hilo_a_memoria(int pid, int tid, char* archivo_pseudocodigo);
int informar_finalizacion_hilo_a_memoria(int pid, int tid);
bool buscar_por_pid(void* proceso);
PCB* obtener_proceso_por_pid(int pid);
bool es_hilo_del_proceso(void* hilo, void* pid_proceso);
void eliminar_hilos_asociados(PCB* proceso);
TCB* buscar_hilo_por_pid_tid(int pid_a_buscar, int tid_a_buscar);
TCB* buscar_en_cola(t_list* cola, int pid_a_buscar, int tid_a_buscar);
TCB* buscar_hilo_en_cola_por_pid_tid(t_list* cola, int pid_a_buscar, int tid_a_buscar);
int obtener_numero_proximo_hilo(PCB* proceso);

#endif