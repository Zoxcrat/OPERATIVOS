#include "../include/mem_fs.h"

void generarMemoryDump(int pid, int tid)
{
    t_contexto_ejecucion *contexto_completo = obtener_contexto(pid, tid);
    void *memoria_hilo = malloc(contexto_completo->limite);
    memcpy(memoria_hilo, memoria_usuario + contexto_completo->base, contexto_completo->limite);

    t_paquete *paquete = crear_paquete();
    agregar_a_paquete(paquete, pid, sizeof(int));
    agregar_a_paquete(paquete, tid, sizeof(int));
    agregar_a_paquete(paquete, contexto_completo->limite, sizeof(uint32_t));
    agregar_a_paquete(paquete, memoria_hilo, contexto_completo->limite);
    enviar_paquete(paquete, fd_filesystem);
    free(contexto_completo);
}