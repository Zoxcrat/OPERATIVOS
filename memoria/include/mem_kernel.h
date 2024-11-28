#ifndef MEM_KERNEL_
#define MEM_KERNEL_

#include "mem_gestor.h"

void atender_kernel();
respuesta_pedido crear_proceso(int pid, int tamanio);
respuesta_pedido finalizar_proceso(int pid);
respuesta_pedido crear_hilo(int pid, char *archivo_de_pseudocodigo);
respuesta_pedido finalizar_hilo(int pid, int tid);
respuesta_pedido asignar_memoria(int tamanio, t_proceso *proceso);
void inicializar_registros(t_registros_cpu *registros);

#endif