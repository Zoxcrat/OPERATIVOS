#ifndef MEM_CLEANUP_
#define MEM_CLEANUP_
#include "mem_gestor.h"
void destruir_proceso(t_proceso *proceso);
void destruir_hilo(t_hilo *hilo);
bool coincidePidKernel(t_proceso *proceso);
bool coincidePidCpu(t_proceso *proceso);
t_hilo *obtener_hilo(int pid, int tid);
void agregar_respuesta_enviar_paquete(t_paquete *paquete, respuesta_pedido respuesta);
#endif