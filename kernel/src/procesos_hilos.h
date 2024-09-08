#ifndef KERNEL_PROCESOS_HILOS_H
#define KERNEL_PROCESOS_HILOS_H

#include "kernel.h"
#include "plani_corto_plazo.h"


PCB* crear_proceso(char *archivo_pseudocodigo, int tamanio_proceso, int prioridad_hilo_0);
void inicializar_proceso(PCB* proceso);
void finalizar_proceso(int proceso_id);
void crear_hilo(PCB* proceso, int prioridad, char* archivo_pseudocodigo);
void finalizar_hilo(int hilo_id, int pid);
void liberar_PCB(t_list* lista_pcbs, int pid);
void mover_hilos_bloqueados_por(TCB* hilo);
void agregar_a_ready(TCB* tcb);
int informar_inicializacion_proceso_a_memoria(int pid, int tamanio);
int informar_finalizacion_proceso_a_memoria(int pid);
int informar_creacion_hilo_a_memoria(int pid, int tid);
int informar_finalizacion_hilo_a_memoria(int pid, int tid);

#endif