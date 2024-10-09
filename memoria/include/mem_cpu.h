#ifndef MEM_CPU_
#define MEM_CPU_

#include "mem_gestor.h"

t_contexto_ejecucion *obtener_contexto(int pid, int tid);
int actualizar_contexto(int pid, int tid);

#endif