#ifndef CONTEXTO_EJECUCION_H
#define CONTEXTO_EJECUCION_H

#include <stdint.h>

typedef struct
{
    uint32_t PC, AX, BX, CX, DX, EX, FX, GX, HX, base, limite;
} t_contexto_ejecucion;

typedef struct t_registros_cpu
{
    uint32_t PC, AX, BX, CX, DX, EX, FX, GX, HX;
} t_registros_cpu;

typedef enum
{
    OK,
    ERROR,
} respuesta_pedido;

#endif // CONTEXTO_EJECUCION_H