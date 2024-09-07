#ifndef KERNEL_PLANI_CORTO_PLAZO_H
#define KERNEL_PLANI_CORTO_PLAZO_H

#include "kernel.h"

void inicializar_plani_corto_plazo();
void lanzar_hilo_plani_corto_plazo_con(void* (*algoritmo_plani)(void*));
void* algoritmo_fifo(void* args);
void* algoritmo_prioridades(void* args);
void* algoritmo_colas_multinivel(void* args);
void enviar_hilo_a_cpu(TCB* hilo);
void procesar_motivo_devolucion(TCB* hilo, motivo_devolucion motivo);

#endif //KERNEL_PLANI_CORTO_PLAZO_H