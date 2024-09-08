#ifndef KERNEL_PROCESOS_HILOS_H
#define KERNEL_PROCESOS_HILOS_H

#include "kernel.h"
#include "plani_corto_plazo.h"

PCB* crear_proceso(char *archivo_pseudocodigo, int tamanio_proceso, int prioridad_hilo_0);
void inicializar_proceso(PCB* proceso);
void finalizar_proceso(int proceso_id);
void crear_hilo(PCB* proceso, int prioridad, char* archivo_pseudocodigo);
void finalizar_hilo(PCB* proceso,int hilo_id);
void liberar_PCB(PCB* proceso);
void liberar_TCB(TCB* hilo);
void transicionar_hilo_a_exit(TCB* hilo);
void mover_hilos_bloqueados_por(TCB* hilo);
int informar_inicializacion_proceso_a_memoria(int pid, int tamanio);
int informar_finalizacion_proceso_a_memoria(int pid);
int informar_creacion_hilo_a_memoria(int pid, int tid, char* archivo_pseudocodigo);
int informar_finalizacion_hilo_a_memoria(int pid, int tid);
PCB* buscar_proceso_por_id(int proceso_id);
TCB* buscar_hilo_por_id_y_proceso(PCB* proceso, int hilo_id);

#endif