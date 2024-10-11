#ifndef KERNEL_PLANI_LARGO_PLAZO_H
#define KERNEL_PLANI_LARGO_PLAZO_H

#include "./main.h"

////////////////////////////////////////////

void inicializar_plani_largo_plazo();
void* planificar_largo_plazo();
PCB* obtener_proceso_en_new();
void mandar_hilo_a_exit(int pid, int tid);

////////////////////////////////////////////

#endif //KERNEL_PLANI_LARGO_PLAZO_H