#ifndef MEM_CPU_
#define MEM_CPU_

#include "mem_gestor.h"

t_registros_cpu *obtener_contexto(int pid, int tid);
int actualizar_contexto(int pid, int tid);

#endif