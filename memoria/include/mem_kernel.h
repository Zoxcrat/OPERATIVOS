#ifndef MEM_KERNEL_
#define MEM_KERNEL_

#include "mem_gestor.h"

void atender_kernel();
int crear_proceso(int pid, int tamanio);
void finalizar_proceso(int pid);
int crear_hilo(int pid, char *archivo_de_pseudocodigo);
void finalizar_hilo(int pid, int tid);
int asignar_memoria(int tamanio, t_proceso *proceso);
void inicializar_registros(t_registros_cpu *registros);

extern int proceso_buscado;

#endif