#ifndef MEM_CPU_
#define MEM_CPU_

#include "mem_gestor.h"

t_contexto_ejecucion *obtener_contexto(int pid, int tid);
respuesta_pedido actualizar_contexto(int pid, int tid, t_registros_cpu *nuevo_contexto);

#endif