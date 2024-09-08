#ifndef KERNEL_PLANI_LARGO_PLAZO_H
#define KERNEL_PLANI_LARGO_PLAZO_H

#include "kernel.h"
#include "procesos_hilos.h"

////////////////////////////////////////////

void inicializar_plani_largo_plazo();
void* planificar_largo_plazo();
PCB* obtener_proceso_en_new();

////////////////////////////////////////////

#endif //KERNEL_PLANI_LARGO_PLAZO_H