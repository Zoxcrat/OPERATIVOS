#ifndef KERNEL_PLANI_CORTO_PLAZO_H
#define KERNEL_PLANI_CORTO_PLAZO_H

#include "./main.h"
#include <time.h>

void inicializar_plani_corto_plazo();
void lanzar_hilo_plani_corto_plazo_con(void* (*algoritmo_plani)(void*));
void* algoritmo_fifo(void* args);
void* algoritmo_prioridades(void* args);
void* algoritmo_colas_multinivel(void* args);
void enviar_hilo_a_cpu(TCB* hilo);
void agregar_a_ready(TCB* tcb);
void manejar_thread_join(int tid_a_unir);
void iniciar_temporizador_quantum();
void inicializar_gestor_quantum();
void *gestor_quantum(void* arg);
t_cola_multinivel* obtener_o_crear_cola_con_prioridad(int prioridad_buscada);
void verificar_eliminacion_cola_multinivel(int prioridad_buscada);
t_cola_multinivel* obtener_cola_con_mayor_prioridad();


#endif //KERNEL_PLANI_CORTO_PLAZO_H